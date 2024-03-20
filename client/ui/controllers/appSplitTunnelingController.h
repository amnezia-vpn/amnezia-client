#ifndef APPSPLITTUNNELINGCONTROLLER_H
#define APPSPLITTUNNELINGCONTROLLER_H

#include <QObject>

#include "settings.h"
#include "ui/models/appSplitTunnelingModel.h"

class AppSplitTunnelingController : public QObject
{
    Q_OBJECT
public:
    explicit AppSplitTunnelingController(const std::shared_ptr<Settings> &settings,
                                         const QSharedPointer<AppSplitTunnelingModel> &sitesModel, QObject *parent = nullptr);

public slots:
    void addApp(const QString &appPath);
    void removeApp(const int index);

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

private:
    std::shared_ptr<Settings> m_settings;

    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
};

#endif // APPSPLITTUNNELINGCONTROLLER_H
