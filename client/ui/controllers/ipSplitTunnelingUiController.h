#ifndef IPSPLITTUNNELINGUICONTROLLER_H
#define IPSPLITTUNNELINGUICONTROLLER_H

#include <QObject>

#include "core/controllers/ipSplitTunnelingController.h"
#include "ui/models/ipSplitTunnelingModel.h"

class IpSplitTunnelingUiController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int routeMode READ getRouteMode WRITE setRouteMode NOTIFY routeModeChanged)
    Q_PROPERTY(bool isSplitTunnelingEnabled READ isSplitTunnelingEnabled NOTIFY isSplitTunnelingEnabledChanged)

    /// <summary>Указывает, включена ли обработка динамических доменов.</summary>
    Q_PROPERTY(bool dynamicDomainRoutingEnabled READ isDynamicDomainRoutingEnabled WRITE setDynamicDomainRoutingEnabled
                       NOTIFY dynamicDomainRoutingEnabledChanged)

public:
    explicit IpSplitTunnelingUiController(IpSplitTunnelingController* ipSplitTunnelingController,
                                         IpSplitTunnelingModel* ipSplitTunnelingModel, QObject *parent = nullptr);

public slots:
    void addSite(QString hostname);
    void removeSite(int index);
    void removeSites();
    void importSites(const QString &fileName, bool replaceExisting);
    void exportSites(const QString &fileName);
    void toggleSplitTunneling(bool enabled);
    void setRouteMode(int routeMode);

    int getRouteMode() const;
    bool isSplitTunnelingEnabled() const;

    /// <summary>Возвращает состояние обработки динамических доменов.</summary>
    /// <returns>Признак включения доменной маршрутизации.</returns>
    bool isDynamicDomainRoutingEnabled() const;
    /// <summary>Меняет состояние обработки динамических доменов.</summary>
    /// <param name="enabled">Новое состояние настройки.</param>
    void setDynamicDomainRoutingEnabled(bool enabled);

    void updateModel();

signals:
    void routeModeChanged();
    void isSplitTunnelingEnabledChanged();
    /// <summary>Уведомляет об изменении настройки доменной маршрутизации.</summary>
    void dynamicDomainRoutingEnabledChanged();
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void saveFile(const QString &fileName, const QString &data);

private:
    IpSplitTunnelingController* m_ipSplitTunnelingController;
    IpSplitTunnelingModel* m_ipSplitTunnelingModel;
};

#endif // IPSPLITTUNNELINGUICONTROLLER_H
