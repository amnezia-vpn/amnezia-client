import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PageEnum 1.0
import ProtocolEnum 1.0
import "../"
import "../../Controls"
import "../../Config"

PageBase {
    id: root
    property var protocol: ProtocolEnum.Any
    page: PageEnum.ClientInfo
    logic: ClientInfoLogic
}
