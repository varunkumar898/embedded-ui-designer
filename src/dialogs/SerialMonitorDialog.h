#pragma once

#include <QDialog>

class DeviceManager;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QSerialPort;

class SerialMonitorDialog : public QDialog {
    Q_OBJECT

public:
    explicit SerialMonitorDialog(DeviceManager* deviceManager, QWidget* parent = nullptr);

private slots:
    void refreshPorts();
    void connectPort();
    void disconnectPort();
    void sendText();
    void readIncoming();

private:
    DeviceManager* m_deviceManager = nullptr;
    QSerialPort* m_port = nullptr;
    QComboBox* m_portPicker = nullptr;
    QSpinBox* m_baudRate = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_connectButton = nullptr;
    QPushButton* m_disconnectButton = nullptr;
    QPushButton* m_sendButton = nullptr;

    void appendLog(const QString& text);
    void updateControls();
};