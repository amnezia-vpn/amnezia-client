import QtQuick
import QtQuick.Controls

QtObject {
    id: root
    
    property var flickable: null
    property var scrollToItemTarget: null
    
    property Connections imeConnection: Connections {
        target: PageController
        function onImeHeightChanged() {
            if (root.scrollToItemTarget && PageController.imeHeight > 0) {
                scrollTimer.restart()
            }
        }
    }
    
    property Timer scrollTimer: Timer {
        interval: 100
        repeat: false
        onTriggered: {
            if (root.scrollToItemTarget && root.flickable) {
                if (PageController.imeHeight > 0) {
                    var item = root.scrollToItemTarget
                    var itemY = item.mapToItem(root.flickable.contentItem, 0, 0).y
                    var itemHeight = item.height
                    var keyboardHeight = PageController.imeHeight + PageController.safeAreaBottomMargin
                    var visibleHeight = root.flickable.height - keyboardHeight
                    
                    var desiredTopOffset = visibleHeight * 0.25
                    var targetContentY = itemY - desiredTopOffset
                    
                    if (targetContentY < 0) {
                        targetContentY = 0
                    }
                    
                    var maxContentY = root.flickable.contentHeight - root.flickable.height
                    if (targetContentY > maxContentY) {
                        targetContentY = maxContentY
                    }
                    
                    root.flickable.contentY = targetContentY
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

