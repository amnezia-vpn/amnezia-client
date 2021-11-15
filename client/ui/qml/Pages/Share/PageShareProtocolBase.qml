import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import ProtocolEnum 1.0
import "./.."
import "../../Controls"
import "../../Config"

PageBase {
    id: root
    property var protocol: ProtocolEnum.Any
    page: PageEnum.ProtocolShare
    logic: ShareConnectionLogic

    readonly property string generateConfigText: qsTr("Generate config")
    readonly property string generatingConfigText: qsTr("Generating config...")
    readonly property string showConfigText: qsTr("Show config")
    property bool genConfigProcess: false
}
