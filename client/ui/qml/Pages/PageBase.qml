import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    property var page: PageEnum.Start
    property var logic: UiLogic

    signal activated(bool reset)

//    width: GC.screenWidth
//    height: GC.screenHeight

}
