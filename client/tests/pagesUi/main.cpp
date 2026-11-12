#include <QApplication>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QUrl>
#include <QTest>

#include "ui/models/containerProps.h"
#include "ui/utils/pageEnum.h"
#include "core/utils/protocolEnum.h"

int runPageSetupWizardConfigSourceTests(int argc, char *argv[]);
int runPageHomeTests(int argc, char *argv[]);
int runPageSettingsConnectionTests(int argc, char *argv[]);
int runPageSettingsDnsTests(int argc, char *argv[]);
int runPageSettingsSplitTunnelingTests(int argc, char *argv[]);
int runPageSettingsAppSplitTunnelingTests(int argc, char *argv[]);
int runPageSettingsKillSwitchTests(int argc, char *argv[]);
int runPageSettingsKillSwitchExceptionsTests(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QApplication app(argc, argv);

    PageLoader::declareQmlPageEnum();

    qmlRegisterUncreatableMetaObject(amnezia::ProtocolEnumNS::staticMetaObject, "ProtocolEnum", 1, 0, "ProtocolEnum", QStringLiteral("Error: only enums"));

    static ContainerProps containerProps;

    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", &containerProps);
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/ui/qml/Filters/ContainersModelFilters.qml")), "ContainersModelFilters", 1, 0, "ContainersModelFilters");

    int result = 0;

    result |= runPageSetupWizardConfigSourceTests(argc, argv);
    result |= runPageHomeTests(argc, argv);
    result |= runPageSettingsConnectionTests(argc, argv);
    result |= runPageSettingsDnsTests(argc, argv);
    result |= runPageSettingsSplitTunnelingTests(argc, argv);
    result |= runPageSettingsAppSplitTunnelingTests(argc, argv);
    result |= runPageSettingsKillSwitchTests(argc, argv);
    result |= runPageSettingsKillSwitchExceptionsTests(argc, argv);

    return result;
}
