#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>

#include "cli_context.h"
#include "cli_ipc.h"
#include "core/errorstrings.h"
#include "logger.h"
#include "migrations.h"
#include "utilities.h"
#include "version.h"

namespace
{

using namespace amnezia;

QString takeOption(QStringList &args, const QString &name, const QString &shortName = {})
{
    const QStringList names = shortName.isEmpty() ? QStringList { name } : QStringList { name, shortName };

    for (int i = 0; i < args.size(); ++i) {
        const QString argument = args.at(i);
        for (const auto &candidate : names) {
            const QString prefix = candidate + "=";
            if (argument.startsWith(prefix)) {
                const QString value = argument.mid(prefix.size());
                args.removeAt(i);
                return value;
            }
        }

        if (!names.contains(argument)) {
            continue;
        }
        if (i + 1 >= args.size()) {
            return {};
        }

        const QString value = args.at(i + 1);
        args.removeAt(i + 1);
        args.removeAt(i);
        return value;
    }

    return {};
}

bool takeFlag(QStringList &args, const QString &name, const QString &shortName = {})
{
    const QStringList names = shortName.isEmpty() ? QStringList { name } : QStringList { name, shortName };
    for (int i = 0; i < args.size(); ++i) {
        if (!names.contains(args.at(i))) {
            continue;
        }
        args.removeAt(i);
        return true;
    }
    return false;
}

QString readSecretFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

int parsePort(const QString &value, int fallback)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    return ok ? parsed : fallback;
}

TransportProto parseTransport(const QString &value, Proto protocol)
{
    if (!value.isEmpty()) {
        return ProtocolProps::transportProtoFromString(value);
    }
    return ProtocolProps::defaultTransportProto(protocol);
}

QString statusSummary(const QJsonObject &status)
{
    QString text = QString("State: %1").arg(status.value("state_text").toString());
    const auto activeServer = status.value("active_server").toObject();
    if (!activeServer.isEmpty()) {
        text += QString(" | Server: #%1 %2").arg(activeServer.value("index").toInt()).arg(activeServer.value("name").toString());
    }
    const QString remoteAddress = status.value("remote_address").toString();
    if (!remoteAddress.isEmpty()) {
        text += QString(" | Remote: %1").arg(remoteAddress);
    }
    return text;
}

void printServers(const QJsonObject &data)
{
    QTextStream out(stdout);
    const auto servers = data.value("servers").toArray();
    if (servers.isEmpty()) {
        out << "No servers configured.\n";
        return;
    }

    for (const auto &value : servers) {
        const auto server = value.toObject();
        out << "#" << server.value("index").toInt();
        if (server.value("default").toBool()) {
            out << " *";
        }
        out << " " << server.value("name").toString()
            << " | " << server.value("host").toString()
            << " | " << server.value("default_container_name").toString()
            << " | " << (server.value("has_installed_containers").toBool() ? "containers" : "no-containers")
            << "\n";
    }
}

void printServer(const QJsonObject &data)
{
    QTextStream out(stdout);
    const auto server = data.value("server").toObject();
    if (server.isEmpty()) {
        out << "Server not found.\n";
        return;
    }

    out << "Index: " << server.value("index").toInt() << "\n";
    out << "Name: " << server.value("name").toString() << "\n";
    out << "Host: " << server.value("host").toString() << "\n";
    out << "Default: " << (server.value("default").toBool() ? "yes" : "no") << "\n";
    out << "Default Container: " << server.value("default_container_name").toString() << "\n";
    out << "Write Access: " << (server.value("has_write_access").toBool() ? "yes" : "no") << "\n";
    out << "Containers:\n";

    for (const auto &value : server.value("containers").toArray()) {
        const auto container = value.toObject();
        out << "  - " << container.value("display_name").toString()
            << " (" << container.value("type").toString() << ")";
        if (container.value("default").toBool()) {
            out << " [default]";
        }
        out << "\n";
    }
}

void printContainers(const QJsonObject &data)
{
    QTextStream out(stdout);
    const auto containers = data.value("containers").toArray();
    if (containers.isEmpty()) {
        out << "No containers installed for this server.\n";
        return;
    }

    for (const auto &value : containers) {
        const auto container = value.toObject();
        out << container.value("display_name").toString()
            << " (" << container.value("type").toString() << ")";
        if (container.value("default").toBool()) {
            out << " [default]";
        }
        out << " | " << container.value("service_type").toString();
        out << " | " << (container.value("supported").toBool() ? "supported" : "unsupported");
        out << "\n";
    }
}

void printCountries(const QJsonObject &data)
{
    QTextStream out(stdout);
    const QString selectedCountryName = data.value("selected_country_name").toString();
    const QString selectedCountryCode = data.value("selected_country_code").toString();
    if (!selectedCountryName.isEmpty() || !selectedCountryCode.isEmpty()) {
        out << "Current: " << selectedCountryName;
        if (!selectedCountryCode.isEmpty()) {
            out << " (" << selectedCountryCode << ")";
        }
        out << "\n";
    }

    const auto countries = data.value("countries").toArray();
    if (countries.isEmpty()) {
        out << "No selectable countries available.\n";
        return;
    }

    for (const auto &value : countries) {
        const auto country = value.toObject();
        out << (country.value("selected").toBool() ? "* " : "  ")
            << country.value("name").toString()
            << " (" << country.value("code").toString() << ")";
        if (country.value("issued").toBool()) {
            out << " [issued]";
        }
        if (country.value("worker_expired").toBool()) {
            out << " [outdated]";
        }
        out << "\n";
    }
}

void printSelectedCountry(const QJsonObject &data)
{
    QTextStream out(stdout);
    const auto server = data.value("server").toObject();
    if (!server.isEmpty()) {
        out << "Server: #" << server.value("index").toInt() << " " << server.value("name").toString() << "\n";
    }

    const QString selectedCountryName = data.value("selected_country_name").toString();
    const QString selectedCountryCode = data.value("selected_country_code").toString();
    if (!selectedCountryName.isEmpty() || !selectedCountryCode.isEmpty()) {
        out << "Current: " << selectedCountryName;
        if (!selectedCountryCode.isEmpty()) {
            out << " (" << selectedCountryCode << ")";
        }
        out << "\n";
    }
}

void printStatus(const QJsonObject &status)
{
    QTextStream out(stdout);
    out << statusSummary(status) << "\n";
    const int errorCode = status.value("last_error").toInt(0);
    if (errorCode != 0) {
        out << status.value("last_error_text").toString() << "\n";
    }
}

void printHelp()
{
    QTextStream out(stdout);
    out <<
R"(amnezia-cli

Usage:
  amnezia-cli status
  amnezia-cli connect [--index N] [--wait SECONDS]
  amnezia-cli disconnect [--wait SECONDS]
  amnezia-cli daemon start|stop|status
  amnezia-cli servers list
  amnezia-cli servers show [--index N]
  amnezia-cli servers add --host HOST --user USER (--password SECRET | --key-file FILE) [--port SSH_PORT] [--name NAME]
  amnezia-cli servers import (--file FILE | --data TEXT)
  amnezia-cli servers remove --index N
  amnezia-cli servers set-default --index N
  amnezia-cli servers scan [--index N]
  amnezia-cli countries list [--index N]
  amnezia-cli countries set --index N --country CODE
  amnezia-cli containers list [--index N]
  amnezia-cli containers set-default --index N --container NAME
  amnezia-cli containers remove --index N --container NAME
  amnezia-cli install server --host HOST --user USER (--password SECRET | --key-file FILE)
                            --container NAME [--port SSH_PORT] [--container-port PORT]
                            [--transport tcp|udp|tcpandudp] [--name NAME] [--key-passphrase PASS]
  amnezia-cli install container --index N --container NAME
                               [--container-port PORT] [--transport tcp|udp|tcpandudp]
                               [--key-passphrase PASS]
  amnezia-cli logs cleanup

Global:
  --json           Print raw JSON response

Container names:
  openvpn, cloak, shadowsocks, wireguard, awg, awg2, xray, ssxray, ikev2, dns, torwebsite, sftp, socks5proxy
)";
}

int finish(const cli::Result &result, bool jsonOutput, void (*printer)(const QJsonObject &) = nullptr)
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    if (jsonOutput) {
        out << QJsonDocument(cli::resultToJson(result)).toJson(QJsonDocument::Indented);
        return result.exitCode;
    }

    if (!result.message.isEmpty()) {
        (result.ok ? out : err) << result.message << "\n";
    }

    if (result.ok && printer) {
        printer(result.data);
    }

    return result.exitCode;
}

cli::Result waitForState(const QString &desiredState, int timeoutSeconds)
{
    const qint64 deadlineMs = timeoutSeconds > 0 ? qint64(timeoutSeconds) * 1000 : 0;
    QElapsedTimer timer;
    timer.start();

    while (true) {
        const auto status = cli::ipc::sendRequest(QJsonObject { { "command", "status" } });
        if (!status.ok) {
            return status;
        }

        const auto state = status.data.value("state").toString();
        const int lastError = status.data.value("last_error").toInt();

        if (state == desiredState) {
            return cli::Result::success(statusSummary(status.data), status.data);
        }
        if ((state == "error") || (desiredState == "connected" && state == "disconnected" && lastError != 0)) {
            return cli::Result::failure(status.data.value("last_error_text").toString(), 1, status.data);
        }
        if (deadlineMs > 0 && timer.elapsed() >= deadlineMs) {
            return cli::Result::failure("Timed out waiting for requested VPN state", 1, status.data);
        }

        QThread::msleep(300);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("amnezia-cli"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral(ORGANIZATION_NAME));

    Utils::initializePath(Logger::systemLogDir());

    Migrations migrations;
    migrations.doMigrations();

    QStringList args = app.arguments().mid(1);
    const bool jsonOutput = takeFlag(args, "--json");

    if (!args.isEmpty() && args.first() == "__daemon") {
        cli::Context context;
        cli::IpcServer server(&context);
        if (!server.isListening()) {
            QTextStream(stderr) << "Failed to start CLI daemon: " << server.errorString() << "\n";
            return 1;
        }
        return app.exec();
    }

    if (args.isEmpty() || takeFlag(args, "--help", "-h")) {
        printHelp();
        return 0;
    }

    const bool daemonRunning = cli::ipc::isDaemonRunning();
    const auto runDirect = [&](auto handler) {
        cli::Context context;
        return handler(context);
    };
    const auto runViaDaemon = [&](const QJsonObject &request) {
        return cli::ipc::sendRequest(request);
    };

    const QString command = args.takeFirst();

    if (command == "daemon") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("daemon subcommand required"), jsonOutput);
        }

        const QString action = args.takeFirst();
        if (action == "start") {
            const bool started = cli::ipc::ensureDaemonRunning(QCoreApplication::applicationFilePath());
            return finish(started ? cli::Result::success("CLI daemon is running") : cli::Result::failure("Failed to start CLI daemon"),
                          jsonOutput);
        }
        if (action == "stop") {
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "daemon.shutdown" } })
                                        : cli::Result::success("CLI daemon is not running"),
                          jsonOutput);
        }
        if (action == "status") {
            return finish(cli::Result::success(daemonRunning ? "CLI daemon is running" : "CLI daemon is not running"), jsonOutput);
        }

        return finish(cli::Result::failure(QString("Unknown daemon action '%1'").arg(action)), jsonOutput);
    }

    if (command == "status") {
        if (!daemonRunning) {
            return finish(cli::Result::success("CLI daemon is not running",
                                               QJsonObject {
                                                       { "state", "disconnected" },
                                                       { "state_text", "Disconnected" },
                                                       { "last_error", 0 },
                                                       { "last_error_text", errorString(ErrorCode::NoError) },
                                               }),
                          jsonOutput, printStatus);
        }

        return finish(runViaDaemon(QJsonObject { { "command", "status" } }), jsonOutput, printStatus);
    }

    if (command == "connect") {
        const int index = parsePort(takeOption(args, "--index", "-i"), -1);
        const int timeout = parsePort(takeOption(args, "--wait"), 60);

        if (!cli::ipc::ensureDaemonRunning(QCoreApplication::applicationFilePath())) {
            return finish(cli::Result::failure("Failed to start CLI daemon"), jsonOutput);
        }

        auto result = runViaDaemon(QJsonObject {
                { "command", "connect" },
                { "index", index },
        });
        if (!result.ok || timeout == 0) {
            return finish(result, jsonOutput, printStatus);
        }

        return finish(waitForState("connected", timeout), jsonOutput, printStatus);
    }

    if (command == "disconnect") {
        const int timeout = parsePort(takeOption(args, "--wait"), 30);
        if (!daemonRunning) {
            return finish(cli::Result::success("Already disconnected"), jsonOutput);
        }

        auto result = runViaDaemon(QJsonObject { { "command", "disconnect" } });
        if (!result.ok || timeout == 0) {
            return finish(result, jsonOutput, printStatus);
        }

        return finish(waitForState("disconnected", timeout), jsonOutput, printStatus);
    }

    if (command == "servers") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("servers subcommand required"), jsonOutput);
        }

        const QString action = args.takeFirst();
        if (action == "list") {
            auto handler = [](cli::Context &context) { return context.listServers(); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.list" } }) : runDirect(handler), jsonOutput,
                          printServers);
        }
        if (action == "show") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.showServer(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.show" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }
        if (action == "add") {
            ServerCredentials credentials;
            credentials.hostName = takeOption(args, "--host");
            credentials.userName = takeOption(args, "--user");
            credentials.port = parsePort(takeOption(args, "--port"), 22);
            credentials.secretData = takeOption(args, "--password");
            if (credentials.secretData.isEmpty()) {
                credentials.secretData = takeOption(args, "--secret");
            }

            const QString keyFile = takeOption(args, "--key-file");
            if (credentials.secretData.isEmpty() && !keyFile.isEmpty()) {
                credentials.secretData = readSecretFromFile(keyFile);
            }

            const QString name = takeOption(args, "--name");
            auto handler = [name, credentials](cli::Context &context) { return context.addServer(name, credentials); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "servers.add" },
                                                       { "host", credentials.hostName },
                                                       { "user", credentials.userName },
                                                       { "secret", credentials.secretData },
                                                       { "port", credentials.port },
                                                       { "name", name },
                                               })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }
        if (action == "import") {
            const QString fileName = takeOption(args, "--file");
            const QString data = takeOption(args, "--data");
            if (fileName.isEmpty() == data.isEmpty()) {
                return finish(cli::Result::failure("Use exactly one of --file or --data"), jsonOutput);
            }

            if (!fileName.isEmpty()) {
                auto handler = [fileName](cli::Context &context) { return context.importConfigFromFile(fileName); };
                return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.import_file" }, { "file", fileName } })
                                            : runDirect(handler),
                              jsonOutput);
            }

            auto handler = [data](cli::Context &context) { return context.importConfigFromData(data); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.import_data" }, { "data", data } })
                                        : runDirect(handler),
                          jsonOutput);
        }
        if (action == "remove") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.removeServer(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.remove" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput);
        }
        if (action == "set-default") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.setDefaultServer(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.set_default" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }
        if (action == "scan") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.scanServer(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "servers.scan" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }

        return finish(cli::Result::failure(QString("Unknown servers action '%1'").arg(action)), jsonOutput);
    }

    if (command == "countries") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("countries subcommand required"), jsonOutput);
        }

        const QString action = args.takeFirst();
        if (action == "list") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.listCountries(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "countries.list" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput, printCountries);
        }
        if (action == "set") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            const QString countryCode = takeOption(args, "--country");
            auto handler = [index, countryCode](cli::Context &context) { return context.setCountry(index, countryCode); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "countries.set" },
                                                       { "index", index },
                                                       { "country", countryCode },
                                               })
                                        : runDirect(handler),
                          jsonOutput, printSelectedCountry);
        }

        return finish(cli::Result::failure(QString("Unknown countries action '%1'").arg(action)), jsonOutput);
    }

    if (command == "containers") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("containers subcommand required"), jsonOutput);
        }

        const QString action = args.takeFirst();
        if (action == "list") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            auto handler = [index](cli::Context &context) { return context.listContainers(index); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "containers.list" }, { "index", index } })
                                        : runDirect(handler),
                          jsonOutput, printContainers);
        }
        if (action == "set-default") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            const QString container = takeOption(args, "--container", "-c");
            auto handler = [index, container](cli::Context &context) {
                return context.setDefaultContainer(index, cli::containerFromCliName(container));
            };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "containers.set_default" },
                                                       { "index", index },
                                                       { "container", container },
                                               })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }
        if (action == "remove") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            const QString container = takeOption(args, "--container", "-c");
            auto handler = [index, container](cli::Context &context) {
                return context.removeContainer(index, cli::containerFromCliName(container));
            };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "containers.remove" },
                                                       { "index", index },
                                                       { "container", container },
                                               })
                                        : runDirect(handler),
                          jsonOutput);
        }

        return finish(cli::Result::failure(QString("Unknown containers action '%1'").arg(action)), jsonOutput);
    }

    if (command == "install") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("install target required"), jsonOutput);
        }

        const QString target = args.takeFirst();
        if (target == "server") {
            ServerCredentials credentials;
            credentials.hostName = takeOption(args, "--host");
            credentials.userName = takeOption(args, "--user");
            credentials.port = parsePort(takeOption(args, "--port"), 22);
            credentials.secretData = takeOption(args, "--password");
            if (credentials.secretData.isEmpty()) {
                credentials.secretData = takeOption(args, "--secret");
            }

            const QString keyFile = takeOption(args, "--key-file");
            if (credentials.secretData.isEmpty() && !keyFile.isEmpty()) {
                credentials.secretData = readSecretFromFile(keyFile);
            }

            const QString name = takeOption(args, "--name");
            const QString containerName = takeOption(args, "--container", "-c");
            const auto container = cli::containerFromCliName(containerName);
            const auto protocol = ContainerProps::defaultProtocol(container);
            const int containerPort = parsePort(takeOption(args, "--container-port"), ProtocolProps::defaultPort(protocol));
            const auto transport = parseTransport(takeOption(args, "--transport"), protocol);
            const QString keyPassphrase = takeOption(args, "--key-passphrase");

            auto handler = [name, credentials, container, containerPort, transport, keyPassphrase](cli::Context &context) {
                return context.installServer(name, credentials, container, containerPort, transport, keyPassphrase);
            };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "install.server" },
                                                       { "host", credentials.hostName },
                                                       { "user", credentials.userName },
                                                       { "secret", credentials.secretData },
                                                       { "ssh_port", credentials.port },
                                                       { "name", name },
                                                       { "container", containerName },
                                                       { "container_port", containerPort },
                                                       { "transport", ProtocolProps::transportProtoToString(transport) },
                                                       { "key_passphrase", keyPassphrase },
                                               })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }
        if (target == "container") {
            const int index = parsePort(takeOption(args, "--index", "-i"), -1);
            const QString containerName = takeOption(args, "--container", "-c");
            const auto container = cli::containerFromCliName(containerName);
            const auto protocol = ContainerProps::defaultProtocol(container);
            const int containerPort = parsePort(takeOption(args, "--container-port"), ProtocolProps::defaultPort(protocol));
            const auto transport = parseTransport(takeOption(args, "--transport"), protocol);
            const QString keyPassphrase = takeOption(args, "--key-passphrase");

            auto handler = [index, container, containerPort, transport, keyPassphrase](cli::Context &context) {
                return context.installContainer(index, container, containerPort, transport, keyPassphrase);
            };
            return finish(daemonRunning ? runViaDaemon(QJsonObject {
                                                       { "command", "install.container" },
                                                       { "index", index },
                                                       { "container", containerName },
                                                       { "container_port", containerPort },
                                                       { "transport", ProtocolProps::transportProtoToString(transport) },
                                                       { "key_passphrase", keyPassphrase },
                                               })
                                        : runDirect(handler),
                          jsonOutput, printServer);
        }

        return finish(cli::Result::failure(QString("Unknown install target '%1'").arg(target)), jsonOutput);
    }

    if (command == "logs") {
        if (args.isEmpty()) {
            return finish(cli::Result::failure("logs subcommand required"), jsonOutput);
        }

        const QString action = args.takeFirst();
        if (action == "cleanup") {
            auto handler = [](cli::Context &context) { return context.cleanupLogs(); };
            return finish(daemonRunning ? runViaDaemon(QJsonObject { { "command", "logs.cleanup" } }) : runDirect(handler), jsonOutput);
        }

        return finish(cli::Result::failure(QString("Unknown logs action '%1'").arg(action)), jsonOutput);
    }

    return finish(cli::Result::failure(QString("Unknown command '%1'").arg(command)), jsonOutput);
}
