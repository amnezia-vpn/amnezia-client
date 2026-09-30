#include <QTest>
#include <QDebug>
#include <QSslSocket>

#include <openssl/crypto.h>
#include <openssl/opensslv.h>

class TestOpenSSLLibrary : public QObject
{
    Q_OBJECT

private slots:
    void testRuntimeVersionMatchesCompileTime()
    {
        const QString compileVersion = QString::fromUtf8(OPENSSL_VERSION_TEXT);
        const QString runtimeVersion = QString::fromUtf8(OpenSSL_version(OPENSSL_VERSION));

        qInfo() << "OpenSSL compile-time version :" << compileVersion;
        qInfo() << "OpenSSL runtime version      :" << runtimeVersion;

        // If the system library (e.g. macOS LibreSSL) was loaded instead of ours,
        // the runtime string will differ from the compile-time macro.
        QVERIFY2(runtimeVersion == compileVersion,
                 qPrintable(
                     QString("OpenSSL version mismatch – system library loaded instead of bundled one.\n"
                             "  Compile-time : %1\n"
                             "  Runtime      : %2")
                         .arg(compileVersion, runtimeVersion)));
    }

    void testNotSystemLibreSSL()
    {
        const QString runtimeVersion = QString::fromUtf8(OpenSSL_version(OPENSSL_VERSION));

        qInfo() << "OpenSSL runtime version:" << runtimeVersion;

        QVERIFY2(runtimeVersion.startsWith("OpenSSL "),
                 qPrintable(
                     QString("System library loaded instead of bundled OpenSSL.\n"
                             "  Got: %1\n"
                             "  Expected prefix: 'OpenSSL '")
                         .arg(runtimeVersion)));
    }

    void testQtUsesOurOpenSSL()
    {
        const QString qtRuntimeVersion = QSslSocket::sslLibraryVersionString();
        const QString qtBuildVersion   = QSslSocket::sslLibraryBuildVersionString();
        const QString ourRuntime       = QString::fromUtf8(OpenSSL_version(OPENSSL_VERSION));

        qInfo() << "Qt SSL runtime version :" << qtRuntimeVersion;
        qInfo() << "Qt SSL build   version :" << qtBuildVersion;
        qInfo() << "App OpenSSL runtime    :" << ourRuntime;
        qInfo() << "Qt runtime == App runtime:" << (qtRuntimeVersion == ourRuntime ? "YES" : "NO");

        QVERIFY2(qtRuntimeVersion.startsWith("OpenSSL "),
                 qPrintable("Qt is not using OpenSSL at runtime, got: " + qtRuntimeVersion));

#ifndef Q_OS_WIN
        QVERIFY2(qtRuntimeVersion == ourRuntime,
                 qPrintable(
                     QString("Qt SSL library differs from the OpenSSL linked by the app.\n"
                             "  Qt runtime  : %1\n"
                             "  App runtime : %2")
                         .arg(qtRuntimeVersion, ourRuntime)));
#endif
    }

#ifdef Q_OS_WIN
    void testWindowsNoDllHijack()
    {
        const QString runtimeVersion = QString::fromUtf8(OpenSSL_version(OPENSSL_VERSION));
        const QString compileVersion = QString::fromUtf8(OPENSSL_VERSION_TEXT);
        const QString opensslDir     = QString::fromUtf8(OpenSSL_version(OPENSSL_DIR));

        qInfo() << "Windows OpenSSL runtime version :" << runtimeVersion;
        qInfo() << "Windows OpenSSL compile version :" << compileVersion;
        qInfo() << "Windows OpenSSL configured dir  :" << opensslDir;
        qInfo() << "Runtime == Compile              :" << (runtimeVersion == compileVersion ? "YES" : "NO");

        QVERIFY2(runtimeVersion == compileVersion,
                 qPrintable(
                     QString("Wrong OpenSSL DLL loaded on Windows (possible PATH hijack).\n"
                             "  Expected (compile-time) : %1\n"
                             "  Got (runtime)           : %2\n"
                             "  OpenSSL configured dir  : %3")
                         .arg(compileVersion, runtimeVersion, opensslDir)));
    }
#endif // Q_OS_WIN
};

QTEST_MAIN(TestOpenSSLLibrary)
#include "testOpenSSLLibrary.moc"
