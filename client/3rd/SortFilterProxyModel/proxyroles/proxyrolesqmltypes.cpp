#include "proxyrole.h"
#include "joinrole.h"
#include "switchrole.h"
#include "expressionrole.h"
#include "regexprole.h"
#include "filterrole.h"
#include <QQmlEngine>
#include <QCoreApplication>

namespace qqsfpm {

void registerProxyRoleTypes() {
    qmlRegisterUncreatableType<ProxyRole>("SortFilterProxyModel", 0, 2, "ProxyRole", "ProxyRole is an abstract class");
    qmlRegisterType<JoinRole>("SortFilterProxyModel", 0, 2, "JoinRole");
    qmlRegisterType<SwitchRole>("SortFilterProxyModel", 0, 2, "SwitchRole");
    qmlRegisterType<ExpressionRole>("SortFilterProxyModel", 0, 2, "ExpressionRole");
    qmlRegisterType<RegExpRole>("SortFilterProxyModel", 0, 2, "RegExpRole");
    qmlRegisterType<FilterRole>("SortFilterProxyModel", 0, 2, "FilterRole");
}

Q_COREAPP_STARTUP_FUNCTION(registerProxyRoleTypes)

}
