import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.About

    BackButton {
        id: back_from_start
    }

    Caption {
        id: caption
        font.pixelSize: 22
        text: qsTr("About Amnezia")
    }

    RichLabelType {
        id: label_about
        anchors.top: caption.bottom

        text: qsTr("AmneziaVPN is opensource software, it's free forever. Our goal is to make the best VPN client in the world.
<ul>
<li>Sources on <a href=\"https://github.com/amnezia-vpn/desktop-client\">GitHub</a></li>
<li><a href=\"https://amnezia.org/\">Web Site</a></li>
<li><a href=\"https://t.me/amnezia_vpn_en\">Telegram group</a></li>
<li><a href=\"https://signal.group/#CjQKIB2gUf8QH_IXnOJMGQWMDjYz9cNfmRQipGWLFiIgc4MwEhAKBONrSiWHvoUFbbD0xwdh\">Signal group</a></li>
</ul>
")
    }

    Caption {
        id: caption2
        anchors.topMargin: 20
        font.pixelSize: 22
        text: qsTr("Support")
        anchors.top: label_about.bottom
    }

    RichLabelType {
        id: label_support
        anchors.top: caption2.bottom

        text: qsTr("Have questions? You can get support by:
<ul>
<li><a href=\"https://t.me/amnezia_vpn_en\">Telegram group</a> (preferred way)</li>
<li>Create issue on <a href=\"https://github.com/amnezia-vpn/desktop-client/issues\">GitHub</a></li>
<li>Email to: <a href=\"support@amnezia.org\">support@amnezia.org</a></li>
</ul>")
    }

    Caption {
        id: caption3
        anchors.topMargin: 20
        font.pixelSize: 22
        text: qsTr("Donate")
        anchors.top: label_support.bottom
    }

    RichLabelType {
        id: label_donate
        anchors.top: caption3.bottom

        text: qsTr("Please support Amnezia project by donation, we really need it now more than ever.
<ul>
<li>By credit card on <a href=\"https://www.patreon.com/amneziavpn\">Patreon</a> (starting from $1)</li>
<li>Send some coins to addresses listed <a href=\"https://github.com/amnezia-vpn/desktop-client/blob/master/README.md\">on GitHub page</a></li>
</ul>
")
    }

    Logo {
        id: logo
        anchors.bottom: parent.bottom
    }
}
