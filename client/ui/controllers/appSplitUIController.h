#ifndef APPSPLITUICONTROLLER_H
#define APPSPLITUICONTROLLER_H

#include <QObject>

#include "ui/models/appSplitTunnelingModel.h"
#include "core/controllers/splitTunnelingController.h"

class AppSplitUIController : public QObject
{
    Q_OBJECT
public:
    explicit AppSplitUIController(QSharedPointer<SplitTunnelingController> splitTunnelingController,
                                  const QSharedPointer<AppSplitTunnelingModel> &appSplitTunnelingModel, 
                                  QObject *parent = nullptr);

public slots:
    void addApp(const QString &appPath);
    void addApps(QVector<QPair<QString, QString>> apps);
    void removeApp(const int index);

signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);

private:
    QSharedPointer<SplitTunnelingController> m_splitTunnelingController;
    QSharedPointer<AppSplitTunnelingModel> m_appSplitTunnelingModel;
};

#endif // APPSPLITUICONTROLLER_H
