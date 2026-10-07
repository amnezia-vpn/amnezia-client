#pragma once
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QNetworkInterface>
#include <QUuid>
#ifdef Q_OS_WIN
#include "platforms/windows/daemon/windowsfirewall.h"
#endif

// The service owns the bridge and its stdin lifetime; credentials never leave
// the service except through the inherited private pipe to its child.
class TunnelSharing final : public QObject {
public:
    static TunnelSharing &instance() { static TunnelSharing value; return value; }
    QJsonObject status() const { return {{"state", m_state}, {"message", m_message}}; }
    QJsonObject start(QJsonObject endpoint, const QString &ssid, const QString &password) {
#ifdef Q_OS_WIN
        if (m_process.state() != QProcess::NotRunning) return status();
        if (endpoint.isEmpty()) return fail("Connect XRay before enabling sharing.");
        if (ssid.toUtf8().size() < 1 || ssid.toUtf8().size() > 32 || password.size() < 8 || password.size() > 63)
            return fail("SSID must be 1–32 bytes; password must be 8–63 characters.");
        const auto dir = QCoreApplication::applicationDirPath();
        if (!QFileInfo::exists(dir + "/sharing/Share.ps1") || !QFileInfo::exists(dir + "/sharing/tapbridge.exe"))
            return fail("Sharing components are missing. Install the complete Share package.");
        m_firewallReplyFile = qEnvironmentVariable("ProgramData") +
            "/AmneziaVPN Share/log/tap-firewall-" +
            QUuid::createUuid().toString(QUuid::WithoutBraces) + ".reply";
        QDir().mkpath(qEnvironmentVariable("ProgramData") + "/AmneziaVPN Share/log");
        m_stopFile = qEnvironmentVariable("ProgramData") +
            "/AmneziaVPN Share/log/tunnel-sharing-" +
            QUuid::createUuid().toString(QUuid::WithoutBraces) + ".stop";
        m_hotspotFirewallReplyFile = qEnvironmentVariable("ProgramData") +
            "/AmneziaVPN Share/log/hotspot-firewall-" +
            QUuid::createUuid().toString(QUuid::WithoutBraces) + ".reply";
        QFile::remove(m_firewallReplyFile);
        QFile::remove(m_stopFile);
        QFile::remove(m_hotspotFirewallReplyFile);
        m_waitingForTapReply = true;
        m_waitingForHotspotReply = true;
        endpoint.insert("ssid", ssid); endpoint.insert("wifiPassword", password);
        endpoint.insert("firewallReplyFile", m_firewallReplyFile);
        endpoint.insert("stopFile", m_stopFile);
        endpoint.insert("hotspotFirewallReplyFile", m_hotspotFirewallReplyFile);
        m_state = "starting"; m_message = "Preparing TAP and Wi-Fi hotspot";
        m_process.setProgram(qEnvironmentVariable("SystemRoot") + "/System32/WindowsPowerShell/v1.0/powershell.exe");
        m_process.setArguments({"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", dir + "/sharing/Share.ps1"});
        m_process.start();
        if (!m_process.waitForStarted(3000)) return fail("Cannot start the sharing worker.");
        m_process.write(QJsonDocument(endpoint).toJson(QJsonDocument::Compact) + '\n');
        return status();
#else
        return fail("Tunnel Sharing is available on Windows only.");
#endif
    }
    QJsonObject stop() {
        if (m_process.state() != QProcess::NotRunning) {
            m_state = "stopping"; m_message = "Stopping hotspot and restoring sharing";
            if (!m_stopFile.isEmpty()) {
                QFile stop(m_stopFile);
                if (stop.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    stop.write("stop");
                    stop.close();
                }
            }
            if (m_waitingForTapReply && !m_firewallReplyFile.isEmpty()) {
                QFile reply(m_firewallReplyFile);
                if (reply.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    reply.write("stop");
                    reply.close();
                }
            }
            m_process.write("stop\n"); m_process.closeWriteChannel();
            if (m_waitingForHotspotReply && !m_hotspotFirewallReplyFile.isEmpty()) {
                QFile reply(m_hotspotFirewallReplyFile);
                if (reply.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    reply.write("stop");
                    reply.close();
                }
            }
        } else { m_state = "stopped"; m_message.clear(); }
        return status();
    }
private:
    void trace(const QString &message) const {
        const auto path = qEnvironmentVariable("ProgramData") +
                          "/AmneziaVPN Share/log/tunnel-sharing-trace.log";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) return;
        file.write((QDateTime::currentDateTime().toString(Qt::ISODateWithMs) +
                    " " + message + "\n").toUtf8());
    }
    TunnelSharing() {
        connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
            m_output += m_process.readAllStandardOutput();
            int end;
            while ((end = m_output.indexOf('\n')) >= 0) {
                const auto line = m_output.left(end).trimmed(); m_output.remove(0, end + 1);
                const auto obj = QJsonDocument::fromJson(line).object();
                trace(QStringLiteral("worker state: %1").arg(obj["state"].toString()));
                if (obj["state"].toString() == "tap-ready") {
#ifdef Q_OS_WIN
                    bool allowed = false;
                    QString error = "Dedicated TAP adapter 'Amnezia Share' was not found.";
                    bool found = false;
                    for (const auto &iface : QNetworkInterface::allInterfaces()) {
                        if (iface.humanReadableName() != "Amnezia Share") continue;
                        found = true;
                        trace(QStringLiteral("matching TAP interface index=%1 name=%2")
                                  .arg(iface.index()).arg(iface.name()));
                        auto firewall = WindowsFirewall::create(nullptr);
                        allowed = firewall && firewall->enableSharingInterface(iface.index());
                        if (!allowed) error = WindowsFirewall::lastSharingError();
                        break;
                    }
                    trace(QStringLiteral("TAP request complete found=%1 allowed=%2 error=%3")
                              .arg(found).arg(allowed).arg(error));
                    if (allowed) {
                        m_state = "starting";
                        m_message = "Настраиваю TAP и точку доступа…";
                    }
                    if (!allowed) {
                        m_state = "error";
                        m_message = error.isEmpty() ? "Unable to add TAP firewall filters." : error;
                    }
                    const QByteArray replyData = allowed
                        ? QByteArray("allow")
                        : QJsonDocument(QJsonObject{{"state", "deny"}, {"message", error}})
                              .toJson(QJsonDocument::Compact);
                    QFile reply(m_firewallReplyFile);
                    const bool replyWritten = reply.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                                              reply.write(replyData) == replyData.size();
                    reply.close();
                    m_waitingForTapReply = false;
                    trace(QStringLiteral("TAP reply file written=%1 bytes=%2")
                              .arg(replyWritten).arg(replyData.size()));
                    if (!replyWritten) {
                        m_state = "error";
                        m_message = "Cannot deliver the TAP firewall result to the sharing worker.";
                    }
#endif
                    continue;
                }
                if (obj["state"].toString() == "hotspot-interface-ready") {
#ifdef Q_OS_WIN
                    bool allowed = false;
                    const int interfaceIndex = obj["interfaceIndex"].toInt();
                    const int tapInterfaceIndex = obj["tapInterfaceIndex"].toInt();
                    const QString hotspotSubnet = obj["clientSubnet"].toString();
                    QString error = "Wi-Fi Direct or TAP interface index is missing.";
                    if (interfaceIndex > 0 && tapInterfaceIndex > 0 && !hotspotSubnet.isEmpty()) {
                        auto firewall = WindowsFirewall::create(nullptr);
                        allowed = firewall &&
                                  firewall->enableSharingDhcpServer(interfaceIndex) &&
                                  firewall->enableSharingForwarding(interfaceIndex,
                                                                    tapInterfaceIndex,
                                                                    hotspotSubnet);
                        if (!allowed) error = WindowsFirewall::lastSharingError();
                    }
                    trace(QStringLiteral("hotspot forwarding firewall request wifi=%1 tap=%2 allowed=%3 error=%4")
                              .arg(interfaceIndex).arg(tapInterfaceIndex).arg(allowed).arg(error));
                    const QByteArray replyData = allowed
                        ? QByteArray("allow")
                        : QJsonDocument(QJsonObject{{"state", "deny"}, {"message", error}})
                              .toJson(QJsonDocument::Compact);
                    QFile reply(m_hotspotFirewallReplyFile);
                    const bool replyWritten = reply.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                                              reply.write(replyData) == replyData.size();
                    reply.close();
                    m_waitingForHotspotReply = false;
                    if (!replyWritten) {
                        m_state = "error";
                        m_message = "Cannot deliver the Wi-Fi DHCP firewall result to the sharing worker.";
                    } else if (!allowed) {
                        m_state = "error";
                        m_message = error;
                    }
#endif
                    continue;
                }
                if (obj.contains("state")) {
                    const auto nextState = obj["state"].toString();
                    // Keep a native WFP failure diagnostic: the PowerShell
                    // worker only knows that its TAP permission handshake was
                    // rejected and would otherwise replace the useful reason.
                    const bool keepExistingError = m_state == "error" &&
                                                   nextState == "error" &&
                                                   !m_message.isEmpty();
                    if (!keepExistingError) {
                        m_state = nextState;
                        m_message = obj["message"].toString();
                    }
                }
            }
        });
        connect(&m_process, &QProcess::readyReadStandardError, this, [this] { m_process.readAllStandardError(); });
        connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
            m_waitingForTapReply = false;
            m_waitingForHotspotReply = false;
            if (!m_firewallReplyFile.isEmpty()) QFile::remove(m_firewallReplyFile);
            if (!m_hotspotFirewallReplyFile.isEmpty()) QFile::remove(m_hotspotFirewallReplyFile);
            if (!m_stopFile.isEmpty()) QFile::remove(m_stopFile);
            m_stopFile.clear();
#ifdef Q_OS_WIN
            if (auto firewall = WindowsFirewall::create(nullptr)) firewall->disableSharingInterface();
#endif
            if (m_state != "error") {
                m_state = code == 0 ? "stopped" : "error";
                m_message = code == 0 ? QString() : "Sharing worker exited unexpectedly.";
            }
        });
    }
    ~TunnelSharing() { stop(); if (!m_process.waitForFinished(15000)) { m_process.kill(); m_process.waitForFinished(3000); } }
    QJsonObject fail(const QString &message) { m_state = "error"; m_message = message; return status(); }
    QProcess m_process;
    QByteArray m_output;
    QString m_firewallReplyFile;
    QString m_hotspotFirewallReplyFile;
    QString m_stopFile;
    bool m_waitingForTapReply = false;
    bool m_waitingForHotspotReply = false;
    QString m_state = "stopped", m_message;
};
