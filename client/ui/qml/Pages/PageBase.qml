import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    property int page: PageEnum.Start
    property var logic: UiLogic
    enabled: logic.pageEnabled

//    width: GC.screenWidth
//    height: GC.screenHeight

}
