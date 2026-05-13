import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

// FBLink VPN — top-level TV navigation stack.
//
// `PageTvRoot` owns the StackView shared by all Android TV screens
// (login → home → servers / subscription). It listens to the FBLink
// controller and swaps the root page when the login state changes so
// users do not have to navigate manually after signing in or signing
// out. It also catches the remote control's Back / Escape key globally
// and pops the stack — that way every TV screen has consistent
// "branded" routing without having to repeat the boilerplate.
FocusScope {
    id: root

    anchors.fill: parent
    focus: true

    Component.onCompleted: {
        console.log("PageTvRoot loaded, loggedIn =", FBLinkController.isLoggedIn)
        tvStack.replace(FBLinkController.isLoggedIn ? "PageTvHome.qml" : "PageTvLogin.qml")
    }

    Connections {
        target: FBLinkController

        function onLoginSuccess() {
            tvStack.clear()
            tvStack.push("PageTvHome.qml")
        }

        function onTvLoginApproved() {
            tvStack.clear()
            tvStack.push("PageTvHome.qml")
        }

        function onLoginStateChanged() {
            if (!FBLinkController.isLoggedIn) {
                tvStack.clear()
                tvStack.push("PageTvLogin.qml")
            }
        }
    }

    StackView {
        id: tvStack
        anchors.fill: parent
        focus: true

        // Use simple slide transitions so navigation feels native on TV.
        pushEnter: Transition {
            ParallelAnimation {
                PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 180 }
                PropertyAnimation { property: "x"; from: parent ? parent.width * 0.04 : 60; to: 0; duration: 220; easing.type: Easing.OutCubic }
            }
        }
        pushExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 140 }
        }
        popEnter: Transition {
            ParallelAnimation {
                PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 180 }
                PropertyAnimation { property: "x"; from: parent ? -parent.width * 0.04 : -60; to: 0; duration: 220; easing.type: Easing.OutCubic }
            }
        }
        popExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 140 }
        }
        replaceEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        replaceExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 }
        }

        onCurrentItemChanged: {
            if (currentItem) {
                currentItem.forceActiveFocus()
            }
        }

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
