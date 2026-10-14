#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSplitter>

#include "workspace/TerminalWidget.h"
#include "workspace/SerialMonitorWidget.h"
#include "workspace/CodeWorkspaceView.h"

class TestWorkspaceWidgets : public QObject {
    Q_OBJECT

private slots:
    void testTerminalInitialLayout();
    void testTerminalNoQuickCommandButtons();
    void testTerminalProjectAndFileLabels();
    void testTerminalWorkingDirectoryAndPrompt();
    void testTerminalCommandExecution();
    void testTerminalCommandHistory();
    void testTerminalCdDirectoryTracking();
    void testTerminalClearAndStop();

    void testSerialMonitorInitialState();
    void testSerialMonitorControlsAndTheme();
    void testSerialMonitorDisabledWhileDisconnected();
    void testSerialMonitorConnectionWorkflow();

    void testWorkspaceViewResizableSplitter();
    void testWorkspaceViewBottomPanelToggle();
    void testWorkspaceViewActiveFileSync();
    void testCaptureScreenshots();
};

void TestWorkspaceWidgets::testTerminalInitialLayout() {
    TerminalWidget terminal;
    
    // Header should contain project and file labels
    QLabel* projLabel = terminal.findChild<QLabel*>("terminalProjectLabel");
    QVERIFY(projLabel != nullptr);
    QVERIFY(projLabel->text().contains("Project:"));

    QLabel* fileLabel = terminal.findChild<QLabel*>("terminalFileLabel");
    QVERIFY(fileLabel != nullptr);
    QVERIFY(fileLabel->text().contains("File:"));

    // Header should contain Clear and Stop buttons
    QPushButton* clearBtn = terminal.findChild<QPushButton*>("terminalClearBtn");
    QVERIFY(clearBtn != nullptr);
    QCOMPARE(clearBtn->text(), QString("Clear"));

    QPushButton* stopBtn = terminal.findChild<QPushButton*>("terminalStopBtn");
    QVERIFY(stopBtn != nullptr);
    QCOMPARE(stopBtn->text(), QString("Stop"));

    // Prompt label and Command input
    QLabel* promptLabel = terminal.findChild<QLabel*>("terminalPromptLabel");
    QVERIFY(promptLabel != nullptr);
    QVERIFY(promptLabel->text().endsWith("$"));

    QLineEdit* cmdInput = terminal.findChild<QLineEdit*>("terminalCommandInput");
    QVERIFY(cmdInput != nullptr);
    QCOMPARE(cmdInput->placeholderText(), QString("Interactive command input..."));

    QPushButton* runBtn = terminal.findChild<QPushButton*>("terminalRunBtn");
    QVERIFY(runBtn != nullptr);
    QCOMPARE(runBtn->text(), QString("Run"));
}

void TestWorkspaceWidgets::testTerminalNoQuickCommandButtons() {
    TerminalWidget terminal;
    
    // Ensure quick-command buttons are completely removed
    QList<QPushButton*> buttons = terminal.findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        QString text = btn->text().trimmed();
        QVERIFY2(text != "pwd", "Quick command 'pwd' button should not exist");
        QVERIFY2(text != "ls -la", "Quick command 'ls -la' button should not exist");
        QVERIFY2(text != "cmake --version", "Quick command 'cmake --version' button should not exist");
        QVERIFY2(text != "arm-gcc --version", "Quick command 'arm-gcc --version' button should not exist");
        QVERIFY2(text != "git status", "Quick command 'git status' button should not exist");
        QVERIFY2(!text.isEmpty(), "Blank button without text should not exist");
    }
}

void TestWorkspaceWidgets::testTerminalProjectAndFileLabels() {
    TerminalWidget terminal;
    
    terminal.setProjectName("EmbeddedTestProject");
    QLabel* projLabel = terminal.findChild<QLabel*>("terminalProjectLabel");
    QVERIFY(projLabel != nullptr);
    QVERIFY(projLabel->text().contains("EmbeddedTestProject"));

    terminal.setActiveFile("Src/main.c");
    QLabel* fileLabel = terminal.findChild<QLabel*>("terminalFileLabel");
    QVERIFY(fileLabel != nullptr);
    QVERIFY(fileLabel->text().contains("Src/main.c"));
}

void TestWorkspaceWidgets::testTerminalWorkingDirectoryAndPrompt() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString projectRoot = tempDir.path();

    TerminalWidget terminal(projectRoot);
    QCOMPARE(terminal.currentWorkingDirectory(), projectRoot);

    QLabel* promptLabel = terminal.findChild<QLabel*>("terminalPromptLabel");
    QVERIFY(promptLabel != nullptr);
    QCOMPARE(promptLabel->text(), projectRoot + " $");
}

void TestWorkspaceWidgets::testTerminalCommandExecution() {
    QTemporaryDir tempDir;
    TerminalWidget terminal(tempDir.path());

    terminal.executeCommand("echo \"SHELL_VERIFICATION_TEST_OUTPUT\"");
    
    // Wait for the process to finish
    int attempts = 0;
    while (terminal.isProcessRunning() && attempts < 100) {
        QTest::qWait(50);
        attempts++;
    }
    QVERIFY(!terminal.isProcessRunning());

    QPlainTextEdit* outputArea = terminal.findChild<QPlainTextEdit*>("terminalOutputArea");
    QVERIFY(outputArea != nullptr);
    QVERIFY(outputArea->toPlainText().contains("SHELL_VERIFICATION_TEST_OUTPUT"));
    QVERIFY(outputArea->toPlainText().contains("[Process exited 0]"));
}

void TestWorkspaceWidgets::testTerminalCommandHistory() {
    TerminalWidget terminal;
    QLineEdit* cmdInput = terminal.findChild<QLineEdit*>("terminalCommandInput");
    QVERIFY(cmdInput != nullptr);

    terminal.executeCommand("cmd_first");
    terminal.executeCommand("cmd_second");
    terminal.executeCommand("cmd_third");

    // History contains these 3 commands
    // Simulating Key_Up navigation
    QTest::keyClick(cmdInput, Qt::Key_Up);
    QCOMPARE(cmdInput->text(), QString("cmd_third"));

    QTest::keyClick(cmdInput, Qt::Key_Up);
    QCOMPARE(cmdInput->text(), QString("cmd_second"));

    QTest::keyClick(cmdInput, Qt::Key_Up);
    QCOMPARE(cmdInput->text(), QString("cmd_first"));

    // Simulating Key_Down navigation
    QTest::keyClick(cmdInput, Qt::Key_Down);
    QCOMPARE(cmdInput->text(), QString("cmd_second"));

    QTest::keyClick(cmdInput, Qt::Key_Down);
    QCOMPARE(cmdInput->text(), QString("cmd_third"));

    QTest::keyClick(cmdInput, Qt::Key_Down);
    QCOMPARE(cmdInput->text(), QString(""));
}

void TestWorkspaceWidgets::testTerminalCdDirectoryTracking() {
    QTemporaryDir tempDir;
    QDir dir(tempDir.path());
    dir.mkdir("sub_build_dir");
    QString subDirPath = dir.filePath("sub_build_dir");

    TerminalWidget terminal(tempDir.path());
    QCOMPARE(terminal.currentWorkingDirectory(), tempDir.path());

    // Execute cd sub_build_dir
    terminal.executeCommand("cd sub_build_dir");

    int attempts = 0;
    while (terminal.isProcessRunning() && attempts < 100) {
        QTest::qWait(50);
        attempts++;
    }
    QVERIFY(!terminal.isProcessRunning());

    // The current working directory should update to canonical path
    QCOMPARE(QDir(terminal.currentWorkingDirectory()).canonicalPath(), QDir(subDirPath).canonicalPath());

    QLabel* promptLabel = terminal.findChild<QLabel*>("terminalPromptLabel");
    QVERIFY(promptLabel != nullptr);
    QVERIFY(promptLabel->text().endsWith("$"));
    QVERIFY(promptLabel->text().contains("sub_build_dir"));
}

void TestWorkspaceWidgets::testTerminalClearAndStop() {
    TerminalWidget terminal;
    QPlainTextEdit* outputArea = terminal.findChild<QPlainTextEdit*>("terminalOutputArea");
    QVERIFY(outputArea != nullptr);

    terminal.executeCommand("echo 'temporary text'");
    QTest::qWait(200);
    QVERIFY(!outputArea->toPlainText().isEmpty());

    QPushButton* clearBtn = terminal.findChild<QPushButton*>("terminalClearBtn");
    QVERIFY(clearBtn != nullptr);
    clearBtn->click();
    QVERIFY(outputArea->toPlainText().isEmpty());

    // Test stop on long running command
    terminal.executeCommand("sleep 10");
    QTest::qWait(100);
    QVERIFY(terminal.isProcessRunning());

    QPushButton* stopBtn = terminal.findChild<QPushButton*>("terminalStopBtn");
    QVERIFY(stopBtn != nullptr);
    stopBtn->click();
    QTest::qWait(200);
    QVERIFY(!terminal.isProcessRunning());
}

void TestWorkspaceWidgets::testSerialMonitorInitialState() {
    SerialMonitorWidget monitor;

    QVERIFY(!monitor.isConnected());

    QPushButton* connectBtn = monitor.findChild<QPushButton*>("serialConnectBtn");
    QPushButton* disconnectBtn = monitor.findChild<QPushButton*>("serialDisconnectBtn");
    QPushButton* sendBtn = monitor.findChild<QPushButton*>("serialSendBtn");
    QLineEdit* sendInput = monitor.findChild<QLineEdit*>("serialSendInput");
    QLabel* statusLabel = monitor.findChild<QLabel*>("serialStatusLabel");

    QVERIFY(connectBtn != nullptr);
    QVERIFY(disconnectBtn != nullptr);
    QVERIFY(sendBtn != nullptr);
    QVERIFY(sendInput != nullptr);
    QVERIFY(statusLabel != nullptr);

    // Initial state: Connect enabled, Disconnect disabled, Send disabled, Input disabled
    QVERIFY(connectBtn->isEnabled());
    QVERIFY(!disconnectBtn->isEnabled());
    QVERIFY(!sendBtn->isEnabled());
    QVERIFY(!sendInput->isEnabled());
    QVERIFY(statusLabel->text().contains("Disconnected"));
}

void TestWorkspaceWidgets::testSerialMonitorControlsAndTheme() {
    SerialMonitorWidget monitor;

    // Check no unexplained white buttons exist
    QList<QPushButton*> buttons = monitor.findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        QString text = btn->text().trimmed();
        QVERIFY2(!text.isEmpty(), "Blank button without text must not exist in Serial Monitor");
    }

    // Verify baud rates
    QComboBox* baudCombo = monitor.findChild<QComboBox*>("serialBaudCombo");
    QVERIFY(baudCombo != nullptr);
    QVERIFY(baudCombo->count() > 0);
    QVERIFY(baudCombo->findText("115200") != -1);
    QVERIFY(baudCombo->findText("9600") != -1);

    // Verify text/hex mode switcher
    QComboBox* modeCombo = monitor.findChild<QComboBox*>("serialModeCombo");
    QVERIFY(modeCombo != nullptr);
    QVERIFY(modeCombo->findText("Text") != -1);
    QVERIFY(modeCombo->findText("Hex") != -1);

    // Verify line ending switcher
    QComboBox* lineEndingCombo = monitor.findChild<QComboBox*>("serialLineEndingCombo");
    QVERIFY(lineEndingCombo != nullptr);
    QVERIFY(lineEndingCombo->findText("CRLF (\\r\\n)") != -1);
}

void TestWorkspaceWidgets::testSerialMonitorDisabledWhileDisconnected() {
    SerialMonitorWidget monitor;
    QLineEdit* sendInput = monitor.findChild<QLineEdit*>("serialSendInput");
    QPushButton* sendBtn = monitor.findChild<QPushButton*>("serialSendBtn");

    QVERIFY(!sendBtn->isEnabled());
    QVERIFY(!sendInput->isEnabled());

    // Attempting to send while disconnected should not crash or send data
    monitor.sendData("test payload");
    QPlainTextEdit* logArea = monitor.findChild<QPlainTextEdit*>("serialLogArea");
    QVERIFY(logArea != nullptr);
    QVERIFY(logArea->toPlainText().contains("Error: Cannot send while disconnected."));
}

void TestWorkspaceWidgets::testSerialMonitorConnectionWorkflow() {
    SerialMonitorWidget monitor;
    
    // Disconnect when already disconnected should be safe no-op
    monitor.disconnectPort();
    QVERIFY(!monitor.isConnected());

    // Connect to nonexistent port should safely fail and report error
    monitor.connectPort("nonexistent_test_port", 115200);
    QVERIFY(!monitor.isConnected());
}

void TestWorkspaceWidgets::testWorkspaceViewResizableSplitter() {
    CodeWorkspaceView view;
    view.resize(1000, 700);

    QSplitter* rightSplitter = view.findChild<QSplitter*>("rightVerticalSplitter");
    QVERIFY(rightSplitter != nullptr);

    // Splitter handle width must be >= 5px for comfortable dragging
    QVERIFY(rightSplitter->handleWidth() >= 5);

    // Upper boundary of bottom panel is resizable
    QList<int> initialSizes = rightSplitter->sizes();
    QCOMPARE(initialSizes.size(), 2);
    QVERIFY(initialSizes[0] > 0); // Editor area
    QVERIFY(initialSizes[1] > 0); // Bottom panel area

    // Resize bottom panel by setting new splitter sizes (simulating dragging upper boundary)
    QList<int> newSizes = { 400, 300 };
    rightSplitter->setSizes(newSizes);
    QList<int> updatedSizes = rightSplitter->sizes();
    QVERIFY(updatedSizes[1] > 0);
}

void TestWorkspaceWidgets::testWorkspaceViewBottomPanelToggle() {
    CodeWorkspaceView view;
    view.resize(1000, 700);

    QVERIFY(view.isBottomPanelVisible());

    view.toggleBottomPanel();
    QVERIFY(!view.isBottomPanelVisible());

    view.toggleBottomPanel();
    QVERIFY(view.isBottomPanelVisible());
}

void TestWorkspaceWidgets::testWorkspaceViewActiveFileSync() {
    QTemporaryDir tempDir;
    QDir dir(tempDir.path());
    QString testFilePath = dir.filePath("test_firmware.c");
    QFile file(testFilePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("int main() { return 0; }\n");
    file.close();

    CodeWorkspaceView view;
    view.openProject(tempDir.path());
    view.openFile(testFilePath);

    TerminalWidget* terminal = view.findChild<TerminalWidget*>();
    QVERIFY(terminal != nullptr);

    QLabel* fileLabel = terminal->findChild<QLabel*>("terminalFileLabel");
    QVERIFY(fileLabel != nullptr);
    QVERIFY(fileLabel->text().contains("test_firmware.c"));
}

void TestWorkspaceWidgets::testCaptureScreenshots() {
    QTemporaryDir tempDir;
    QDir dir(tempDir.path());
    QString testFilePath = dir.filePath("main.c");
    QFile file(testFilePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("/* Main embedded firmware application */\n#include <stdio.h>\n\nint main(void) {\n    printf(\"STM32 Firmware Active\\n\");\n    return 0;\n}\n");
    file.close();

    CodeWorkspaceView view;
    view.resize(1100, 750);
    view.openProject(tempDir.path());
    view.openFile(testFilePath);

    TerminalWidget* terminal = view.findChild<TerminalWidget*>();
    QVERIFY(terminal != nullptr);
    terminal->executeCommand("ls -la");
    QTest::qWait(200);

    QTabWidget* bottomTabs = view.bottomTabs();
    QVERIFY(bottomTabs != nullptr);

    // 1. Render Terminal view
    bottomTabs->setCurrentWidget(terminal);
    view.show();
    QTest::qWait(100);

    QPixmap terminalPixmap(view.size());
    view.render(&terminalPixmap);
    QString termImgPath = "/home/cherry/.gemini/antigravity-ide/brain/8b72b437-82a8-4207-a9bb-283e17f9d724/terminal_view.png";
    terminalPixmap.save(termImgPath);
    QVERIFY(QFile::exists(termImgPath));

    // 2. Render Serial Monitor view
    SerialMonitorWidget* serial = view.serialMonitorWidget();
    QVERIFY(serial != nullptr);
    bottomTabs->setCurrentWidget(serial);
    QTest::qWait(100);

    QPixmap serialPixmap(view.size());
    view.render(&serialPixmap);
    QString serialImgPath = "/home/cherry/.gemini/antigravity-ide/brain/8b72b437-82a8-4207-a9bb-283e17f9d724/serial_monitor_view.png";
    serialPixmap.save(serialImgPath);
    QVERIFY(QFile::exists(serialImgPath));
}

QTEST_MAIN(TestWorkspaceWidgets)
#include "test_workspace_widgets.moc"
