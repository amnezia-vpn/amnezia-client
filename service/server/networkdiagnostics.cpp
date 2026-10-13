#include "networkdiagnostics.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

constexpr int kProcessTimeoutMsec = 30000;
const QString kSnapshotLabel = QStringLiteral("diagnostics");

#if defined(Q_OS_WIN)
const QStringList kSectionOrder = { "system",          "adapters",           "link-type",
                                     "drivers-known",   "drivers-store",      "drivers-net-class",
                                     "routes",          "route-resolution",   "dns",
                                     "ipv6-localhost",  "proxy",              "services-processes",
                                     "firewall",        "wfp-filters" };
#elif defined(Q_OS_LINUX)
const QStringList kSectionOrder = { "system",     "adapters",           "link-type",     "drivers",
                                     "routes",     "route-resolution",   "dns",           "ipv6-localhost",
                                     "proxy",      "services-processes", "firewall",      "netfilter-full" };
#elif defined(Q_OS_MACOS)
const QStringList kSectionOrder = { "system",     "adapters",           "link-type",     "drivers",
                                     "routes",     "route-resolution",   "dns",           "ipv6-localhost",
                                     "proxy",      "services-processes", "firewall" };
#endif

}

QString NetworkDiagnostics::run()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_LINUX) && !defined(Q_OS_MACOS)
    return QStringLiteral("ERROR: network diagnostics is not supported on this platform");
#else
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return QStringLiteral("ERROR: failed to create secure temp directory");
    }

#if defined(Q_OS_WIN)
    const QString resourcePath = QStringLiteral(":/network_diagnostics/network-diagnostics-windows.ps1");
    const QString scriptPath = tempDir.filePath(QStringLiteral("network-diagnostics-windows.ps1"));
#elif defined(Q_OS_MACOS)
    const QString resourcePath = QStringLiteral(":/network_diagnostics/network-diagnostics-macos.sh");
    const QString scriptPath = tempDir.filePath(QStringLiteral("network-diagnostics-macos.sh"));
#else
    const QString resourcePath = QStringLiteral(":/network_diagnostics/network-diagnostics-linux.sh");
    const QString scriptPath = tempDir.filePath(QStringLiteral("network-diagnostics-linux.sh"));
#endif

    if (!QFile::copy(resourcePath, scriptPath)) {
        return QStringLiteral("ERROR: failed to extract diagnostics script");
    }

    QProcess process;
    process.setWorkingDirectory(tempDir.path()); // scripts write ./snapshot-<label>/ relative to CWD

#if defined(Q_OS_WIN)
    process.start(QStringLiteral("powershell"),
                   { "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-WindowStyle", "Hidden",
                     "-File", scriptPath, "-Label", kSnapshotLabel });
#else
    process.start(QStringLiteral("/bin/bash"), { scriptPath, "--label", kSnapshotLabel });
#endif

    if (!process.waitForStarted(5000)) {
        return QStringLiteral("ERROR: failed to start diagnostics script (%1)").arg(process.errorString());
    }
    if (!process.waitForFinished(kProcessTimeoutMsec)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("ERROR: diagnostics script timed out after %1 s").arg(kProcessTimeoutMsec / 1000);
    }

    const QString snapshotDir = tempDir.filePath(QStringLiteral("snapshot-%1").arg(kSnapshotLabel));
    QString combined;
    QTextStream out(&combined);
    int sectionsFound = 0;
    for (const QString &section : kSectionOrder) {
        QFile sectionFile(QStringLiteral("%1/%2.txt").arg(snapshotDir, section));
        if (!sectionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out << "=== " << section << " === (section missing)\n\n";
            continue;
        }
        QString sectionText = QString::fromUtf8(sectionFile.readAll());
        if (sectionText.startsWith(QChar(0xFEFF))) {
            sectionText.remove(0, 1);
        }
        out << sectionText << "\n";
        sectionsFound++;
    }

    if (sectionsFound == 0) {
        return QStringLiteral("ERROR: diagnostics script produced no output (exit code %1)").arg(process.exitCode());
    }

    return combined;
#endif
}
