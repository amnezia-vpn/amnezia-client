#include "WireGuardLogic.h"
#include "core/servercontroller.h"
#include <functional>
#include "../../uilogic.h"

using namespace amnezia;
using namespace PageEnumNS;

WireGuardLogic::WireGuardLogic(UiLogic *logic, QObject *parent):
    PageProtocolLogicBase(logic, parent)
{

}

void WireGuardLogic::updateProtocolPage(const QJsonObject &wireGuardConfig, DockerContainer container, bool haveAuthData)
{
    qDebug() << "WireGuardLogic::updateProtocolPage";

    auto lastConfigJsonDoc = QJsonDocument::fromJson(wireGuardConfig.value(config_key::last_config).toString().toUtf8());
    auto lastConfigJson = lastConfigJsonDoc.object();

    QString wireGuardLastConfigText;
    QStringList lines = lastConfigJson.value(config_key::config).toString().replace("\r", "").split("\n");
    for (const QString &l: lines) {
            wireGuardLastConfigText.append(l + "\n");
    }

    set_wireGuardLastConfigText(wireGuardLastConfigText);
    set_isThirdPartyConfig(wireGuardConfig.value(config_key::isThirdPartyConfig).toBool());
}
