#ifndef MARKETPLACEUPDATECONTROLLER_H
#define MARKETPLACEUPDATECONTROLLER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class UpdateController;

// Detects a newer version published in the App Store / Google Play and asks the UI
// to show the same changelog drawer the desktop updater uses. The store is only the
// trigger; the changelog text itself comes from UpdateController (gateway).
class MarketplaceUpdateController : public QObject
{
    Q_OBJECT

public:
    explicit MarketplaceUpdateController(UpdateController *updateController, QObject *parent = nullptr);

public slots:
    void start();
    void openStoreUrl();

signals:
    // Emitted when the store advertises a newer version and the changelog fetch has settled
    void updateAvailable();

private:
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    QUrl versionSourceUrl() const;
    bool parseStoreVersion(const QByteArray &body, QString &version, QString &storeUrl);
    void fetchChangelogAndNotify();
#endif

    UpdateController *m_updateController;
    QNetworkAccessManager m_nam;
    QString m_storeUrl;
};

#endif // MARKETPLACEUPDATECONTROLLER_H
