#include <QApplication>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTest>

#include "ui/utils/pageEnum.h"

using namespace amnezia;

int runPageSetupWizardConfigSourceTests(int argc, char *argv[]);
int runPageHomeTests(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QApplication app(argc, argv);

    PageLoader::declareQmlPageEnum();

    qmlRegisterUncreatableMetaObject(ProtocolEnumNS::staticMetaObject, "ProtocolEnum", 1, 0, "ProtocolEnum", QStringLiteral("Error: only enums"));

    int result = 0;

    result |= runPageSetupWizardConfigSourceTests(argc, argv);
    result |= runPageHomeTests(argc, argv);

    return result;
}
