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

    property real progressBarValue: 0

    Connections {
        target: InstallController

        function onInstallationErrorOccurred(errorMessage) {
            closePage()
            PageController.showErrorMessage(errorMessage)
        }

        function onInstallContainerFinished() {
            goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState(true)
            } else if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSettings)) {
                goToPage(PageEnum.PageSettingsServersList, false)
                goToPage(PageEnum.PageSettingsServerInfo, false)
            } else {
                goToPage(PageEnum.PageHome)
            }
        }

        function onInstallServerFinished() {
            goToStartPage()
            if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageHome)) {
                PageController.restorePageHomeState()
            } else if (stackView.currentItem.objectName === PageController.getPagePath(PageEnum.PageSettings)) {
                goToPage(PageEnum.PageSettingsServersList, false)
            } else {
                var pagePath = PageController.getPagePath(PageEnum.PageStart)
                stackView.replace(pagePath, { "objectName" : pagePath })
            }
        }
    }

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
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

            ListView {
                // todo change id naming
                id: container
                width: parent.width
                height: container.contentItem.height
                currentIndex: -1
                clip: true
                interactive: false
                model: proxyContainersModel

                delegate: Item {
                    implicitWidth: container.width
                    implicitHeight: delegateContent.implicitHeight

                    ColumnLayout {
                        id: delegateContent

                        anchors.fill: parent
                        anchors.rightMargin: 16
                        anchors.leftMargin: 16

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.topMargin: 20

                            headerText: "Установка"
                            descriptionText: name
                        }

                        ProgressBarType {
                            id: progressBar

                            Layout.fillWidth: true
                            Layout.topMargin: 32

//                            value: progressBarValue

                            Timer {
                                id: timer

                                interval: 300
                                repeat: true
                                running: true
                                onTriggered: {
                                    progressBar.value += 0.001
                                }
                            }
                        }

                        ParagraphTextType {
                            Layout.fillWidth: true
                            Layout.topMargin: 8

                            text: "Обычно это занимает не больше 5 минут"
                        }
                    }
                }
            }
        }
    }
}
