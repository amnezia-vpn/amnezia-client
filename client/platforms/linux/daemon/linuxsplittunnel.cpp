#include "linuxsplittunnel.h"
#include "linuxfirewall.h"
#include "logger.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

namespace {
Logger logger("LinuxSplitTunnel");
const int kScanIntervalMs = 2000;

// cgroup v1: /sys/fs/cgroup/net_cls/<brand>vpnexclusions/tasks
// cgroup v2: /sys/fs/cgroup/<brand>vpnexclusions/cgroup.procs
bool isCgroupV2() {
    return !QFile::exists("/sys/fs/cgroup/net_cls");
}

QString cgroupV1Dir()  { return LinuxFirewall::cgroupPath(); }
QString cgroupV2Dir()  { return QStringLiteral("/sys/fs/cgroup/amnvpnexclusions"); }
// Path as seen by the kernel (relative to cgroup2 root, used in iptables --path)
QString cgroupV2KernelPath() { return QStringLiteral("/amnvpnexclusions"); }

QString cgroupProcsFile() {
    return isCgroupV2()
        ? cgroupV2Dir() + "/cgroup.procs"
        : cgroupV1Dir() + "/tasks";
}

int sh(const QString& cmd) {
    return QProcess::execute("/bin/bash", {"-c", cmd});
}
}

LinuxSplitTunnel::LinuxSplitTunnel(QObject* parent) : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setInterval(kScanIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &LinuxSplitTunnel::scanAndAssignProcesses);
}

LinuxSplitTunnel::~LinuxSplitTunnel() {
    stop();
}

// static
bool LinuxSplitTunnel::findPhysicalRoute(const QString& vpnServerIp, QString& gateway, QString& iface) {
    QProcess p;
    p.start("ip", {"route", "get", vpnServerIp});
    if (!p.waitForFinished(3000)) return false;

    const QString output = p.readAllStandardOutput();
    QRegularExpression re(R"(via (\S+) dev (\S+))");
    auto m = re.match(output);
    if (!m.hasMatch()) return false;

    gateway = m.captured(1);
    iface = m.captured(2);
    return true;
}

void LinuxSplitTunnel::start(const QString& physicalGateway, const QString& physicalInterface) {
    if (m_running) stop();

    m_physicalGateway = physicalGateway;
    m_physicalInterface = physicalInterface;
    m_running = true;

    logger.info() << "start() cgroup" << (isCgroupV2() ? "v2" : "v1")
                  << "gateway=" << m_physicalGateway << "iface=" << m_physicalInterface;
    setupNetworking();
    setupBypassRoute();
    scanAndAssignProcesses();
    m_timer->start();

    logger.info() << "Split tunnel started via" << m_physicalGateway << "dev" << m_physicalInterface;
}

void LinuxSplitTunnel::stop() {
    if (!m_running) return;
    m_timer->stop();
    teardownNetworking();
    teardownBypassRoute();
    m_running = false;
    logger.info() << "Split tunnel stopped";
}

void LinuxSplitTunnel::setExcludedApps(const QStringList& appPaths) {
    m_excludedAppPaths = appPaths;
    if (m_running) scanAndAssignProcesses();
}

void LinuxSplitTunnel::setupNetworking() {
    const QString tag = LinuxFirewall::packetTag();
    const QString tbl = LinuxFirewall::kRtableName;

    if (isCgroupV2()) {
        // cgroup v2: create a plain cgroup directory — no classid, match by kernel path in iptables.
        sh(QStringLiteral("mkdir -p %1").arg(cgroupV2Dir()));

        sh(QStringLiteral("iptables -t mangle -C OUTPUT -m cgroup --path %1 -j MARK --set-mark %2 2>/dev/null"
                          " || iptables -t mangle -A OUTPUT -m cgroup --path %1 -j MARK --set-mark %2")
               .arg(cgroupV2KernelPath(), tag));
    } else {
        // cgroup v1: write classid into net_cls cgroup.
        const QString cgroupDir = cgroupV1Dir();
        const QString id = LinuxFirewall::cgroupId();

        sh(QStringLiteral("if [ ! -d %1 ] ; then mkdir -p %1 && sleep 0.1 && echo %2 > %1/net_cls.classid ; fi")
               .arg(cgroupDir, id));

        sh(QStringLiteral("iptables -t mangle -C OUTPUT -m cgroup --cgroup %1 -j MARK --set-mark %2 2>/dev/null"
                          " || iptables -t mangle -A OUTPUT -m cgroup --cgroup %1 -j MARK --set-mark %2")
               .arg(id, tag));
    }

    // Ensure the table name is registered (ip commands reject unknown names)
    sh(QStringLiteral("grep -q '%1' /etc/iproute2/rt_tables"
                      " || echo '10011 %1' >> /etc/iproute2/rt_tables").arg(tbl));

    // Common: routing rule and filter accept rule
    sh(QStringLiteral("ip rule list | grep -q 'fwmark %1' || ip rule add from all fwmark %1 lookup %2 pri 100")
           .arg(tag, tbl));

    sh(QStringLiteral("iptables -C OUTPUT -m mark --mark %1 -j ACCEPT 2>/dev/null"
                      " || iptables -A OUTPUT -m mark --mark %1 -j ACCEPT")
           .arg(tag));

    // MASQUERADE so bypass traffic leaves with the physical source IP, not the VPN IP
    sh(QStringLiteral("iptables -t nat -C POSTROUTING -m mark --mark %1 -o %2 -j MASQUERADE 2>/dev/null"
                      " || iptables -t nat -A POSTROUTING -m mark --mark %1 -o %2 -j MASQUERADE")
           .arg(tag, m_physicalInterface));

    QProcess::execute("ip", {"route", "flush", "cache"});
}

void LinuxSplitTunnel::teardownNetworking() {
    const QString tag = LinuxFirewall::packetTag();
    const QString tbl = LinuxFirewall::kRtableName;

    sh(QStringLiteral("iptables -D OUTPUT -m mark --mark %1 -j ACCEPT 2>/dev/null").arg(tag));
    sh(QStringLiteral("iptables -t nat -D POSTROUTING -m mark --mark %1 -o %2 -j MASQUERADE 2>/dev/null")
           .arg(tag, m_physicalInterface));

    if (isCgroupV2()) {
        sh(QStringLiteral("iptables -t mangle -D OUTPUT -m cgroup --path %1 -j MARK --set-mark %2 2>/dev/null")
               .arg(cgroupV2KernelPath(), tag));
        // Move all procs back to root cgroup before removing the directory
        sh(QStringLiteral("cat %1/cgroup.procs 2>/dev/null | xargs -I{} sh -c 'echo {} > /sys/fs/cgroup/cgroup.procs 2>/dev/null'; rmdir %1 2>/dev/null")
               .arg(cgroupV2Dir()));
    } else {
        const QString id = LinuxFirewall::cgroupId();
        sh(QStringLiteral("iptables -t mangle -D OUTPUT -m cgroup --cgroup %1 -j MARK --set-mark %2 2>/dev/null")
               .arg(id, tag));
    }

    sh(QStringLiteral("ip rule list | grep -q 'fwmark %1' && ip rule del from all fwmark %1 lookup %2 2>/dev/null")
           .arg(tag, tbl));

    QProcess::execute("ip", {"route", "flush", "cache"});
}

void LinuxSplitTunnel::setupBypassRoute() {
    QProcess::execute("ip", {"route", "replace", "default", "via", m_physicalGateway,
                              "dev", m_physicalInterface, "table", LinuxFirewall::kRtableName});
    QProcess::execute("ip", {"route", "flush", "cache"});
}

void LinuxSplitTunnel::teardownBypassRoute() {
    QProcess::execute("ip", {"route", "del", "default", "table", LinuxFirewall::kRtableName});
    QProcess::execute("ip", {"route", "flush", "cache"});
}

void LinuxSplitTunnel::scanAndAssignProcesses() {
    if (m_excludedAppPaths.isEmpty()) return;

    const QDir procDir("/proc");
    const QStringList entries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        bool ok;
        const qint64 pid = entry.toLongLong(&ok);
        if (!ok || pid <= 0) continue;

        const QString exePath = QFile::symLinkTarget(QStringLiteral("/proc/%1/exe").arg(pid));
        if (!exePath.isEmpty() && m_excludedAppPaths.contains(exePath)) {
            assignPid(pid);
        }
    }
}

void LinuxSplitTunnel::assignPid(qint64 pid) {
    const QString path = cgroupProcsFile();
    QFile tasks(path);
    if (!tasks.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logger.warning() << "Cannot open cgroup procs file:" << path;
        return;
    }
    tasks.write(QString::number(pid).toUtf8() + "\n");
}
