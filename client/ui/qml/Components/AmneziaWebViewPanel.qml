import QtQuick 2.2
import AmneziaWebView 1.0
import Style 1.0

Rectangle {
    
    id: panel

    property alias title: webView.title
    property alias icon: webView.icon
    property alias progress: webView.progress
    property alias html: webView.html
    property alias url: webView.url
    property alias back: webView.back
    property alias stop: webView.stop
    property alias reload: webView.reload
    property alias forward: webView.forward
    property alias pressGrabTime: webView.pressGrabTime
    
    property string onFailedUrl: ""
    property string onFailedHtml: "<h2>" + qsTr("Can`t load page") + "</h2>"
    property int preferredWidth: panel.width
    property int preferredHeight: panel.height

    property var scriptResult: undefined
    property var requestResult: undefined
    
    property var call_provider: undefined
    property var call_data: undefined

    property string status: "unknown"  // success, failed
    
    signal urlCalled(variant url)
    signal alertCalled(variant message)
    signal scriptCalled(string funName, variant args)
    signal reloadCalled()
    signal eventSended(variant event)
    signal requestSended(variant request)
    signal loadFinished()
    signal loadFailed()

    color: AmneziaStyle.color.onyxBlack
    
    function call(funName, args) {
        var script
        if (panel.call_provider) {
            panel.call_data = args
            script = "call(" + funName + ")"
        } else {
            script = funName + "(" + JSON.stringify(args) + ")"
        }

        webView.evaluateJavaScript(script)
    }
    
    function event(e) {
        call("receiveEvent", e)
    }
    
    function request(r) {
        var req = r
        req.result = call("receiveRequest", req)
        return req
    }
    
    function newEvent(type) {
        var e = {}
        e.type = type
        return e
    }
    
    function newRequest(type) {
        var r = {}
        r.type = type
        r.result = null
        return r
    }

    function toogleScale() {
        webView.doToogleScale()
    }

    function zoomIn() {
        webView.doZoomIn()
    }

    function zoomOut() {
        webView.doZoomOut()
    }


    AmneziaWebView {
        id: webView
        anchors.fill: parent

        property bool loaded: false
        focus: true
        smooth: false
        backgroundColor: AmneziaStyle.color.onyxBlack

        property int panelPreferredWidth: panel.preferredWidth
        onPanelPreferredWidthChanged: {
            webView.preferredWidth = panel.preferredWidth;
            //    if (webView.loaded)
            //        doZoomOrScale();
        }

        preferredWidth: panel.preferredWidth   //webView.flexible ? panel.preferredWidth : Math.max(webView.contentsSize.width, 1024)
        preferredHeight: panel.preferredHeight

        contentsScale: 1
        // onDoubleClick: {
        //     if (webView.flexible)
        //         return

        //     async.call(doToogleScale)
        // }

        function doToogleScale() {
            //    webView.needScale = !webView.needScale
            //    if (webView.needScale) {
            //        doScale()
            //    } else {
            //        webView.contentsScale = webView.zoomvalue
            //    }
        }

        function doZoomOrScale() {
            //    if (webView.needScale)
            //        doScale()
            //    else
            //       webView.contentsScale = webView.zoomvalue;
        }

        function doZoomIn() {
            //    if (webView.zoomvalue > 0.3) {
            //        webView.zoomvalue = webView.contentsScale
            //        webView.zoomvalue -= 0.1
            //        webView.contentsScale = webView.zoomvalue
            //        webView.needScale = false
            //    }
        }

        function doZoomOut() {
            //    if (webView.zoomvalue < 2.5) {
            //        webView.zoomvalue = webView.contentsScale
            //        webView.zoomvalue += 0.1
            //        webView.contentsScale = webView.zoomvalue
            //        webView.needScale = false
            //    }
        }

        function doScale() {
            //    var zoom = flickableItem.width / webView.preferredWidth
            //    webView.contentsScale = zoom;
        }

        pressGrabTime: 400
        settings.defaultFontSize: 14
        settings.standardFontFamily: "Arial"
        //settings.developerExtrasEnabled: isDebugEnabled
        onAlert: panel.alertCalled(message)
        onUrlChanged: {
            //flickableItem.contentX = 0
            //flickableItem.contentY = 0
            panel.urlCalled(url)
        }
        onLoadStarted: {
            webView.loaded = false
            //webView.contentsScale = 1
        }
        onLoadFinished: {
            panel.status = "success"
            webView.loaded = true
            panel.loadFinished()
            //async.call(doZoomOrScale)
        }
        onLoadFailed: {
            console.debug("qml:  html load failed: " + html)
            if(html == ""){
                if (panel.onFailedUrl !== "")
                    panel.url = panel.onFailedUrl;
                else if (panel.onFailedHtml !== "")
                    panel.html = panel.onFailedHtml;
            }

            panel.status = "failed"
            panel.loadFailed()
        }
        javaScriptWindowObjects: [
            QtObject  {
                AmneziaWebView.windowObjectName: "script"
                function call(functionName, args) {
                    panel.scriptCalled(functionName, args)
                    return scriptResult;
                }
            },

            QtObject  {
                AmneziaWebView.windowObjectName: "send"
                function event(e) {
                    panel.eventSended(e)
                }
                function request(r) {
                    var req = r
                    panel.requestSended(req)
                    req.result = requestResult
                    return req;
                }
            },
            QtObject  {
                AmneziaWebView.windowObjectName: "log"

                function error(msg){
                    webView.evaluateJavaScript("console.error('" + msg + "')")
                    return api.log.error(msg)
                }
                function warn(msg){
                    webView.evaluateJavaScript("console.warn('" + msg + "')")
                    return api.log.warn(msg)
                }

                function info(msg){
                    webView.evaluateJavaScript("console.info('" + msg + "')")
                    return api.log.info(msg)
                }

                function debug(msg){
                    webView.evaluateJavaScript("console.log('" + msg + "')")
                    return api.log.debug(msg)
                }

                function time(tag, msg){
                    webView.evaluateJavaScript("console.log('" + msg + "')")
                    return api.log.time(tag, msg)
                }
            },

            QtObject  {
                AmneziaWebView.windowObjectName: "webView"

                function scrollUp() {
                    //flickableItem.contentY = 0;
                }

                function reload() {
                    //panel.reloadCalled()
                }
            },

            QtObject  {
                AmneziaWebView.windowObjectName: "call_provider"
                function data(fn) {
                    return panel.call_provider ? panel.call_provider.data(fn, panel.call_data) : panel.call_data
                }
            }
        ]
    }

}

