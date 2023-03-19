#include "ViewConfigLogic.h"
#include "core/errorstrings.h"
#include "../uilogic.h"


ViewConfigLogic::ViewConfigLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void ViewConfigLogic::onUpdatePage()
{
    set_configText(QJsonDocument(configJson()).toJson());

    auto s = configJson()[config_key::isThirdPartyConfig].toBool();

    m_openVpnLastConfigs = m_openVpnMalStrings =
            "<style> \
            div { line-height: 0.5; } \
            </style><div>";

    m_warningStringNumber = 3;
    m_warningActive = false;

    const QJsonArray &containers = configJson()[config_key::containers].toArray();
    int i = 0;
    for (const QJsonValue &v: containers) {
        auto containerName = v.toObject()[config_key::container].toString();
        QJsonObject containerConfig = v.toObject()[containerName.replace("amnezia-", "")].toObject();
        if (containerConfig[config_key::isThirdPartyConfig].toBool()) {
            auto lastConfig = containerConfig.value(config_key::last_config).toString();
            auto lastConfigJson = QJsonDocument::fromJson(lastConfig.toUtf8()).object();
            QStringList lines = lastConfigJson.value(config_key::config).toString().replace("\r", "").split("\n");
            QString lastConfigText;
            for (const QString &l: lines) {
                    lastConfigText.append(l + "\n");
            }
            set_configText(lastConfigText);
        }


        if (v.toObject()[config_key::container].toString() == "amnezia-openvpn") {
            QString lastConfig = v.toObject()[ProtocolProps::protoToString(Proto::OpenVpn)]
                    .toObject()[config_key::last_config].toString();

            QString lastConfigJson = QJsonDocument::fromJson(lastConfig.toUtf8()).object()[config_key::config]
                    .toString();

            QStringList lines = lastConfigJson.replace("\r", "").split("\n");
            for (const QString &l: lines) {
                i++;
                QRegularExpressionMatch match = m_re.match(l);
                if (dangerousTags.contains(match.captured(0))) {
                    QString t = QString("<p><font color=\"red\">%1</font>").arg(l);
                    m_openVpnLastConfigs.append(t + "\n");
                    m_openVpnMalStrings.append(t);
                    if (m_warningStringNumber == 3) m_warningStringNumber = i - 3;
                    m_warningActive = true;
                    qDebug() << "ViewConfigLogic : malicious scripts warning:" << l;
                }
                else {
                    m_openVpnLastConfigs.append("<p>" + l + "&nbsp;\n");
                }
            }
        }
    }

    emit openVpnLastConfigsChanged(m_openVpnLastConfigs);
    emit openVpnMalStringsChanged(m_openVpnMalStrings);
    emit warningStringNumberChanged(m_warningStringNumber);
    emit warningActiveChanged(m_warningActive);
}

void ViewConfigLogic::importConfig()
{
    m_settings->addServer(configJson());
    m_settings->setDefaultServer(m_settings->serversCount() - 1);


    if (!configJson().contains(config_key::containers) || configJson().value(config_key::containers).toArray().isEmpty()) {
        uiLogic()->m_selectedServerIndex = m_settings->defaultServerIndex();
        uiLogic()->m_selectedDockerContainer = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
        uiLogic()->onUpdateAllPages();
        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
        emit uiLogic()->goToPage(Page::ServerContainers);
    } else {
        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
    }
}

