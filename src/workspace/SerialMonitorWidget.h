#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>

#ifdef HAVE_QT_SERIALPORT
#include <QSerialPort>
#endif

class SerialMonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit SerialMonitorWidget(QWidget *parent = nullptr);
    ~SerialMonitorWidget() override;

    bool isConnected() const { return m_isConnected; }

    QPushButton* connectButton() const { return m_connectBtn; }
    QPushButton* disconnectButton() const { return m_disconnectBtn; }
    QPushButton* sendButton() const { return m_sendBtn; }
    QLineEdit* inputField() const { return m_inputLine; }
    QLabel* statusLabel() const { return m_statusBadge; }
    QComboBox* baudCombo() const { return m_baudCombo; }
    QComboBox* modeCombo() const { return m_formatCombo; }
    QComboBox* lineEndingCombo() const { return m_lineEndingCombo; }
    QPlainTextEdit* logView() const { return m_logView; }

public slots:
    void refreshPorts();
    void connectPort();
    void connectPort(const QString& portName, int baudRate);
    void disconnectPort();
    void sendData();
    void sendData(const QString& text);
    void clearLog();

private slots:
    void onReadyRead();

private:
    void setupUi();
    void updateControls();
    void appendLog(const QString& text, bool isIncoming = true);

    QComboBox *m_portCombo = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_disconnectBtn = nullptr;
    QLabel *m_statusBadge = nullptr;
    QComboBox *m_lineEndingCombo = nullptr;
    QComboBox *m_formatCombo = nullptr;
    QPushButton *m_clearBtn = nullptr;

    QPlainTextEdit *m_logView = nullptr;
    QLineEdit *m_inputLine = nullptr;
    QPushButton *m_sendBtn = nullptr;

#ifdef HAVE_QT_SERIALPORT
    QSerialPort *m_serialPort = nullptr;
#endif
    bool m_isConnected = false;
};
