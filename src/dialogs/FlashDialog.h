#pragma once

#include <QDialog>
#include <QProcess>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class DeviceManager;

class FlashDialog : public QDialog {
    Q_OBJECT

public:
    explicit FlashDialog(DeviceManager* deviceManager, QWidget* parent = nullptr);

    static QString commandForProfile(const QString& profile, const QString& port,
                                     const QString& firmwarePath);

private slots:
    void refreshPorts();
    void updateCommand();
    void chooseFirmware();
    void copyCommand();
    void runCommand();
    void appendProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    DeviceManager* m_deviceManager = nullptr;
    QComboBox* m_boardCombo = nullptr;
    QComboBox* m_portCombo = nullptr;
    QLineEdit* m_firmwareEdit = nullptr;
    QPlainTextEdit* m_commandEdit = nullptr;
    QPlainTextEdit* m_logEdit = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_runButton = nullptr;
    QProcess* m_process = nullptr;

    void reloadPortList();
    void selectDetectedProfile();
    void appendLog(const QString& text);
};