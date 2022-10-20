import QtQuick
import QtQuick.Controls
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    property var page: PageEnum.Start
    property var logic: UiLogic

    property bool pageActive: false

    signal activated(bool reset)
    signal deactivated()

    onActivated: pageActive = true
    onDeactivated: pageActive = false

//    width: GC.screenWidth
//    height: GC.screenHeight

}
