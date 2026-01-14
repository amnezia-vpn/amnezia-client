#ifndef PROTOCOLSUICONTROLLER_H
#define PROTOCOLSUICONTROLLER_H

#include <QObject>
#include <QJsonObject>

#include "ui/models/protocolsModel.h"

class ProtocolsUiController : public QObject
{
    Q_OBJECT
    
public:
    explicit ProtocolsUiController(ProtocolsModel* protocolsModel, QObject *parent = nullptr);
    
public slots:
    void updateProtocols(const QJsonObject &config);
    QJsonObject getProtocolsConfig();

private:
    ProtocolsModel* m_protocolsModel;
};

#endif // PROTOCOLSUICONTROLLER_H

