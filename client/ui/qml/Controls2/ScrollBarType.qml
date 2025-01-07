import QtQuick
import QtQuick.Controls

import "./"
import "../Controls2"

ScrollBar {
    id: root
    
    policy: parent.height >= parent.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
}
