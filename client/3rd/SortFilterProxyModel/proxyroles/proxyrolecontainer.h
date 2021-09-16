#ifndef PROXYROLECONTAINER_H
#define PROXYROLECONTAINER_H

#include <QList>
#include <QQmlListProperty>

namespace qqsfpm {

class ProxyRole;
class QQmlSortFilterProxyModel;

class ProxyRoleContainer {
public:
    virtual ~ProxyRoleContainer() = default;

    QList<ProxyRole*> proxyRoles() const;
    void appendProxyRole(ProxyRole* proxyRole);
    void removeProxyRole(ProxyRole* proxyRole);
    void clearProxyRoles();

    QQmlListProperty<ProxyRole> proxyRolesListProperty();

protected:
    QList<ProxyRole*> m_proxyRoles;

private:
    virtual void onProxyRoleAppended(ProxyRole* proxyRole) = 0;
    virtual void onProxyRoleRemoved(ProxyRole* proxyRole) = 0;
    virtual void onProxyRolesCleared() = 0;

    static void append_proxyRole(QQmlListProperty<ProxyRole>* list, ProxyRole* proxyRole);
    static int count_proxyRole(QQmlListProperty<ProxyRole>* list);
    static ProxyRole* at_proxyRole(QQmlListProperty<ProxyRole>* list, int index);
    static void clear_proxyRoles(QQmlListProperty<ProxyRole>* list);
};

}

#define ProxyRoleContainer_iid "fr.grecko.SortFilterProxyModel.ProxyRoleContainer"
Q_DECLARE_INTERFACE(qqsfpm::ProxyRoleContainer, ProxyRoleContainer_iid)

#endif // PROXYROLECONTAINER_H
