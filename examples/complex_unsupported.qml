import QtQuick 2.15
import QtQuick.Controls 2.15

// Complex QML file containing unsupported items and dynamic bindings to be rejected
Item {
    id: rootItem
    width: 600
    height: 400

    Rectangle {
        id: bgPanel
        x: 30
        y: 30
        width: parent.width - 60
        height: 300
        color: "#0F172A"

        Text {
            id: dynamicText
            anchors.fill: parent
            text: "Status: " + device.status
            color: isOnline ? "#22C55E" : "#EF4444"
        }

        Loader {
            id: pageLoader
            source: "SubPage.qml"
        }

        Repeater {
            id: listRepeater
            model: 10
        }

        states: [
            State {
                name: "active"
                PropertyChanges { target: bgPanel; color: "#1E293B" }
            }
        ]

        transitions: [
            Transition {
                from: "*"
                to: "*"
            }
        ]

        MouseArea {
            id: touchArea
            anchors.centerIn: parent
            onClicked: {
                console.log("Touched screen");
            }
        }
    }
}
