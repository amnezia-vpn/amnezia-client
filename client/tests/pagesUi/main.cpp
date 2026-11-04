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

    return result;
}
