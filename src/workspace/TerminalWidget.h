#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QStringList>

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    explicit TerminalWidget(const QString& workingDir, QWidget *parent = nullptr);
    ~TerminalWidget() override;

    void setWorkingDirectory(const QString& dir);
    QString workingDirectory() const { return m_workingDirectory; }
    QString currentWorkingDirectory() const { return m_workingDirectory; }
    bool isProcessRunning() const { return m_process && m_process->state() != QProcess::NotRunning; }
    QString outputText() const { return m_outputView ? m_outputView->toPlainText() : QString(); }
    QProcess* process() const { return m_process; }

    void setProjectName(const QString& name);
    void setActiveFile(const QString& filePath);
    void setProjectRoot(const QString& rootDir);

    QLabel* projectLabel() const { return m_projectLabel; }
    QLabel* fileLabel() const { return m_fileLabel; }
    QLabel* promptLabel() const { return m_promptLabel; }
    QLineEdit* commandInput() const { return m_inputLine; }
    QPushButton* clearButton() const { return m_clearBtn; }
    QPushButton* stopButton() const { return m_stopBtn; }
    QPushButton* runButton() const { return m_runBtn; }
    QPlainTextEdit* outputView() const { return m_outputView; }

public slots:
    void executeCommand(const QString& command);
    void clear();
    void stopProcess();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onReturnPressed();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setupUi();
    void appendOutput(const QString& text, const QColor& color = QColor());
    void updatePromptLabel();
    void updateHeaderLabels();

    QString m_workingDirectory;
    QString m_projectName = "MyEmbeddedProject";
    QString m_activeFile;
    QString m_projectRoot;

    QProcess *m_process = nullptr;

    // Header widgets
    QLabel *m_titleLabel = nullptr;
    QLabel *m_projectLabel = nullptr;
    QLabel *m_fileLabel = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;

    // Terminal area
    QPlainTextEdit *m_outputView = nullptr;

    // Prompt & Input widgets
    QLabel *m_promptLabel = nullptr;
    QLineEdit *m_inputLine = nullptr;
    QPushButton *m_runBtn = nullptr;

    QStringList m_history;
    int m_historyIndex = -1;
};
