import QtQuick
import QtCharts

ChartView {
    id: chartView
    legend.visible: false
    animationOptions: ChartView.AllAnimations
    animationDuration: 2000.0

    backgroundColor: "#1C1D21"
    plotAreaColor: "#1C1D21"
    margins.top: 0
    margins.bottom: 0
    margins.left: 0
    margins.right: 0
    antialiasing: true


    property bool shouldUpdate: SystemController.appHasFocus

    function getUTCSeconds() {
        return new Date().setMilliseconds(0) / 1000
    }

    function addValues(rx, tx) {
        let currentTime = getUTCSeconds()

        xAxis.min = currentTime - 60
        xAxis.max = currentTime

        if (rx > yAxis.max) yAxis.max = rx * 1.1
        if (tx > yAxis.max) yAxis.max = tx * 1.1

        rxLine.append(currentTime, rx)
        txLine.append(currentTime, tx)
    }

    function printAll() {
        var rxValues = ConnectionController.getRxView()
        var txValues = ConnectionController.getTxView()
        var times = ConnectionController.getTimes()

        let currentTime = getUTCSeconds()
        xAxis.min = currentTime - 60
        xAxis.max = currentTime


        rxLine.clear()
        txLine.clear()

        if (times.length === 0) return

        xAxis.min = times[0]
        xAxis.max = times[times.length - 1]

        for (let i = 0; i < times.length; i++)
        {
            if (rxValues[i] > yAxis.max) yAxis.max = rxValues[i]
            if (txValues[i] > yAxis.max) yAxis.max = txValues[i]

            rxLine.append(times[i], rxValues[i])
            txLine.append(times[i], txValues[i])
        }
    }

    Component.onCompleted: {
        printAll()
    }

    Connections {
        target: ConnectionController
        function onBytesChanged() {
            if (shouldUpdate) {
                addValues(ConnectionController.rxBytes, ConnectionController.txBytes)
            }
        }
    }

    Connections {
        target: SystemController
        function onAppHasFocusChanged() {
            if (shouldUpdate) { printAll() }
        }
    }

    ValueAxis {
        id: yAxis
        min: -100
        max: 1000
        visible: false
        labelsVisible: false
        gridLineColor: "transparent"
    }

    ValueAxis {
        id: xAxis
        visible: false
        labelsVisible: false
        gridLineColor: "transparent"
    }

    SplineSeries {
        id: rxLine
        name: "Received Bytes"
        //width: 2
        axisX: xAxis
        axisY: yAxis
        style: Qt.RoundCap
        capStyle: Qt.RoundCap
        useOpenGL: true
        color: "#70553c"
        XYPoint { x: getUTCSeconds(); y: 0 }
    }

    SplineSeries {
        id: txLine
        name: "Transmitted Bytes"
        //width: 2
        axisX: xAxis
        axisY: yAxis
        style: Qt.RoundCap
        capStyle: Qt.RoundCap
        useOpenGL: true
        color: "#737274"
        XYPoint { x: getUTCSeconds(); y: 0 }
    }
}
