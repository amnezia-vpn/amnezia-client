#ifndef UPDATEUICONTROLLER_H
#define UPDATEUICONTROLLER_H

#include <QObject>

#include "core/controllers/updateController.h"

class UpdateUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString changelogText READ getChangelogText NOTIFY updateFound)
    Q_PROPERTY(QString headerText READ getHeaderText NOTIFY updateFound)
    Q_PROPERTY(bool checking READ isChecking NOTIFY checkingChanged)

public:
    explicit UpdateUiController(UpdateController* updateController, QObject *parent = nullptr);

    QString getHeaderText() const;
    QString getChangelogText() const;
    QString getVersion() const;
    bool isChecking() const;

public slots:
    void checkForUpdates();
    void runInstaller();

signals:
    void updateFound();
    void manualUpdateCheckStarted();
    void manualUpdateCheckNoUpdates();
    void checkingChanged();

private:
    void onUpdateCheckFinished(bool updateAvailable);

    UpdateController* m_updateController;
    bool m_manualCheckRunning = false;
    bool m_isChecking = false;
};

#endif // UPDATEUICONTROLLER_H
