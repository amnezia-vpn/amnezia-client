import QtQuick

import ProtocolEnum 1.0

import "./"

PageServiceMtProtoSettingsBase {
    configModel: MtProxyConfigModel
    protocol: ProtocolEnum.MtProxy

    pageTitle: qsTr("MTProxy settings")
    readMoreUrl: "https://core.telegram.org/proxy"
    noInternetText: qsTr("No internet connection. Connect to the internet to change MTProxy settings.")
    startedText: qsTr("MTProxy started")
    stoppedText: qsTr("MTProxy stopped")
    deployFirstText: qsTr("Deploy MTProxy first")
    connectionLinkText: qsTr("MTProxy connection link")
    deleteText: qsTr("Delete MTProxy")
    enableText: qsTr("Enable MTProxy")
    enableToEditText: qsTr("Enable MTProxy to edit settings")
    newSecretSavedText: qsTr("New secret saved. It will be applied when MTProxy is started.")
    natSwitchText: qsTr("Server is behind NAT / Docker bridge")
    natSwitchDescriptionText: qsTr("Enable if your server is not directly accessible from the internet, e.g. Docker or private network")
    natExternalIpHeaderText: qsTr("External IP")
    natExternalIpErrorText: qsTr("NAT external IP: enter a valid IPv4 address")
}
