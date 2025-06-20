#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <QJsonObject>

class ProtocolConfig
{
public:
    ProtocolConfig(const QString &protocolName);

    QString protocolName;
    
    virtual QJsonObject toJson() const;
};

#endif // PROTOCOLCONFIG_H
