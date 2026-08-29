#ifndef UPDATEUICONTROLLER_H
#define UPDATEUICONTROLLER_H

#include <QObject>

#include "core/controllers/updateController.h"

class UpdateUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString version READ getVersion NOTIFY updateFound)
    Q_PROPERTY(QString releaseInfoText READ getReleaseInfoText NOTIFY updateFound)
    Q_PROPERTY(QString description READ getDescription NOTIFY updateFound)
    Q_PROPERTY(QStringList tags READ getTags NOTIFY updateFound)
    Q_PROPERTY(QStringList newFeatures READ getNewFeatures NOTIFY updateFound)
    Q_PROPERTY(QStringList improvements READ getImprovements NOTIFY updateFound)
    Q_PROPERTY(QStringList bugFixes READ getBugFixes NOTIFY updateFound)
    Q_PROPERTY(int updateState READ getUpdateState NOTIFY updateStateChanged)
    Q_PROPERTY(bool isStoreUpdate READ isStoreUpdate CONSTANT)
    Q_PROPERTY(bool isCheckRunning READ isCheckRunning NOTIFY isCheckRunningChanged)

public:
    explicit UpdateUiController(UpdateController* updateController, QObject *parent = nullptr);

    QString getVersion() const;
    QString getReleaseInfoText() const;
    QString getDescription() const;
    QStringList getTags() const;
    QStringList getNewFeatures() const;
    QStringList getImprovements() const;
    QStringList getBugFixes() const;
    int getUpdateState() const;
    bool isStoreUpdate() const;
    bool isCheckRunning() const;

public slots:
    void checkForUpdates();
    void update();
    void install();
    void retry();

signals:
    void updateFound();
    void updateNotFound();
    void updateCheckFailed();
    void updateStateChanged();
    void isCheckRunningChanged();

private:
    UpdateController* m_updateController;
};

#endif // UPDATEUICONTROLLER_H
