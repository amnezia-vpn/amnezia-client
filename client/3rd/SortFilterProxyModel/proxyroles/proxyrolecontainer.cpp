#include "proxyrolecontainer.h"

namespace qqsfpm {

QList<ProxyRole*> ProxyRoleContainer::proxyRoles() const
{
    return m_proxyRoles;
}

void ProxyRoleContainer::appendProxyRole(ProxyRole* proxyRole)
{
    m_proxyRoles.append(proxyRole);
    onProxyRoleAppended(proxyRole);
}

void ProxyRoleContainer::removeProxyRole(ProxyRole* proxyRole)
{
    m_proxyRoles.removeOne(proxyRole);
    onProxyRoleRemoved(proxyRole);
}

void ProxyRoleContainer::clearProxyRoles()
{
    m_proxyRoles.clear();
    onProxyRolesCleared();
}

QQmlListProperty<ProxyRole> ProxyRoleContainer::proxyRolesListProperty()
{
    return QQmlListProperty<ProxyRole>(reinterpret_cast<QObject*>(this), &m_proxyRoles,
                                    &ProxyRoleContainer::append_proxyRole,
                                    &ProxyRoleContainer::count_proxyRole,
                                    &ProxyRoleContainer::at_proxyRole,
                                    &ProxyRoleContainer::clear_proxyRoles);
}

void ProxyRoleContainer::append_proxyRole(QQmlListProperty<ProxyRole>* list, ProxyRole* proxyRole)
{
    if (!proxyRole)
        return;

    ProxyRoleContainer* that = reinterpret_cast<ProxyRoleContainer*>(list->object);
    that->appendProxyRole(proxyRole);
}

int ProxyRoleContainer::count_proxyRole(QQmlListProperty<ProxyRole>* list)
{
    QList<ProxyRole*>* ProxyRoles = static_cast<QList<ProxyRole*>*>(list->data);
    return ProxyRoles->count();
}

ProxyRole* ProxyRoleContainer::at_proxyRole(QQmlListProperty<ProxyRole>* list, int index)
{
    QList<ProxyRole*>* ProxyRoles = static_cast<QList<ProxyRole*>*>(list->data);
    return ProxyRoles->at(index);
}

void ProxyRoleContainer::clear_proxyRoles(QQmlListProperty<ProxyRole> *list)
{
    ProxyRoleContainer* that = reinterpret_cast<ProxyRoleContainer*>(list->object);
    that->clearProxyRoles();
}

}
