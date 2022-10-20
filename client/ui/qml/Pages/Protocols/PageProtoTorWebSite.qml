import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageProtocolBase {
    id: root
    protocol: ProtocolEnum.TorWebSite
    logic: UiLogic.protocolLogic(protocol)

    BackButton {
        id: back
    }

    Caption {
        id: caption
        text: qsTr("Tor Web Site settings")
    }


    ColumnLayout {
        id: content
        enabled: logic.pageEnabled
        anchors.top: caption.bottom
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: pb_save.top
        anchors.margins: 20
        anchors.topMargin: 10



        RowLayout {
            Layout.fillWidth: true

            LabelType {
                id: lbl_onion
                Layout.preferredWidth: 0.3 * root.width - 10
                text: qsTr("Web site onion address")
            }
            TextFieldType {
                id: tf_site_address
                Layout.fillWidth: true
                text: logic.labelTorWebSiteAddressText
                readOnly: true
            }
        }

        ShareConnectionButtonCopyType {
            Layout.fillWidth: true
            Layout.topMargin: 5
            copyText: tf_site_address.text
        }

        RichLabelType {
            Layout.fillWidth: true
            Layout.topMargin: 15
            text: qsTr("Notes:<ul>
<li>Use <a href=\"https://www.torproject.org/download/\">Tor Browser</a> to open this url.</li>
<li>After installation it takes several minutes while your onion site will become available in the Tor Network.</li>
<li>When configuring WordPress set the domain as this onion address.</li>
</ul>
")
        }
    }

}
