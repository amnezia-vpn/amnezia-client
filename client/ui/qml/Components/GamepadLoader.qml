import QtQuick
import QtGamepadLegacy

Item {
    id: root
    
    property alias gamepad: gamepad
    property alias gamepadKeyNav: gamepadKeyNav
    
    Gamepad {
        id: gamepad
        deviceId: GamepadManager.connectedGamepads.length > 0 ? GamepadManager.connectedGamepads[0] : -1

        onButtonStartChanged: {
            if (buttonStart) {
                ServersModel.setProcessedServerIndex(ServersModel.defaultIndex)
                ConnectionController.connectButtonClicked()
            }
        }
    }

    GamepadKeyNavigation {
        id: gamepadKeyNav
        gamepad: gamepad
        active: true
    }

    Connections {
        target: GamepadManager
        function onConnectedGamepadsChanged() {
            if (GamepadManager.connectedGamepads.length > 0) {
                gamepad.deviceId = GamepadManager.connectedGamepads[0]
            } else {
                gamepad.deviceId = -1
            }
        }
    }
}
