import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import PageEnum 1.0
import UpdateState 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    readonly property bool hasChangelog: UpdateController.description !== ""
                                         || UpdateController.newFeatures.length > 0
                                         || UpdateController.improvements.length > 0
                                         || UpdateController.bugFixes.length > 0

    Connections {
        target: UpdateController

        function onUpdateStateChanged() {
            if (UpdateController.updateState === UpdateState.ReadyToInstall) {
                PageController.showNotificationMessage(qsTr("Done. Install the update"))
            } else if (UpdateController.updateState === UpdateState.DownloadError) {
                PageController.showNotificationMessage(qsTr("Download failed. Download manually from amnezia.org"))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: AmneziaStyle.color.bgBase
    }

    NavigationBarType {
        id: navigationBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        IconButtonType {
            id: supportButton

            icon.source: "qrc:/images/controls/headphones.svg"

            onClicked: supportDrawer.open()
        }
    }

    FlickableType {
        id: fl

        anchors.top: navigationBar.bottom
        anchors.bottom: bottomBar.top
        contentHeight: content.implicitHeight + 16 + 32

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 24

            ColumnLayout {
                id: headerBlock

                Layout.fillWidth: true
                spacing: 12

                Item {
                    id: imageBlock

                    Layout.fillWidth: true
                    Layout.preferredHeight: 98

                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        maskSource: imageBlockMask
                        maskThresholdMin: 0.5
                        maskSpreadAtMin: 1.0
                    }

                    Image {
                        id: heroImage

                        x: -imageBlock.width * 0.3827
                        y: -imageBlock.height * 2.4255
                        width: imageBlock.width * 1.8771
                        height: imageBlock.height * 5.8577

                        source: "qrc:/images/update/updateHeroDark.jpg"
                        fillMode: Image.Stretch
                    }

                    Rectangle {
                        id: imageBlockMask

                        anchors.fill: parent
                        radius: 16
                        visible: false
                        layer.enabled: true
                    }

                    BackdropBlurType {
                        anchors.centerIn: parent
                        width: versionText.implicitWidth + 26
                        height: 55

                        sourceItem: heroImage

                        DisplayTextType {
                            id: versionText

                            anchors.centerIn: parent

                            text: "v. " + UpdateController.version
                        }
                    }
                }

                Flow {
                    id: metaRow

                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: UpdateController.tags

                        delegate: StatusBadgeType {
                            text: modelData
                        }
                    }

                    SmallTextType {
                        height: UpdateController.tags.length > 0 ? 22 : implicitHeight

                        verticalAlignment: Text.AlignVCenter

                        text: UpdateController.releaseInfoText
                    }
                }
            }

            ColumnLayout {
                id: changelogBlock

                Layout.fillWidth: true
                spacing: 12

                H1TextType {
                    Layout.fillWidth: true

                    text: qsTr("New version available")
                }

                H3TextType {
                    Layout.fillWidth: true

                    visible: text !== ""

                    color: AmneziaStyle.color.textTertiary

                    text: root.hasChangelog ? UpdateController.description : qsTr("No release notes yet")
                }

                ChangelogSectionType {
                    Layout.fillWidth: true

                    iconSource: "qrc:/images/update/plus-circle.svg"
                    title: qsTr("New")
                    items: UpdateController.newFeatures
                }

                ChangelogSectionType {
                    Layout.fillWidth: true

                    iconSource: "qrc:/images/update/arrow-up-circle.svg"
                    title: qsTr("Improved")
                    items: UpdateController.improvements
                }

                ChangelogSectionType {
                    Layout.fillWidth: true

                    iconSource: "qrc:/images/update/wrench.svg"
                    title: qsTr("Fixed")
                    items: UpdateController.bugFixes
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        filled: true

        ButtonType {
            id: updateButton

            Layout.fillWidth: true

            loading: !UpdateController.isStoreUpdate && UpdateController.updateState === UpdateState.Downloading
            enabled: UpdateController.updateState !== UpdateState.Downloading

            leftImageSource: !UpdateController.isStoreUpdate && UpdateController.updateState === UpdateState.ReadyToInstall
                             ? "qrc:/images/update/arrow-down-circle.svg" : ""

            text: {
                if (UpdateController.isStoreUpdate) {
                    return qsTr("Update app")
                }
                switch (UpdateController.updateState) {
                case UpdateState.Downloading: return qsTr("Downloading update...")
                case UpdateState.ReadyToInstall: return qsTr("Install update")
                case UpdateState.DownloadError: return qsTr("Retry")
                default: return qsTr("Update app")
                }
            }

            onClicked: {
                if (UpdateController.isStoreUpdate) {
                    UpdateController.update()
                    return
                }

                switch (UpdateController.updateState) {
                case UpdateState.ReadyToInstall:
                    UpdateController.install()
                    break
                case UpdateState.DownloadError:
                    UpdateController.retry()
                    break
                default:
                    UpdateController.update()
                    break
                }
            }
        }
    }

    UpdateSupportDrawer {
        id: supportDrawer

        parent: root
    }
}
