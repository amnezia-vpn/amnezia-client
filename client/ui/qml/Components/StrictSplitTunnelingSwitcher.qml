import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Controls2"

// Block disallowed apps from the tunnel (issue #2457): apps the VPN does not cover, excluded or
// of a work profile or another user, may not bind to its interface directly. Read when the tunnel
// starts, so it is locked while connected; enforced on the AmneziaWG/WireGuard datapath.
SwitcherType {
    id: root

    visible: Qt.platform.os === "android"
    enabled: ServersUiController.isDefaultServerDefaultContainerSupportsStrictSplitTunneling
             && !ConnectionController.isConnected

    text: qsTr("Block disallowed apps from the tunnel")
    descriptionText: qsTr("Keeps apps that are not allowed into the VPN out of its tunnel: apps excluded from the list, in a work profile or in another user. They cannot force their way into the tunnel and learn the server's address. Supports AmneziaWG and WireGuard.")

    checked: SettingsController.strictSplitTunnelingEnabled

    function restoreBinding() {
        root.checked = Qt.binding(function() { return SettingsController.strictSplitTunnelingEnabled })
    }

    onToggled: function() {
        if (!checked) {
            SettingsController.strictSplitTunnelingEnabled = false
            restoreBinding()
            return
        }

        var headerText = qsTr("Block disallowed apps from the tunnel?")
        var descriptionText = qsTr("Some apps detect a VPN by connecting through its interface directly, bypassing the split tunneling rules, and can learn the server's IP address this way. With this setting on, such connections are dropped. This includes apps in a work profile or another user, which can do the same without split tunneling.\n\nIn the \"only the apps from the list\" mode, and with the VPN in a work profile or another user, Private DNS set to a provider hostname stops working for the apps in the VPN. Set Private DNS to Automatic or Off.")
        var yesButtonText = qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            SettingsController.strictSplitTunnelingEnabled = true
            restoreBinding()
        }
        var noButtonFunction = function() {
            restoreBinding()
        }

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
