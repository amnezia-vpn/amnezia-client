#include "authData.h"

namespace amnezia
{

QJsonObject AuthData::toJson() const
{
    QJsonObject obj;
    if (!apiKey.isEmpty()) {
        obj[apiDefs::key::apiKey] = apiKey;
    }
    return obj;
}

AuthData AuthData::fromJson(const QJsonObject& json)
{
    AuthData data;
    data.apiKey = json.value(apiDefs::key::apiKey).toString();
    return data;
}

} // namespace amnezia

