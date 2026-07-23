#ifndef MARKETPLACEUPDATECONTROLLER_H
#define MARKETPLACEUPDATECONTROLLER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class MarketplaceUpdateController : public QObject
{
    Q_OBJECT

public:
    explicit MarketplaceUpdateController(QObject *parent = nullptr);

public slots:
    void start();

private:
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    QUrl versionSourceUrl() const;
    bool parseStoreVersion(const QByteArray &body, QString &version, QString &storeUrl);
    void showCover();
    void hideCover();
    void showUpdatePrompt(const QString &storeUrl);

    QNetworkAccessManager m_nam;
#endif
};

#endif // MARKETPLACEUPDATECONTROLLER_H
