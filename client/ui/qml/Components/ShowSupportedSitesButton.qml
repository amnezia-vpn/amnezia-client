import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

Item {
    id: root

    property var drawerParent
    property var sitesList

    onSitesListChanged: function() {
        if (sitesList) {
            for (var i = 0; i < sitesList.length; i++) {
                sitesModel.append({"name": sitesList[i]})
            }
        }
    }

    ListModel {
       id: sitesModel
    }

    implicitWidth: showDetailsButton.implicitWidth
    implicitHeight: showDetailsButton.implicitHeight

    BasicButtonType {
        id: showDetailsButton

        anchors.left: parent.left
        anchors.topMargin: 16
        anchors.leftMargin: 8

        implicitHeight: 32

        defaultColor: AmneziaStyle.color.transparent
        hoveredColor: AmneziaStyle.color.translucentWhite
        pressedColor: AmneziaStyle.color.sheerWhite
        disabledColor: AmneziaStyle.color.mutedGray
        textColor: AmneziaStyle.color.goldenApricot

        text: qsTr("List of supported sites")

        clickedFunc: function() {
            supportedSitesDrawer.open()
        }
    }

    DrawerType2 {
        id: supportedSitesDrawer
        parent: root.drawerParent
//        onClosed: {
//            if (!GC.isMobile()) {
//                defaultActiveFocusItem.forceActiveFocus()
//            }
//        }

        anchors.fill: parent
        expandedHeight: parent.height * 0.9
        expandedContent: Item {
            Connections {
                target: supportedSitesDrawer
                enabled: !GC.isMobile()
                function onOpened() {
                    focusItem2.forceActiveFocus()
                }
            }

            implicitHeight: supportedSitesDrawer.expandedHeight

            Item {
                id: focusItem2
                KeyNavigation.tab: showDetailsBackButton
//                onFocusChanged: {
//                    if (focusItem2.activeFocus) {
//                        fl.contentY = 0
//                    }
//                }
            }

            BackButtonType {
                id: showDetailsBackButton

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16

//                KeyNavigation.tab: showDetailsCloseButton

                backButtonFunction: function() {
                    supportedSitesDrawer.close()
                }
            }

            Header2Type {
                id: supportedSitesDrawerHeader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: showDetailsBackButton.bottom
                anchors.topMargin: 16
                anchors.leftMargin: 16

                headerText: qsTr("List of supported sites")
            }

            ListView {
                id: sitesListView
                model: sitesModel

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: supportedSitesDrawerHeader.bottom
                anchors.bottom: parent.bottom
                anchors.topMargin: 16

                clip: true
                interactive: true

                ScrollBar.vertical: ScrollBar {
                    id: scrollBar
                    policy: ScrollBar.AlwaysOn
                }

                delegate: Item {
                    width: sitesListView.width
                    height: siteName.implicitHeight

                    ParagraphTextType {
                        id: siteName
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        text: name
                    }
                }
            }
        }
    }
}
