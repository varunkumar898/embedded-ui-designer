import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file Main.qml
 * @brief Primary MVVM View Component for Embedded UI Designer
 * 
 * Demonstrates bi-directional reactive data-binding between the QML declarative UI
 * and the C++ DocumentModel, WidgetModel, and DeviceManager instances.
 */
ApplicationWindow {
    id: appWindow
    visible: true
    width: 1400
    height: 900
    title: qsTr("Embedded UI Designer (MVVM) - %1%2")
           .arg(documentModel.projectName)
           .arg(documentModel.canUndo ? " *" : "")

    color: "#181A1F"

    // -------------------------------------------------------------------------
    // Top Navigation & Action Toolbar
    // -------------------------------------------------------------------------
    header: ToolBar {
        background: Rectangle { color: "#21252B"; border.color: "#2C313A"; height: 48 }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            // Undo / Redo controls bound directly to C++ QUndoStack via DocumentModel
            Button {
                text: "↩ Undo"
                enabled: documentModel.canUndo
                onClicked: documentModel.undo()
                ToolTip.visible: hovered
                ToolTip.text: documentModel.undoText
            }

            Button {
                text: "↪ Redo"
                enabled: documentModel.canRedo
                onClicked: documentModel.redo()
                ToolTip.visible: hovered
                ToolTip.text: documentModel.redoText
            }

            ToolSeparator {}

            // Resolution Presets bound to DocumentModel
            Label { text: "Display:"; color: "#9DA5B4"; font.bold: true }
            ComboBox {
                id: resCombo
                model: ["320 × 240 (QVGA)", "480 × 320 (HVGA)", "800 × 480 (WVGA)", "240 × 240 (Square)"]
                currentIndex: 0
                onActivated: (index) => {
                    if (index === 0) { documentModel.screenWidth = 320; documentModel.screenHeight = 240; }
                    else if (index === 1) { documentModel.screenWidth = 480; documentModel.screenHeight = 320; }
                    else if (index === 2) { documentModel.screenWidth = 800; documentModel.screenHeight = 480; }
                    else if (index === 3) { documentModel.screenWidth = 240; documentModel.screenHeight = 240; }
                }
            }

            ToolSeparator {}

            // Screen Switcher bound to DocumentModel
            Label { text: "Screen:"; color: "#9DA5B4"; font.bold: true }
            ComboBox {
                id: screenCombo
                model: documentModel.screensList
                textRole: "name"
                currentIndex: documentModel.activeScreenIndex
                Layout.preferredWidth: 140
                onActivated: (index) => {
                    documentModel.activeScreenIndex = index;
                }
            }

            Button {
                text: "+"
                onClicked: documentModel.addScreen()
                ToolTip.visible: hovered
                ToolTip.text: "Add New Screen"
            }

            ToolSeparator {}

            // Hardware Bridge: Serial port detection bound to DeviceManager
            Label { text: "MCU Port:"; color: "#9DA5B4"; font.bold: true }
            ComboBox {
                id: portCombo
                model: deviceManager.portNames
                Layout.preferredWidth: 160
                onActivated: (index) => {
                    deviceManager.selectedPort = deviceManager.portNames[index]
                }
            }

            Button {
                text: "↻"
                onClicked: deviceManager.refreshPorts()
                ToolTip.visible: hovered
                ToolTip.text: "Refresh Connected Hardware Ports"
            }

            Item { Layout.fillWidth: true } // Spacer

            Button {
                text: "Export µGFX"
                highlighted: true
                onClicked: {
                    var success = documentModel.exportUgfx("output_ugfx");
                    statusToast.show(success ? "µGFX project exported to output_ugfx/" : "Export failed: " + documentModel.lastExportError());
                }
            }

            Button {
                text: "Export QUL"
                onClicked: {
                    var success = documentModel.exportQul("output_qul");
                    statusToast.show(success ? "Qt for MCUs project exported to output_qul/" : "Export failed: " + documentModel.lastExportError());
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Main Workspace Layout
    // -------------------------------------------------------------------------
    property bool flashConsoleExpanded: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---------------------------------------------------------------------
        // Upper Editor Section (3-Pane Layout)
        // ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

        // 1. LEFT TOOLBOX PALETTE
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: "#21252B"
            border.color: "#2C313A"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Label {
                    text: "COMPONENT PALETTE"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#5C6370"
                }

                // Add component buttons that dispatch Undoable Commands to DocumentModel
                Button {
                    Layout.fillWidth: true
                    text: "+ Push Button"
                    onClicked: documentModel.addWidgetWithUndo("Button", 20, 20, 120, 40, "#2196F3", "Click Me")
                }

                Button {
                    Layout.fillWidth: true
                    text: "+ Text Label"
                    onClicked: documentModel.addWidgetWithUndo("Label", 20, 80, 140, 30, "#FFFFFF", "Status: Ready")
                }

                Button {
                    Layout.fillWidth: true
                    text: "+ Rectangle Panel"
                    onClicked: documentModel.addWidgetWithUndo("Rectangle", 20, 130, 160, 80, "#2C313A", "")
                }

                Button {
                    Layout.fillWidth: true
                    text: "+ Progress Bar"
                    onClicked: documentModel.addWidgetWithUndo("ProgressBar", 20, 230, 160, 20, "#4CAF50", "")
                }

                Button {
                    Layout.fillWidth: true
                    text: "+ Image / Icon"
                    onClicked: documentModel.addWidgetWithUndo("Image", 20, 260, 100, 80, "#61AFEF", "")
                }

                Item { Layout.fillHeight: true }

                // Hardware Status Box
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    color: "#1E2227"
                    radius: 4
                    border.color: "#2C313A"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        Label { text: "DETECTED HARDWARE"; font.pixelSize: 10; color: "#5C6370"; font.bold: true }
                        Label {
                            text: deviceManager.portCount > 0 ? deviceManager.detectBoardType(deviceManager.selectedPort) : "No board connected"
                            color: deviceManager.portCount > 0 ? "#98C379" : "#E06C75"
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: deviceManager.selectedPort ? ("Port: " + deviceManager.selectedPort) : "Plug USB cable..."
                            color: "#ABB2BF"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        // 2. CENTER CANVAS (Embedded Target Screen Surface)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#181A1F"
            clip: true

            Flickable {
                id: flickableCanvas
                anchors.fill: parent
                contentWidth: Math.max(width, targetScreen.width + 160)
                contentHeight: Math.max(height, targetScreen.height + 160)

                // Simulated embedded microcontroller screen surface
                Rectangle {
                    id: targetScreen
                    width: documentModel.screenWidth
                    height: documentModel.screenHeight
                    anchors.centerIn: parent
                    color: "#0F1014"
                    border.color: "#61AFEF"
                    border.width: 1

                    // Subtle background grid
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.strokeStyle = "#1A1D23";
                            ctx.lineWidth = 1;
                            for (var x = 0; x <= width; x += 10) {
                                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke();
                            }
                            for (var y = 0; y <= height; y += 10) {
                                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke();
                            }
                        }
                    }

                    // ---------------------------------------------------------
                    // REPEATER: Bi-directional binding to DocumentModel (DOM)
                    // Each row in DocumentModel is instantiated as an interactive item
                    // ---------------------------------------------------------
                    Repeater {
                        model: documentModel

                        delegate: Rectangle {
                            id: widgetDelegate
                            x: model.widgetX
                            y: model.widgetY
                            width: model.widgetWidth
                            height: model.widgetHeight
                            color: model.widgetColor
                            radius: model.widgetType === "Button" ? 4 : 0

                            border.color: documentModel.selectedIndex === index ? "#61AFEF" : "transparent"
                            border.width: 2

                            Text {
                                anchors.centerIn: parent
                                text: model.widgetText
                                color: model.widgetType === "Label" ? model.widgetColor : "#FFFFFF"
                                font.bold: true
                                font.pixelSize: 13
                                visible: model.widgetText !== ""
                            }

                            // Interactive Drag & Move Handler with Undo Command
                            MouseArea {
                                anchors.fill: parent
                                drag.target: widgetDelegate
                                drag.axis: Drag.XAndYAxis
                                drag.minimumX: 0
                                drag.maximumX: targetScreen.width - widgetDelegate.width
                                drag.minimumY: 0
                                drag.maximumY: targetScreen.height - widgetDelegate.height

                                property real startX: 0
                                property real startY: 0

                                onPressed: {
                                    documentModel.selectedIndex = index;
                                    startX = widgetDelegate.x;
                                    startY = widgetDelegate.y;
                                }

                                onReleased: {
                                    // Snap to 10px grid
                                    var snappedX = Math.round(widgetDelegate.x / 10) * 10;
                                    var snappedY = Math.round(widgetDelegate.y / 10) * 10;
                                    widgetDelegate.x = snappedX;
                                    widgetDelegate.y = snappedY;

                                    if (snappedX !== startX || snappedY !== startY) {
                                        // Push MoveWidgetCommand onto C++ QUndoStack
                                        documentModel.moveWidgetWithUndo(index, snappedX, snappedY);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. RIGHT PROPERTIES PANEL (MVVM Inspector)
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: "#21252B"
            border.color: "#2C313A"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Label {
                    text: "PROPERTIES INSPECTOR"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#5C6370"
                }

                // Empty state when nothing is selected
                Item {
                    visible: documentModel.selectedWidget === null
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Label {
                        anchors.centerIn: parent
                        text: "Select a widget on the canvas\nto inspect its properties."
                        color: "#5C6370"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Property fields bound to DocumentModel::selectedWidget
                ColumnLayout {
                    visible: documentModel.selectedWidget !== null
                    Layout.fillWidth: true
                    spacing: 8

                    Label { text: "Widget ID:"; color: "#ABB2BF"; font.pixelSize: 12 }
                    TextField {
                        Layout.fillWidth: true
                        text: documentModel.selectedWidget ? documentModel.selectedWidget.id : ""
                        onTextChanged: {
                            if (documentModel.selectedWidget && text !== "" && text !== documentModel.selectedWidget.id) {
                                documentModel.selectedWidget.id = text;
                            }
                        }
                    }

                    Label { text: "Text Content:"; color: "#ABB2BF"; font.pixelSize: 12 }
                    TextField {
                        Layout.fillWidth: true
                        text: documentModel.selectedWidget ? documentModel.selectedWidget.text : ""
                        onTextChanged: {
                            if (documentModel.selectedWidget && text !== documentModel.selectedWidget.text) {
                                documentModel.selectedWidget.text = text;
                            }
                        }
                    }

                    Label { 
                        text: "Target Screen Navigation:"; 
                        color: "#ABB2BF"; 
                        font.pixelSize: 12; 
                        visible: documentModel.selectedWidget ? (documentModel.selectedWidget.type === "Button") : false 
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Target Screen ID (e.g. Screen_2)"
                        visible: documentModel.selectedWidget ? (documentModel.selectedWidget.type === "Button") : false
                        text: documentModel.selectedWidget ? documentModel.selectedWidget.targetScreenId : ""
                        onTextChanged: {
                            if (documentModel.selectedWidget && text !== documentModel.selectedWidget.targetScreenId) {
                                documentModel.selectedWidget.targetScreenId = text;
                            }
                        }
                    }

                    Label { 
                        text: "Image Asset Path:"; 
                        color: "#ABB2BF"; 
                        font.pixelSize: 12; 
                        visible: documentModel.selectedWidget ? (documentModel.selectedWidget.type === "Image") : false 
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "path/to/asset.png"
                        visible: documentModel.selectedWidget ? (documentModel.selectedWidget.type === "Image") : false
                        text: documentModel.selectedWidget ? documentModel.selectedWidget.imagePath : ""
                        onTextChanged: {
                            if (documentModel.selectedWidget && text !== documentModel.selectedWidget.imagePath) {
                                documentModel.selectedWidget.imagePath = text;
                            }
                        }
                    }

                    GridLayout {
                        columns: 2
                        Layout.fillWidth: true
                        Label { text: "X:"; color: "#ABB2BF" }
                        SpinBox {
                            from: 0; to: 2000
                            value: documentModel.selectedWidget ? documentModel.selectedWidget.x : 0
                            onValueModified: if (documentModel.selectedWidget) documentModel.selectedWidget.x = value
                        }

                        Label { text: "Y:"; color: "#ABB2BF" }
                        SpinBox {
                            from: 0; to: 2000
                            value: documentModel.selectedWidget ? documentModel.selectedWidget.y : 0
                            onValueModified: if (documentModel.selectedWidget) documentModel.selectedWidget.y = value
                        }

                        Label { text: "Width:"; color: "#ABB2BF" }
                        SpinBox {
                            from: 10; to: 2000
                            value: documentModel.selectedWidget ? documentModel.selectedWidget.width : 0
                            onValueModified: if (documentModel.selectedWidget) documentModel.selectedWidget.width = value
                        }

                        Label { text: "Height:"; color: "#ABB2BF" }
                        SpinBox {
                            from: 10; to: 2000
                            value: documentModel.selectedWidget ? documentModel.selectedWidget.height : 0
                            onValueModified: if (documentModel.selectedWidget) documentModel.selectedWidget.height = value
                        }
                    }

                    Button {
                        text: "Delete Widget"
                        Layout.fillWidth: true
                        onClicked: {
                            if (documentModel.selectedIndex >= 0) {
                                documentModel.removeWidgetWithUndo(documentModel.selectedIndex);
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

        // ---------------------------------------------------------------------
        // Bottom Hardware Flashing Subsystem Dock (Prompt 3)
        // ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: flashConsoleExpanded ? 240 : 38
            color: "#181A1F"
            border.color: "#2C313A"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header Bar for Flashing Subsystem
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: "#21252B"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10

                        Label {
                            text: "⚡ HARDWARE FLASHING (ASYNC OPENOCD / ESPTOOL)"
                            font.bold: true
                            font.pixelSize: 11
                            color: flashController.isFlashing ? "#98C379" : "#61AFEF"
                        }

                        ToolSeparator {}

                        Label { text: "Target:"; color: "#ABB2BF"; font.pixelSize: 12 }
                        ComboBox {
                            id: flashTargetCombo
                            model: ["STM32 (OpenOCD + ST-Link)", "RISC-V (OpenOCD + FTDI)", "ESP32 (esptool)"]
                            currentIndex: 0
                            Layout.preferredWidth: 200
                        }

                        Label { text: "Binary:"; color: "#ABB2BF"; font.pixelSize: 12 }
                        TextField {
                            id: flashBinInput
                            text: "output_ugfx/build/ugfx_app.bin"
                            Layout.preferredWidth: 240
                        }

                        Button {
                            text: flashController.isFlashing ? "Flashing..." : "⚡ Flash MCU"
                            enabled: !flashController.isFlashing
                            highlighted: true
                            onClicked: {
                                flashConsoleExpanded = true;
                                if (flashTargetCombo.currentIndex === 0) {
                                    flashController.flashSTM32(flashBinInput.text);
                                } else if (flashTargetCombo.currentIndex === 1) {
                                    flashController.flashRISCV(flashBinInput.text);
                                } else {
                                    flashController.flashESP32(flashBinInput.text);
                                }
                            }
                        }

                        Button {
                            text: "⏹ Abort"
                            enabled: flashController.isFlashing
                            onClicked: flashController.cancelFlash()
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: flashConsoleExpanded ? "▼ Collapse" : "▲ Expand Console"
                            onClicked: flashConsoleExpanded = !flashConsoleExpanded
                        }
                    }
                }

                // Real-Time Toolchain Terminal Console (Prompt 3 requirement #4)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: flashConsoleExpanded
                    color: "#0F1014"

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 6

                        TextArea {
                            id: flashConsoleArea
                            readOnly: true
                            font.family: "Consolas, Courier, monospace"
                            font.pixelSize: 12
                            color: "#7EE787"
                            background: null
                            selectByMouse: true
                            wrapMode: TextEdit.WrapAnywhere
                        }
                    }
                }
            }
        }
    }

    // Connect to FlashController real-time output signal (Prompt 3 requirement #4)
    Connections {
        target: flashController
        function onConsoleOutputUpdate(text) {
            flashConsoleArea.append(text);
        }
    }

    // Status Notification Toast
    Popup {
        id: statusToast
        x: (parent.width - width) / 2
        y: parent.height - 80
        width: 440
        height: 42
        modal: false
        focus: false
        closePolicy: Popup.CloseOnPressOutside
        background: Rectangle { color: "#21252B"; border.color: "#61AFEF"; radius: 6 }
        Label {
            id: statusToastText
            anchors.centerIn: parent
            color: "#98C379"
            font.bold: true
        }
        function show(msg) {
            statusToastText.text = msg;
            statusToast.open();
            toastTimer.restart();
        }
        Timer { id: toastTimer; interval: 3500; onTriggered: statusToast.close() }
    }
}
