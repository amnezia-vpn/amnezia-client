#ifndef SELFHOSTEDUPDATEBOOTSTRAPPER_H
#define SELFHOSTEDUPDATEBOOTSTRAPPER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

#include "core/utils/commonStructs.h"

class SecureServersRepository;

class SelfHostedUpdateBootstrapper : public QObject
{
    Q_OBJECT

public:
    explicit SelfHostedUpdateBootstrapper(SecureServersRepository *serversRepository, QObject *parent = nullptr);

    bool start();
    bool publishNow();

signals:
    void publishFinished(bool success);

private:
    struct Payload {
        QString rootDir;
        QString manifestPath;
        QString version;
        QStringList filePaths;
        QMap<QString, QString> fileSha256ByName;
        QByteArray manifestSha256;
    };

    QString findPayloadDir() const;
    bool loadPayload(const QString &payloadDir, Payload &payload) const;
    bool selectServerCredentials(amnezia::ServerCredentials &credentials) const;
    static bool publishPayload(Payload payload, amnezia::ServerCredentials credentials);

    bool m_publishScheduled = false;
    bool m_publishInProgress = false;
    bool m_publishSucceeded = false;
    SecureServersRepository *m_serversRepository = nullptr;
};

#endif // SELFHOSTEDUPDATEBOOTSTRAPPER_H
