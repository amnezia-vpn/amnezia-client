#ifndef UPDATECONTROLLER_H
#define UPDATECONTROLLER_H

#include <QObject>

#include "settings.h"

class UpdateController : public QObject
{
    Q_OBJECT
public:
    explicit UpdateController(const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

    Q_PROPERTY(QString changelogText READ getChangelogText NOTIFY updateFound)
    Q_PROPERTY(QString headerText READ getHeaderText NOTIFY updateFound)
public slots:
    QString getHeaderText();
    QString getChangelogText();

    void checkForUpdates();
    void runInstaller();
signals:
    void updateFound();
    void errorOccured(const QString &errorMessage);
private:
    std::shared_ptr<Settings> m_settings;

    QString m_changelogText;
    QString m_version;
    QString m_releaseDate;
    QString m_downloadUrl;
};

#endif // UPDATECONTROLLER_H
