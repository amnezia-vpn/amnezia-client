#include "cliControlProtocol.h"

#include <QJsonDocument>

namespace CliControl
{

    QString commandName(Command command)
    {
        switch (command) {
        case Command::Raise: return QStringLiteral("raise");
        case Command::Status: return QStringLiteral("status");
        case Command::ListServers: return QStringLiteral("list");
        case Command::Connect: return QStringLiteral("connect");
        case Command::Disconnect: return QStringLiteral("disconnect");
        case Command::Toggle: return QStringLiteral("toggle");
        case Command::Invalid: return QStringLiteral("invalid");
        }
        return QStringLiteral("invalid");
    }

    namespace
    {

        Command commandFromName(const QString &name)
        {
            if (name == QStringLiteral("raise"))
                return Command::Raise;
            if (name == QStringLiteral("status"))
                return Command::Status;
            if (name == QStringLiteral("list"))
                return Command::ListServers;
            if (name == QStringLiteral("connect"))
                return Command::Connect;
            if (name == QStringLiteral("disconnect"))
                return Command::Disconnect;
            if (name == QStringLiteral("toggle"))
                return Command::Toggle;
            return Command::Invalid;
        }

        void selectCommand(Request &request, Command command)
        {
            if (request.command != Command::Raise) {
                request.command = Command::Invalid;
                request.error = QStringLiteral("multiple_control_commands");
                return;
            }
            request.command = command;
        }

    } // namespace

    Request requestFromArguments(const QStringList &arguments)
    {
        Request request;
        for (int i = 1; i < arguments.size(); ++i) {
            const QString &argument = arguments.at(i);
            if (argument == QStringLiteral("--status")) {
                selectCommand(request, Command::Status);
            } else if (argument == QStringLiteral("--list-servers")) {
                selectCommand(request, Command::ListServers);
            } else if (argument == QStringLiteral("--disconnect")) {
                selectCommand(request, Command::Disconnect);
            } else if (argument == QStringLiteral("--toggle")) {
                selectCommand(request, Command::Toggle);
            } else if (argument == QStringLiteral("--connect")) {
                selectCommand(request, Command::Connect);
                if (request.command == Command::Invalid) {
                    continue;
                }
                if (i + 1 >= arguments.size() || arguments.at(i + 1).startsWith(QStringLiteral("--"))) {
                    request.command = Command::Invalid;
                    request.error = QStringLiteral("missing_server_id");
                    continue;
                }
                request.serverId = arguments.at(++i);
            }
        }
        return request;
    }

    Request requestFromJson(const QJsonObject &json)
    {
        Request request;
        request.command = commandFromName(json.value(QStringLiteral("command")).toString());
        request.serverId = json.value(QStringLiteral("serverId")).toString();
        if (request.command == Command::Invalid) {
            request.error = QStringLiteral("unknown_command");
        } else if (request.command == Command::Connect && request.serverId.isEmpty()) {
            request.command = Command::Invalid;
            request.error = QStringLiteral("missing_server_id");
        }
        return request;
    }

    QJsonObject requestToJson(const Request &request)
    {
        QJsonObject json { { QStringLiteral("command"), commandName(request.command) } };
        if (!request.serverId.isEmpty()) {
            json.insert(QStringLiteral("serverId"), request.serverId);
        }
        if (!request.error.isEmpty()) {
            json.insert(QStringLiteral("error"), request.error);
        }
        return json;
    }

    QByteArray toJsonLine(const QJsonObject &json)
    {
        return QJsonDocument(json).toJson(QJsonDocument::Compact) + '\n';
    }

} // namespace CliControl
