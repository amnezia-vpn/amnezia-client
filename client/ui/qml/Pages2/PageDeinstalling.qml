import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Component.onCompleted: PageController.disableControls(true)
    Component.onDestruction: PageController.disableControls(false)

    SortFilterProxyModel {
        id: proxyServersModel
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    FlickableType {
        id: fl
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 16

            Repeater {
                model: proxyServersModel
                delegate: Item {
                    implicitWidth: parent.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.topMargin: 20

                            headerText: qsTr("Removing services from %1").arg(name)
                        }

                        ProgressBarType {
                            id: progressBar

                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            Timer {
                                id: timer

                                interval: 300
                                repeat: true
                                running: true
                                onTriggered: {
                                    progressBar.value += 0.003
                                }
                            }
                        }

                        ParagraphTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 8

                            text: qsTr("Usually it takes no more than 5 minutes")
                        }
                    }
                }
            }
        }
    }
}
