import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Item {
    id: root

    anchors.fill: parent

    Component.onCompleted: {
        console.log("PageTvRoot loaded, loggedIn =", FBLinkController.isLoggedIn)
        tvStack.replace(FBLinkController.isLoggedIn ? "PageTvHome.qml" : "PageTvLogin.qml")
    }

    Connections {
        target: FBLinkController

        function onLoginSuccess() {
            tvStack.replace("PageTvHome.qml")
        }

        function onTvLoginApproved() {
            tvStack.replace("PageTvHome.qml")
        }

        function onLoginStateChanged() {
            if (!FBLinkController.isLoggedIn) {
                tvStack.replace("PageTvLogin.qml")
            }
        }
    }

    StackView {
        id: tvStack
        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
                if (tvStack.depth > 1) {
                    tvStack.pop()
                } else {
                    PageController.hideWindow()
                }
                event.accepted = true
            }
        }
    }
}
