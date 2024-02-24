import QtQuick 2.15
import QtQml 2.15

Behavior {
    id: root

    property QtObject fadeTarget: targetProperty.object
    property string fadeProperty: "opacity"
    property int fadeDuration: 200
    property int fadeValue: 0
    property string easingType: "Quad"

    property alias exitAnimation: exitAnimation
    property alias enterAnimation: enterAnimation

    SequentialAnimation {
        NumberAnimation {
            id: exitAnimation
            target: root.fadeTarget
            property: root.fadeProperty
            duration: root.fadeDuration
            to: root.fadeValue
            easing.type: root.easingType === "Linear" ? Easing.Linear : Easing["In"+root.easingType]
        }
        PropertyAction { }
        NumberAnimation {
            id: enterAnimation
            target: root.fadeTarget
            property: root.fadeProperty
            duration: root.fadeDuration
            to: target[property]
            easing.type:  root.easingType === "Linear" ? Easing.Linear : Easing["Out"+root.easingType]
        }
    }
}
