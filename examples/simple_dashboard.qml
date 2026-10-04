import QtQuick 2.15
import QtQuick.Controls 2.15

// Simple clean QML design with literal property values only
Rectangle {
    id: statusCard
    x: 40
    y: 40
    width: 320
    height: 220
    color: "#1E293B"
    radius: 8

    Text {
        id: titleLabel
        x: 60
        y: 60
        width: 280
        height: 32
        text: "Sensor Telemetry"
        color: "#38BDF8"
        font.pixelSize: 18
        font.bold: true
    }

    Button {
        id: actionButton
        x: 60
        y: 110
        width: 130
        height: 40
        text: "Calibrate"
        color: "#2563EB"
        textColor: "#FFFFFF"
        radius: 6
    }

    ProgressBar {
        id: levelGauge
        x: 60
        y: 170
        width: 260
        height: 24
        value: 0.72
    }
}
