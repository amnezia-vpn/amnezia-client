pragma Singleton

import QtQuick

QtObject {
    signal questionRequested(string headerText, string descriptionText, string yesButtonText, string noButtonText,
                             var yesButtonFunction, var noButtonFunction)

    function askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction) {
        questionRequested(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
