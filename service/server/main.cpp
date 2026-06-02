#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdlib>
#include <cstring>
#include <thread>

#include "version.h"
#include "localserver.h"
#include "logger.h"
#include "systemservice.h"
#include "xray.h"
#include "core/utils/utilities.h"

#ifdef Q_OS_WIN
#include "platforms/windows/daemon/windowsdaemontunnel.h"
#include <windows.h>

namespace {
int s_argc = 0;
char** s_argv = nullptr;
}  // namespace

#else
#include <unistd.h>
#endif

namespace {

constexpr const char* kXrayWorkerArg = "--xray-worker";

void writeWorkerEvent(const QJsonObject& obj)
{
    const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
    std::fflush(stdout);
}

void workerMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    const char* level = "debug";
    switch (type) {
    case QtDebugMsg:    level = "debug";   break;
    case QtInfoMsg:     level = "info";    break;
    case QtWarningMsg:  level = "warn";    break;
    case QtCriticalMsg: level = "error";   break;
    case QtFatalMsg:    level = "fatal";   break;
    }
    writeWorkerEvent({{"ev", "log"}, {"level", QString::fromLatin1(level)}, {"msg", msg}});
}

int readStdinChunk(char* buf, int cap)
{
#ifdef Q_OS_WIN
    DWORD n = 0;
    BOOL ok = ReadFile(GetStdHandle(STD_INPUT_HANDLE), buf, cap, &n, nullptr);
    return ok ? static_cast<int>(n) : -1;
#else
    return static_cast<int>(::read(STDIN_FILENO, buf, cap));
#endif
}

int runXrayWorker(int argc, char** argv)
{
    qInstallMessageHandler(workerMessageHandler);
    QCoreApplication app(argc, argv);

    auto* xray = new Xray;
    auto* buf  = new QByteArray;

    auto exitWorker = [xray](int code) {
        xray->stopXray();
        std::fflush(stdout);
        std::_Exit(code);
    };

    auto handleLine = [xray, exitWorker](const QByteArray& line) {
        const QJsonObject cmd = QJsonDocument::fromJson(line).object();
        const QString op = cmd.value("op").toString();
        if (op == "start") {
            const QString cfg = cmd.value("config").toString();
            const bool ok = xray->startXray(cfg);
            writeWorkerEvent({{"ev", ok ? "ready" : "failed"}});
        } else if (op == "stop") {
            writeWorkerEvent({{"ev", "stopped"}});
            exitWorker(0);
        }
    };

    auto onChunk = [buf, handleLine](const QByteArray& data) {
        buf->append(data);
        int nl;
        while ((nl = buf->indexOf('\n')) >= 0) {
            const QByteArray line = buf->left(nl);
            buf->remove(0, nl + 1);
            handleLine(line);
        }
    };

    // Detached reader thread: stdin is an anonymous pipe (the parent's write end).
    // QSocketNotifier doesn't work on anonymous pipes on Windows, so use a blocking
    // read in a thread and dispatch events back to the main thread. On EOF the
    // worker exits immediately via _Exit to avoid races with QCoreApplication
    // teardown while this thread is still alive.
    std::thread([onChunk, exitWorker]() {
        char chunk[4096];
        while (true) {
            const int n = readStdinChunk(chunk, sizeof(chunk));
            if (n <= 0) {
                // Parent gone or pipe error: shut xray down and exit.
                exitWorker(0);
                return;
            }
            QMetaObject::invokeMethod(qApp, [onChunk, data = QByteArray(chunk, n)]() {
                onChunk(data);
            }, Qt::QueuedConnection);
        }
    }).detach();

    return app.exec();
}

bool hasXrayWorkerArg(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], kXrayWorkerArg) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

int runApplication(int argc, char** argv)
{
    QCoreApplication app(argc,argv);

#ifdef Q_OS_WIN
    if(argc > 2){
        s_argc = argc;
        s_argv = argv;
        QStringList tokens;
        for (int i = 1; i < argc; ++i) {
            tokens.append(QString(argv[i]));
        }

        if (!tokens.empty() && tokens[0] == "tunneldaemon") {
            WindowsDaemonTunnel *daemon = new WindowsDaemonTunnel();
            daemon->run(tokens);
        }
    }
#endif

    LocalServer localServer;
    return app.exec();

}


int main(int argc, char **argv)
{
    if (hasXrayWorkerArg(argc, argv)) {
        return runXrayWorker(argc, argv);
    }

    Utils::initializePath(Logger::systemLogDir());

    if (argc >= 2) {
        qInfo() << "Started as console application";
        return runApplication(argc, argv);
    }
    else {
        qInfo() << "Started as system service";
#ifdef Q_OS_WIN
        SystemService systemService(argc, argv);
        return systemService.exec();
#else
    return runApplication(argc, argv);
#endif

    }
}
