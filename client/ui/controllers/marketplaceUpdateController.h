#ifndef MARKETPLACEUPDATECONTROLLER_H
#define MARKETPLACEUPDATECONTROLLER_H

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
    void startNetworkCheck();
    QUrl versionSourceUrl() const;
    bool parseStoreVersion(const QByteArray &body, QString &version, QString &storeUrl);
    void showUpdatePrompt(const QString &storeUrl);
#endif

#if defined(Q_OS_ANDROID)
private slots:
    void onPlayUpdateResult(int status);
#endif
};

#endif // MARKETPLACEUPDATECONTROLLER_H
