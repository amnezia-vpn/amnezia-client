#ifndef CLI_CONTROL_PROTOCOL_H
#define CLI_CONTROL_PROTOCOL_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace CliControl
{

    enum class Command {
        Raise,
        Status,
        ListServers,
        Connect,
        Disconnect,
        Toggle,
        Invalid
    };

    struct Request
    {
        Command command = Command::Raise;
        QString serverId;
        QString error;

        bool isControlCommand() const
        {
            return command != Command::Raise;
        }
        bool isValid() const
        {
            return command != Command::Invalid;
        }
    };

    QString commandName(Command command);
    Request requestFromArguments(const QStringList &arguments);
    Request requestFromJson(const QJsonObject &json);
    QJsonObject requestToJson(const Request &request);
    QByteArray toJsonLine(const QJsonObject &json);

} // namespace CliControl

#endif // CLI_CONTROL_PROTOCOL_H
