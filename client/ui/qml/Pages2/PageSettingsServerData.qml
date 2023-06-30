import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    Connections {
        target: InstallController

        function onScanServerFinished(isInstalledContainerFound) {
            var message = ""
            if (isInstalledContainerFound) {
                message = qsTr("All installed containers have been added to the application")
            } else {
                message = qsTr("Не найдено установленных контейнеров")
            }

            PageController.showErrorMessage(message)
        }
    }

    Connections {
        target: ServersModel

        function onCurrentlyProcessedServerIndexChanged() {
            content.isServerWithWriteAccess = ServersModel.isCurrentlyProcessedServerHasWriteAccess()
        }
    }

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            property bool isServerWithWriteAccess: ServersModel.isCurrentlyProcessedServerHasWriteAccess()

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear Amnezia cache")
                descriptionText: qsTr("May be needed when changing other settings")

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Clear cached profiles?")
                    questionDrawer.descriptionText = qsTr("some description")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        ContainersModel.clearCachedProfiles()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Проверить сервер на наличие ранее установленных сервисов Amnezia")
                descriptionText: qsTr("Добавим их в приложение, если они не отображались")

                clickedFunction: function() {
                    InstallController.scanServerForInstalledContainers()
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Remove server from application")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Remove server?")
                    questionDrawer.descriptionText = qsTr("All installed AmneziaVPN services will still remain on the server.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected) {
                            ConnectionController.closeConnection()
                        }
                        ServersModel.removeServer()
                        if (!ServersModel.getServersCount()) {
                            PageController.replaceStartPage()
                        } else {
                            goToStartPage()
                            goToPage(PageEnum.PageSettingsServersList)
                        }
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {}

            LabelWithButtonType {
                visible: content.isServerWithWriteAccess
                Layout.fillWidth: true

                text: qsTr("Clear server from Amnezia software")
                textColor: "#EB5757"

                clickedFunction: function() {
                    questionDrawer.headerText = qsTr("Clear server from Amnezia software?")
                    questionDrawer.descriptionText = qsTr(" All containers will be deleted on the server. This means that configuration files, keys and certificates will be deleted.")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        goToPage(PageEnum.PageDeinstalling)
                        if (ServersModel.isDefaultServerCurrentlyProcessed && ConnectionController.isConnected) {
                            ConnectionController.closeVpnConnection()
                        }
                        ContainersModel.removeAllContainers()
                        closePage()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }

            DividerType {
                visible: content.isServerWithWriteAccess
            }

            QuestionDrawer {
                id: questionDrawer
            }
        }
    }
}
