#include <QCoreApplication>
#include <QJsonDocument>

#include "core/utils/cliControlProtocol.h"

namespace
{

    bool require(bool condition, const char *message)
    {
        if (!condition) {
            qCritical("%s", message);
        }
        return condition;
    }

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const auto connect = CliControl::requestFromArguments(
            { QStringLiteral("AmneziaVPN"), QStringLiteral("--connect"), QStringLiteral("stable-server-id") });
    if (!require(connect.command == CliControl::Command::Connect, "connect command was not parsed")
        || !require(connect.serverId == QStringLiteral("stable-server-id"), "server ID was not preserved")) {
        return 1;
    }

    const auto status = CliControl::requestFromArguments({ QStringLiteral("AmneziaVPN"), QStringLiteral("--status") });
    if (!require(status.command == CliControl::Command::Status, "status command was not parsed")) {
        return 1;
    }

    const QList<QPair<QString, CliControl::Command>> commands {
        { QStringLiteral("--list-servers"), CliControl::Command::ListServers },
        { QStringLiteral("--disconnect"), CliControl::Command::Disconnect },
        { QStringLiteral("--toggle"), CliControl::Command::Toggle }
    };
    for (const auto &[argument, expected] : commands) {
        const auto parsed = CliControl::requestFromArguments({ QStringLiteral("AmneziaVPN"), argument });
        if (!require(parsed.command == expected, "control command was not parsed")) {
            return 1;
        }
    }

    const auto missingId = CliControl::requestFromArguments(
            { QStringLiteral("AmneziaVPN"), QStringLiteral("--connect") });
    if (!require(!missingId.isValid() && missingId.error == QStringLiteral("missing_server_id"),
                 "connect without a server ID was accepted")) {
        return 1;
    }

    const auto conflict = CliControl::requestFromArguments(
            { QStringLiteral("AmneziaVPN"), QStringLiteral("--status"), QStringLiteral("--toggle") });
    if (!require(!conflict.isValid(), "conflicting commands were accepted")) {
        return 1;
    }

    const auto roundTrip = CliControl::requestFromJson(
            QJsonDocument::fromJson(CliControl::toJsonLine(CliControl::requestToJson(connect))).object());
    if (!require(roundTrip.command == connect.command && roundTrip.serverId == connect.serverId,
                 "JSON request round-trip failed")) {
        return 1;
    }

    return 0;
}
