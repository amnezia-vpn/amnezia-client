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

    m_openVpnLastConfigs = m_openVpnMalStrings =
            "<style> \
            div { line-height: 0.5; } \
            </style><div>";

    m_warningStringNumber = 3;
    m_warningActive = false;

    const QJsonArray &containers = configJson()[config_key::containers].toArray();
    int i = 0;
    for (const QJsonValue &v: containers) {
        QString cfg_json = v.toObject()[ProtocolProps::protoToString(Proto::OpenVpn)]
                .toObject()[config_key::last_config].toString();

        QString openvpn_cfg = QJsonDocument::fromJson(cfg_json.toUtf8()).object()[config_key::config]
                .toString();

        openvpn_cfg.replace("\r", "");

        QStringList lines = openvpn_cfg.split("\n");
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
        uiLogic()->selectedServerIndex = m_settings->defaultServerIndex();
        uiLogic()->selectedDockerContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
        uiLogic()->onUpdateAllPages();
        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
        emit uiLogic()->goToPage(Page::ServerContainers);
    } else {
        emit uiLogic()->goToPage(Page::Vpn);
        emit uiLogic()->setStartPage(Page::Vpn);
    }
}

