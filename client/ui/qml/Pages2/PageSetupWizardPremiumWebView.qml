import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0
import AmneziaWebView 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property string premiumUrl: LanguageModel.getCurrentLanguageIndex() === 1  // 1 = Russian
        ? "https://storage.googleapis.com/amnezia/amnezia.org?m-path=/ru/premium"
        : "https://storage.googleapis.com/amnezia/amnezia.org?m-path=/en/premium"

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        z: 1000  // Ensure BackButton is above WebView

        onActiveFocusChanged: {
            // Focus handling
        }
    }

    ColumnLayout {
        id: webViewContainer

        anchors.top: backButton.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: 16

        spacing: 0

        AmneziaWebView {
            id: webView

            Layout.fillWidth: true
            Layout.fillHeight: true

            preferredWidth: webViewContainer.width
            preferredHeight: webViewContainer.height

            url: root.premiumUrl

            onLoadFailed: {
                console.error("Failed to load Premium page:", root.premiumUrl)
            }

            onLoadFinished: {
                console.log("Premium page loaded successfully")
            }
        }
    }
}

