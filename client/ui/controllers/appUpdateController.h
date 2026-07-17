#ifndef APPUPDATECONTROLLER_H
#define APPUPDATECONTROLLER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class AppUpdateController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool blocking READ blocking NOTIFY stateChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY stateChanged)
    Q_PROPERTY(bool noInternet READ noInternet NOTIFY stateChanged)
    Q_PROPERTY(bool updateRequired READ updateRequired NOTIFY stateChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString storeVersion READ storeVersion NOTIFY stateChanged)

public:
    explicit AppUpdateController(QObject *parent = nullptr);

    bool blocking() const { return m_state != UpToDate; }
    bool checking() const { return m_state == Checking; }
    bool noInternet() const { return m_state == NoInternet; }
    bool updateRequired() const { return m_state == UpdateRequired; }
    QString currentVersion() const;
    QString storeVersion() const { return m_storeVersion; }

public slots:
    void start();
    void recheck() { start(); }
    void openStore();
    void quit();

signals:
    void stateChanged();

private:
    enum State {
        Checking,
        NoInternet,
        UpdateRequired,
        UpToDate
    };

    void setState(State state);

#if defined(Q_OS_IOS) || (defined(Q_OS_ANDROID) && defined(QT_DEBUG))
    void startHttpCheck(const QUrl &url);
    void applyStoreVersion(const QString &version, const QString &storeUrl);
    QUrl versionSourceUrl() const;
    bool parseVersion(const QByteArray &body, QString &version, QString &storeUrl);
#endif

    QNetworkAccessManager m_nam;
    State m_state { Checking };
    QString m_storeVersion;
    QString m_storeUrl;
};

#endif // APPUPDATECONTROLLER_H
