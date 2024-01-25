import QtQuick
import QtCharts

ChartView {
    id: chartView
    legend.visible: false
    theme: ChartView.ChartThemeDark
    plotArea: Qt.rect(0, 0, 400, 50)

    function addValues(rx, tx)
    {
        let currentTime = new Date().getTime()
        rxLine.append(currentTime, rx)
        txLine.append(currentTime, tx)
        if (rx > yAxis.max)
        {
            yAxis.max = rx
        }
        if (tx > yAxis.max)
        {
            yAxis.max = tx
        }
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            let currentTime = new Date().getTime()
            maxAnimation.to = currentTime
            maxAnimation.running = true
            minAnimation.to = currentTime - 60000
            minAnimation.running = true
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

    PropertyAnimation {
        id: maxAnimation
        target: xAxis
        properties: "max"
        duration: 1500
    }

    PropertyAnimation {
        id: minAnimation
        target: xAxis
        properties: "min"
        duration: 1500
    }

    SplineSeries {
        id: rxLine
        name: "Received Bytes"
        axisX: xAxis
        axisY: yAxis
        capStyle: Qt.RoundCap
        color: "orange"

        XYPoint { x: new Date().getTime(); y: 0 }
    }

    SplineSeries {
        id: txLine
        name: "Transmitted Bytes"
        axisX: xAxis
        axisY: yAxis
        capStyle: Qt.RoundCap
        color: "grey"

        XYPoint { x: new Date().getTime(); y: 0 }
    }

    onWidthChanged: {
        chartView.plotArea = Qt.rect(0, 0, width, 50)
    }
}
