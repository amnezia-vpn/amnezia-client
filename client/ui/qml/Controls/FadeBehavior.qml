import QtQuick
import QtQml

Behavior {
    id: root

    property QtObject fadeTarget: targetProperty.object
    property string fadeProperty: "scale"
    property int fadeDuration: 150
    property string easingType: "Quad"

    property alias outAnimation: outAnimation
    property alias inAnimation: inAnimation

    SequentialAnimation {
        NumberAnimation {
            id: outAnimation
            target: root.fadeTarget
            property: root.fadeProperty
            duration: root.fadeDuration
            to: 0
            easing.type: Easing["In"+root.easingType]
        }
        PropertyAction { }
        NumberAnimation {
            id: inAnimation
            target: root.fadeTarget
            property: root.fadeProperty
            duration: root.fadeDuration
            to: target[property]
            easing.type: Easing["Out"+root.easingType]
        }
    }

}
