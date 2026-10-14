pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

SheetType {
    id: root

    property string headerText
    property string descriptionText
    property string yesButtonText
    property string noButtonText

    property var yesButtonFunction
    property var noButtonFunction

    expandedHeight: content.implicitHeight + 8 + PageController.safeAreaBottomMargin

    ColumnLayout {
        id: content

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 12

        SectionHeaderType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.bottomMargin: 14
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            headerText: root.headerText
            descriptionText: root.descriptionText
        }

        ButtonType {
            id: yesButton
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            text: root.yesButtonText

            onClicked: {
                if (root.yesButtonFunction && typeof root.yesButtonFunction === "function") {
                    root.yesButtonFunction()
                }
            }
        }

        ButtonType {
            id: noButton
            variant: ButtonType.Secondary
            Layout.fillWidth: true
            Layout.rightMargin: 16
            Layout.leftMargin: 16

            visible: root.noButtonText !== ""

            text: root.noButtonText

            onClicked: {
                if (root.noButtonFunction && typeof root.noButtonFunction === "function") {
                    root.noButtonFunction()
                }
            }
        }
    }
}
