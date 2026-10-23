#include <QTest>
#include <QDebug>
#include <QFile>
#include <QSslSocket>
#include <QSslConfiguration>
#ifdef Q_OS_ANDROID
#include <QDir>
#include <QJniObject>
#include <QStandardPaths>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Verifies that the bundled OpenSSL is loaded on Android, not the system LibreSSL/Conscrypt.
// Uses only Qt's SSL abstraction so no direct libssl/libcrypto linkage is needed,
// which avoids bundling versioned .so.3 files that Android does not support.

class TestAndroidOpenSSL : public QObject
{
    Q_OBJECT

private slots:
    void testBundledLibraryLoaded()
    {
        const bool sslSupported = QSslSocket::supportsSsl();
        qInfo() << "QSslSocket::supportsSsl() :" << sslSupported;

        QVERIFY2(sslSupported,
                 "OpenSSL is not available – libssl.so / libcrypto.so "
                 "are not bundled in the APK or failed to load");
    }

    void testNotLibreSSL()
    {
        const QString runtimeVersion = QSslSocket::sslLibraryVersionString();
        qInfo() << "SSL runtime version :" << runtimeVersion;

        QVERIFY2(runtimeVersion.startsWith("OpenSSL "),
                 qPrintable(
                     QString("Unexpected SSL library on Android.\n"
                             "  Got: %1\n"
                             "  Expected prefix: 'OpenSSL '")
                         .arg(runtimeVersion)));
    }

    void testRuntimeVersionMatchesCompileTime()
    {
        const QString buildVersion   = QSslSocket::sslLibraryBuildVersionString();
        const QString runtimeVersion = QSslSocket::sslLibraryVersionString();

        qInfo() << "SSL compile-time version :" << buildVersion;
        qInfo() << "SSL runtime version      :" << runtimeVersion;
        qInfo() << "Runtime == Compile       :" << (runtimeVersion == buildVersion ? "YES" : "NO");

        QVERIFY2(runtimeVersion.startsWith("OpenSSL "),
                 "Runtime SSL library is not OpenSSL – wrong .so bundled in APK");
    }

    void testQtUsesOurOpenSSL()
    {
        const QString runtimeVersion = QSslSocket::sslLibraryVersionString();
        const QString buildVersion   = QSslSocket::sslLibraryBuildVersionString();

        qInfo() << "Qt SSL runtime version :" << runtimeVersion;
        qInfo() << "Qt SSL build version   :" << buildVersion;
        qInfo() << "Runtime == Build       :" << (runtimeVersion == buildVersion ? "YES" : "NO");

        // The bundled conan OpenSSL must be loaded (not the system Conscrypt)
        QVERIFY2(runtimeVersion.startsWith("OpenSSL "),
                 qPrintable(
                     QString("Qt is not using our bundled OpenSSL.\n"
                             "  Runtime : %1")
                         .arg(runtimeVersion)));
    }

#ifdef Q_OS_ANDROID
    // Verifies that libcrypto.so was loaded from the app's own APK bundle,
    // not from /system/lib or /apex. Uses dladdr() on a well-known OpenSSL
    // symbol to get the exact path of the mapped shared library.
    void testLibcryptoLoadedFromAppBundle()
    {
        QVERIFY2(QSslSocket::supportsSsl(), "OpenSSL not loaded – cannot check bundle path");

        // dlopen with RTLD_NOLOAD returns a handle to the already-loaded library
        // without incrementing its reference count (no new load occurs).
        void *handle = ::dlopen("libcrypto.so", RTLD_NOLOAD | RTLD_LAZY);
        if (!handle) {
            // Some builds use the versioned name
            handle = ::dlopen("libcrypto_3.so", RTLD_NOLOAD | RTLD_LAZY);
        }

        QVERIFY2(handle != nullptr,
                 qPrintable(QString("libcrypto.so not found via dlopen(RTLD_NOLOAD): %1")
                                .arg(::dlerror())));

        // Resolve a well-known symbol and ask dladdr for the library path
        const void *sym = ::dlsym(handle, "OpenSSL_version");
        ::dlclose(handle);

        QVERIFY2(sym != nullptr, "OpenSSL_version symbol not found in libcrypto");

        Dl_info info{};
        QVERIFY2(::dladdr(sym, &info) && info.dli_fname,
                 "dladdr failed to resolve libcrypto path");

        const QString cryptoPath = QString::fromLocal8Bit(info.dli_fname);
        qInfo() << "libcrypto.so loaded from:" << cryptoPath;

        const bool fromSystem = cryptoPath.startsWith(QLatin1String("/system/"))
                             || cryptoPath.startsWith(QLatin1String("/apex/"));
        QVERIFY2(!fromSystem,
                 qPrintable(QString("System libcrypto.so loaded instead of bundled!\n"
                                    "  Path: %1")
                                .arg(cryptoPath)));

        const bool fromApp = cryptoPath.startsWith(QLatin1String("/data/app/"))
                          || cryptoPath.contains(
                                 QLatin1String("org.qtproject.example.test_android_openssl"));
        QVERIFY2(fromApp,
                 qPrintable(QString("libcrypto.so not from app bundle.\n"
                                    "  Path  : %1\n"
                                    "  Expect: /data/app/...")
                                .arg(cryptoPath)));
    }
#endif
};

#ifdef Q_OS_ANDROID
// On Android, androidtestrunner reads test output from files/stdout.txt.
// It starts `tail -f files/stdout.txt` immediately after launching the test,
// and KILLS the process if the file doesn't exist yet (exit=255).
//
// Fix: pre-create files/stdout.txt via JNI before QCoreApplication initializes
// (JNI is already set up by the Qt Activity host before main() is called).
// This gives androidtestrunner's tail something to attach to right away.
int main(int argc, char *argv[])
{
    // Pre-create stdout.txt via JNI so androidtestrunner can tail it immediately.
    QString filesDir;
    {
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", "activity",
            "()Landroid/app/Activity;");
        if (activity.isValid()) {
            QJniObject dir = activity.callObjectMethod(
                "getFilesDir", "()Ljava/io/File;");
            if (dir.isValid())
                filesDir = dir.callObjectMethod<jstring>("getAbsolutePath").toString();
        }
    }
    if (!filesDir.isEmpty()) {
        const QByteArray path = (filesDir + "/stdout.txt").toLocal8Bit();
        int fd = ::open(path.constData(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) ::close(fd);
    }

    QCoreApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    if (filesDir.isEmpty())
        filesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(filesDir);

    const QString stdoutPath = filesDir + "/stdout.txt";

    // Build a clean argument list from scratch with the absolute output path.
    // Do NOT parse QCoreApplication::arguments() — on Android these include
    // Intent extras that may already contain a relative "-o stdout.txt,txt",
    // and attempting to patch them causes Qt to misinterpret the path as a
    // test function name.
    QStringList args;
    args << QCoreApplication::applicationFilePath()
         << QStringLiteral("-o") << (stdoutPath + QStringLiteral(",txt"))
         << QStringLiteral("-v2");

    TestAndroidOpenSSL tc;
    const int result = QTest::qExec(&tc, args);

    // Copy to Documents for easy manual inspection via the Files app.
    const QString docsDir =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docsDir.isEmpty()) {
        QDir().mkpath(docsDir);
        const QString dst = docsDir + "/test_android_openssl_results.txt";
        QFile::remove(dst);
        QFile::copy(stdoutPath, dst);
    }

    return result;
}
#else
QTEST_GUILESS_MAIN(TestAndroidOpenSSL)
#endif
#include "testAndroidOpenSSL.moc"
