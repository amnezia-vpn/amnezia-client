import QtQuick
import QtTest 1.2

Item {
    width: 480
    height: 720

    Loader {
        id: pageLoader
        active: false
        asynchronous: false
        source: Qt.resolvedUrl("../../ui/qml/Pages2/PageServiceDnsSettings.qml")
    }

    TestCase {
        name: "PageServiceDnsSettings"
        when: windowShown

        function test_page_loads_without_errors() {
            failOnWarning(/removeButton is not defined/)
            pageLoader.active = true

            tryCompare(pageLoader, "status", Loader.Ready)
            verify(pageLoader.item !== null)
        }
    }
}
