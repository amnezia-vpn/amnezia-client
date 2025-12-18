import QtQuick
import QtQuick.Controls

QtObject {
    id: root
    
    property var listView: null
    property var scrollToItemTarget: null
    
    property Connections imeConnection: Connections {
        target: SettingsController
        function onImeHeightChanged() {
            if (root.scrollToItemTarget && SettingsController.imeHeight > 0) {
                scrollTimer.restart()
            }
        }
    }
    
    property Timer scrollTimer: Timer {
        interval: 100
        repeat: false
        onTriggered: {
            if (root.scrollToItemTarget && root.listView) {
                if (SettingsController.imeHeight > 0) {
                    var item = root.scrollToItemTarget
                    var itemY = item.mapToItem(root.listView.contentItem, 0, 0).y
                    var itemHeight = item.height
                    var keyboardHeight = SettingsController.imeHeight + SettingsController.safeAreaBottomMargin
                    var visibleHeight = root.listView.height - keyboardHeight
                    
                    var desiredTopOffset = visibleHeight * 0.25
                    var targetContentY = itemY - desiredTopOffset
                    
                    if (targetContentY < 0) {
                        targetContentY = 0
                    }
                    
                    var maxContentY = root.listView.contentHeight - root.listView.height
                    if (targetContentY > maxContentY) {
                        targetContentY = maxContentY
                    }
                    
                    root.listView.contentY = targetContentY
                    root.scrollToItemTarget = null
                }
            }
        }
    }
    
    function scrollToItem(item) {
        scrollToItemTarget = item
        scrollTimer.restart()
    }
}

