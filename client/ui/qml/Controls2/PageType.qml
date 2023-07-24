import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property StackView stackView: StackView.view

    function goToPage(page, slide = true) {
        var pagePath = PageController.getPagePath(page)
        if (slide) {
            root.stackView.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
        } else {
            root.stackView.push(pagePath, { "objectName" : pagePath }, StackView.Immediate)
        }
    }

    function closePage() {
        if (root.stackView.depth <= 1) {
            return
        }
        root.stackView.pop()
    }

    function goToStartPage() {
        while (root.stackView.depth > 1) {
            root.stackView.pop()
        }
    }
}
