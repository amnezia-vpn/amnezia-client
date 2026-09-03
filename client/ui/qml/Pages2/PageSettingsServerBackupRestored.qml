import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Components"
import "../Config"

PageType {
    id: root

    property string backupFileName: ""
    property string serverName: ""
    property string serverIp: ""
    property bool isFromSetupWizard: false
    
    Component.onCompleted: {
        // Убеждаемся, что все свойства инициализированы
        if (!backupFileName) backupFileName = ""
        if (!serverName) serverName = ""
        if (!serverIp) serverIp = ""
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        backButtonFunction: function() {
            // После успешного restore всегда идем на главную страницу
            PageController.goToPageHome()
        }

        onActiveFocusChanged: {
            if(backButton.enabled && backButton.activeFocus) {
                flickable.contentY = 0
            }
        }
    }

    FlickableType {
        id: flickable

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: contentColumn.implicitHeight

        ColumnLayout {
            id: contentColumn
            width: flickable.width

            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                
                headerText: qsTr("Backup restored")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                
                text: {
                    var baseText = qsTr("%1 on \"%2\"").arg(backupFileName).arg(serverName)
                    if (serverIp && serverIp.length > 0) {
                        return baseText + ", " + serverIp
                    }
                    return baseText
                }
                color: AmneziaStyle.color.mutedGray
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24
                spacing: 12

                BasicButtonType {
                    Layout.fillWidth: true
                    
                    text: qsTr("To home")
                    implicitHeight: 56
                    
                    clickedFunc: function() {
                        // Переход на главную страницу (PageHome)
                        PageController.goToPageHome()
                    }
                }

                BasicButtonType {
                    Layout.fillWidth: true
                    
                    text: qsTr("To server settings")
                    implicitHeight: 56
                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.transparent
                    pressedColor: AmneziaStyle.color.transparent
                    borderWidth: 1
                    borderColor: "#FFFFFF"
                    textColor: "#FFFFFF"
                    
                    clickedFunc: function() {
                        // Открываем страницу настроек сервера с активной вкладкой "Управление"
                        PageController.goToPage(PageEnum.PageSettingsServerInfo)
                        // Устанавливаем активной вкладку "Управление" через сигнал
                        PageController.goToPageSettingsServerManagement()
                    }
                }
            }
        }
    }
}
