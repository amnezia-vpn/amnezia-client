import QtQuick
import QtQuick.Controls

StackView {
    id: stackView

    function gotoPage(page, slide) {
        if (slide) {
            stackView.push(PageController.getPagePath(page), {}, StackView.PushTransition)
        } else {
            stackView.push(PageController.getPagePath(page), {}, StackView.Immediate)
        }
    }

    function closePage() {
        if (stackView.depth <= 1) {
            return
        }

        stackView.pop()
    }

    Connections {
        target: PageController
        function onGoToPage(page, slide) {
            stackView.gotoPage(page, slide)
        }

        function onClosePage() {
            stackView.closePage()
        }
    }

    Component.onCompleted: {
        PageController.setStartPage()
    }
}
