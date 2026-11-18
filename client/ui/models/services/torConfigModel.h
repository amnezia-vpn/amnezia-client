#ifndef TORCONFIGMODEL_H
#define TORCONFIGMODEL_H

#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/torProtocolConfig.h"

class TorConfigModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString site READ site WRITE setSite NOTIFY changed)

public:
    explicit TorConfigModel(QObject *parent = nullptr);

    QString site() const;
    void setSite(const QString &value);

public slots:
    void updateModel(amnezia::DockerContainer container, const amnezia::TorProtocolConfig &protocolConfig);
    amnezia::TorProtocolConfig getProtocolConfig();

signals:
    void changed();

private:
    amnezia::DockerContainer m_container;
    amnezia::TorProtocolConfig m_protocolConfig;
    amnezia::TorProtocolConfig m_originalProtocolConfig;
};

#endif // TORCONFIGMODEL_H

