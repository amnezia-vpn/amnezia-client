#ifndef AUTHDATA_H
#define AUTHDATA_H

#include <QJsonObject>
#include <QString>

#include "core/utils/api/apiDefs.h"

namespace amnezia
{

struct AuthData {
    QString apiKey;
    
    QJsonObject toJson() const;
    static AuthData fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // AUTHDATA_H

