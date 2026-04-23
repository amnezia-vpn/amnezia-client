#ifndef CLI_COMMON_H
#define CLI_COMMON_H

#include <QJsonObject>
#include <QString>

namespace cli
{

struct Result
{
    bool ok = false;
    int exitCode = 1;
    QString message;
    QJsonObject data;

    static Result success(const QString &message = {}, const QJsonObject &data = {})
    {
        return { true, 0, message, data };
    }

    static Result failure(const QString &message, int exitCode = 1, const QJsonObject &data = {})
    {
        return { false, exitCode, message, data };
    }
};

inline QJsonObject resultToJson(const Result &result)
{
    QJsonObject json;
    json["ok"] = result.ok;
    json["exit_code"] = result.exitCode;
    json["message"] = result.message;
    json["data"] = result.data;
    return json;
}

inline Result resultFromJson(const QJsonObject &json)
{
    Result result;
    result.ok = json.value("ok").toBool(false);
    result.exitCode = json.value("exit_code").toInt(result.ok ? 0 : 1);
    result.message = json.value("message").toString();
    result.data = json.value("data").toObject();
    return result;
}

} // namespace cli

#endif // CLI_COMMON_H
