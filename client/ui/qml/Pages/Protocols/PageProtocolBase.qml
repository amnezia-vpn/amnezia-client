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
    page: PageEnum.ProtocolSettings
}
