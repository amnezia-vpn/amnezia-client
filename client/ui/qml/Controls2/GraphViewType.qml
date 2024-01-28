import QtQuick
import QtCharts

ChartView {
    id: chartView
    legend.visible: false
    theme: ChartView.ChartThemeDark
    plotArea: Qt.rect(0, 0, 400, 50)

    property bool shouldUpdate: SystemController.hasFocus

    function getUTCSeconds() {
        return new Date().setMilliseconds(0) / 1000
    }

    function addValues(rx, tx) {
        let currentTime = getUTCSeconds()

        xAxis.min = currentTime - 60
        xAxis.max = currentTime

        if (rx > yAxis.max)
        {
            yAxis.max = rx
        }
        if (tx > yAxis.max)
        {
            yAxis.max = tx
        }

        rxLine.append(currentTime, rx)
        txLine.append(currentTime, tx)
    }

    function printAll() {
        var rxValues = ConnectionController.getRxView()
        var txValues = ConnectionController.getTxView()
        var times = ConnectionController.getTimes()

        rxLine.clear()
        txLine.clear()

        xAxis.min = times[0]
        xAxis.max = times[times.length - 1]

        for (let i = 0; i < times.length; i++)
        {
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
        function onHasFocusChanged() {
            if (shouldUpdate) { printAll() }
        }
    }

    ValueAxis {
        id: yAxis
        min: 0
        max: 1000000
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
        axisX: xAxis
        axisY: yAxis
        capStyle: Qt.RoundCap
        color: "orange"

        XYPoint { x: getUTCSeconds(); y: 0 }
    }

    SplineSeries {
        id: txLine
        name: "Transmitted Bytes"
        axisX: xAxis
        axisY: yAxis
        capStyle: Qt.RoundCap
        color: "grey"

        XYPoint { x: getUTCSeconds(); y: 0 }
    }

    onWidthChanged: {
        chartView.plotArea = Qt.rect(0, 0, width, 50)
    }
}
