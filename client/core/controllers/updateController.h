#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class UpdateController : public QObject
{
    Q_OBJECT

public:
    explicit UpdateController(QObject *parent = nullptr);

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void openDownloadUrl();

signals:
    void updateChecked(bool hasUpdate, const QString &version, const QString &notes, bool isCritical);

private slots:
    void onVersionCheckFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    QString m_downloadUrl;
    bool m_hasChecked = false;

    bool isNewerVersion(const QString &current, const QString &latest);
};

#endif // UPDATECONTROLLER_H
