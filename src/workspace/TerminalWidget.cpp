#include "TerminalWidget.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QScrollBar>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_workingDirectory(QDir::currentPath())
    , m_projectRoot(QDir::currentPath())
{
    setupUi();
    setWorkingDirectory(QDir::currentPath());
}

TerminalWidget::TerminalWidget(const QString& workingDir, QWidget *parent)
    : QWidget(parent)
    , m_workingDirectory(workingDir.isEmpty() ? QDir::currentPath() : workingDir)
    , m_projectRoot(workingDir.isEmpty() ? QDir::currentPath() : workingDir)
{
    setupUi();
    setWorkingDirectory(m_workingDirectory);
}

TerminalWidget::~TerminalWidget() {
    stopProcess();
}

void TerminalWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 1. Header Toolbar
    // Layout: Terminal | Project: MyProject | File: path | [Clear] | [Stop]
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(10);

    m_titleLabel = new QLabel("Terminal", this);
    m_titleLabel->setObjectName("terminalTitleLabel");
    m_titleLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;").arg(Theme::TextPrimary));
    headerLayout->addWidget(m_titleLabel);

    m_projectLabel = new QLabel(QString("Project: %1").arg(m_projectName), this);
    m_projectLabel->setObjectName("terminalProjectLabel");
    m_projectLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 500;").arg(Theme::Accent));
    headerLayout->addWidget(m_projectLabel);

    m_fileLabel = new QLabel("File: (None)", this);
    m_fileLabel->setObjectName("terminalFileLabel");
    m_fileLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    headerLayout->addWidget(m_fileLabel);

    headerLayout->addStretch(1);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setObjectName("terminalClearBtn");
    m_clearBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 3px 12px; font-size: 11px; font-weight: 500; } "
        "QPushButton:hover { background: %4; color: %5; } "
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, Theme::Accent, Theme::AccentText));
    connect(m_clearBtn, &QPushButton::clicked, this, &TerminalWidget::clear);
    headerLayout->addWidget(m_clearBtn);

    auto* sepLabel = new QLabel("|", this);
    sepLabel->setStyleSheet(QString("color: %1;").arg(Theme::BorderSubtle));
    headerLayout->addWidget(sepLabel);

    m_stopBtn = new QPushButton("Stop", this);
    m_stopBtn->setObjectName("terminalStopBtn");
    m_stopBtn->setEnabled(false);
    m_stopBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 3px 12px; font-size: 11px; font-weight: bold; } "
        "QPushButton:hover:enabled { background: %4; color: #ffffff; border-color: %4; } "
        "QPushButton:disabled { background: %5; color: %6; border-color: %7; } "
    ).arg(Theme::BgRaised, Theme::Danger, Theme::BorderSubtle, Theme::Danger, Theme::BgBase, Theme::TextMuted, Theme::BorderSubtle));
    connect(m_stopBtn, &QPushButton::clicked, this, &TerminalWidget::stopProcess);
    headerLayout->addWidget(m_stopBtn);

    mainLayout->addLayout(headerLayout);

    // 2. Output Area (Monospace shell console)
    m_outputView = new QPlainTextEdit(this);
    m_outputView->setObjectName("terminalOutputArea");
    m_outputView->setReadOnly(true);
    QFont termFont("Monospace", 10);
    termFont.setStyleHint(QFont::Monospace);
    m_outputView->setFont(termFont);
    m_outputView->setStyleSheet(QString(
        "QPlainTextEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 6px; "
        "  font-family: monospace, 'Courier New', 'Consolas'; "
        "} "
    ).arg(Theme::BgBase, Theme::TextPrimary, Theme::BorderSubtle));
    mainLayout->addWidget(m_outputView, 1);

    // 3. Prompt & Interactive Command Input
    m_promptLabel = new QLabel(QString("%1 $").arg(m_workingDirectory), this);
    m_promptLabel->setObjectName("terminalPromptLabel");
    m_promptLabel->setFont(termFont);
    m_promptLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px; margin-top: 2px;").arg(Theme::Accent));
    mainLayout->addWidget(m_promptLabel);

    auto* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->setSpacing(6);

    m_inputLine = new QLineEdit(this);
    m_inputLine->setObjectName("terminalCommandInput");
    m_inputLine->setFont(termFont);
    m_inputLine->setPlaceholderText("Interactive command input...");
    m_inputLine->setStyleSheet(QString(
        "QLineEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 5px 10px; "
        "} "
        "QLineEdit:focus { "
        "  border: 1px solid %4; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, Theme::Accent));
    m_inputLine->installEventFilter(this);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &TerminalWidget::onReturnPressed);
    inputRow->addWidget(m_inputLine, 1);

    m_runBtn = new QPushButton("Run", this);
    m_runBtn->setObjectName("terminalRunBtn");
    m_runBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #0284c7; "
        "  color: #ffffff; "
        "  font-weight: bold; "
        "  border: 1px solid #0284c7; "
        "  border-radius: 4px; "
        "  padding: 5px 18px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover:enabled { "
        "  background-color: #0369a1; "
        "  border-color: #0369a1; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #262626; "
        "  color: #666666; "
        "  border-color: #3f3f3f; "
        "} "
    );
    connect(m_runBtn, &QPushButton::clicked, this, &TerminalWidget::onReturnPressed);
    inputRow->addWidget(m_runBtn);

    mainLayout->addLayout(inputRow);
}

void TerminalWidget::setWorkingDirectory(const QString& dir) {
    if (!dir.isEmpty() && QDir(dir).exists()) {
        m_workingDirectory = QDir(dir).canonicalPath();
    } else {
        m_workingDirectory = QDir::currentPath();
    }
    updatePromptLabel();
}

void TerminalWidget::setProjectName(const QString& name) {
    if (!name.isEmpty()) {
        m_projectName = name;
        updateHeaderLabels();
    }
}

void TerminalWidget::setProjectRoot(const QString& rootDir) {
    m_projectRoot = rootDir;
    setWorkingDirectory(rootDir);
    updateHeaderLabels();
}

void TerminalWidget::setActiveFile(const QString& filePath) {
    m_activeFile = filePath;
    updateHeaderLabels();
}

void TerminalWidget::updateHeaderLabels() {
    if (m_projectLabel) {
        m_projectLabel->setText(QString("Project: %1").arg(m_projectName));
    }
    if (m_fileLabel) {
        if (m_activeFile.isEmpty()) {
            m_fileLabel->setText("File: (None)");
            m_fileLabel->setToolTip("");
        } else {
            QString displayPath = m_activeFile;
            if (!m_projectRoot.isEmpty() && m_activeFile.startsWith(m_projectRoot)) {
                displayPath = QDir(m_projectRoot).relativeFilePath(m_activeFile);
            }
            m_fileLabel->setText(QString("File: %1").arg(displayPath));
            m_fileLabel->setToolTip(m_activeFile);
        }
    }
}

void TerminalWidget::updatePromptLabel() {
    if (m_promptLabel) {
        m_promptLabel->setText(QString("%1 $").arg(m_workingDirectory));
        m_promptLabel->setToolTip(m_workingDirectory);
    }
}

void TerminalWidget::executeCommand(const QString& command) {
    QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) return;

    if (trimmed == "clear") {
        clear();
        return;
    }

    // Add to history
    m_history.removeAll(trimmed);
    m_history.append(trimmed);
    m_historyIndex = m_history.size();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        appendOutput("Warning: Another command is currently executing. Click Stop first.\n", QColor(Theme::Warning));
        return;
    }

    // Echo command with prompt
    appendOutput(QString("%1 $ %2\n").arg(m_workingDirectory, trimmed), QColor(Theme::Accent));

    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyReadStandardOutput);
        connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyReadStandardError);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &TerminalWidget::onProcessFinished);
        connect(m_process, &QProcess::errorOccurred, this, &TerminalWidget::onProcessError);
    }

    m_process->setWorkingDirectory(m_workingDirectory);

    m_stopBtn->setEnabled(true);
    m_runBtn->setEnabled(false);

    // Execute through shell and report final directory after command completes (supports cd)
    QString shellCmd = QString("%1; __ec=$?; echo \"__CWD_REPORT__$(pwd)\"; exit $__ec").arg(trimmed);
    m_process->start("/bin/bash", QStringList() << "-c" << shellCmd);
}

void TerminalWidget::onReturnPressed() {
    QString cmd = m_inputLine->text();
    m_inputLine->clear();
    executeCommand(cmd);
}

void TerminalWidget::onReadyReadStandardOutput() {
    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    QString text = QString::fromUtf8(data);

    // Extract __CWD_REPORT__ if present
    const QString marker = "__CWD_REPORT__";
    int markerIdx = text.indexOf(marker);
    if (markerIdx != -1) {
        int endOfLine = text.indexOf('\n', markerIdx);
        QString newCwd;
        if (endOfLine != -1) {
            newCwd = text.mid(markerIdx + marker.length(), endOfLine - (markerIdx + marker.length())).trimmed();
            text.remove(markerIdx, (endOfLine - markerIdx) + 1);
        } else {
            newCwd = text.mid(markerIdx + marker.length()).trimmed();
            text.remove(markerIdx, text.length() - markerIdx);
        }

        if (!newCwd.isEmpty() && QDir(newCwd).exists()) {
            m_workingDirectory = QDir(newCwd).canonicalPath();
            updatePromptLabel();
        }
    }

    if (!text.isEmpty()) {
        appendOutput(text, QColor(Theme::TextPrimary));
    }
}

void TerminalWidget::onReadyReadStandardError() {
    if (m_process) {
        QByteArray data = m_process->readAllStandardError();
        appendOutput(QString::fromUtf8(data), QColor(Theme::Danger));
    }
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    m_stopBtn->setEnabled(false);
    m_runBtn->setEnabled(true);

    if (exitCode == 0) {
        appendOutput("[Process exited 0]\n", QColor(Theme::Success));
    } else {
        appendOutput(QString("[Process exited with code %1]\n").arg(exitCode), QColor(Theme::Warning));
    }
}

void TerminalWidget::onProcessError(QProcess::ProcessError error) {
    m_stopBtn->setEnabled(false);
    m_runBtn->setEnabled(true);
    if (error == QProcess::FailedToStart) {
        appendOutput("[Failed to start process]\n", QColor(Theme::Danger));
    } else if (error == QProcess::Crashed) {
        appendOutput("[Process terminated]\n", QColor(Theme::Danger));
    }
}

void TerminalWidget::stopProcess() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(300)) {
            m_process->kill();
            m_process->waitForFinished(200);
        }
        appendOutput("[Process stopped by user]\n", QColor(Theme::Warning));
        m_stopBtn->setEnabled(false);
        m_runBtn->setEnabled(true);
    }
}

void TerminalWidget::clear() {
    m_outputView->clear();
}

void TerminalWidget::appendOutput(const QString& text, const QColor& color) {
    QTextCursor cursor = m_outputView->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    format.setForeground(color.isValid() ? color : QColor(Theme::TextPrimary));
    cursor.setCharFormat(format);
    cursor.insertText(text);

    m_outputView->verticalScrollBar()->setValue(m_outputView->verticalScrollBar()->maximum());
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_inputLine && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (!m_history.isEmpty() && m_historyIndex > 0) {
                --m_historyIndex;
                m_inputLine->setText(m_history.at(m_historyIndex));
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (m_historyIndex < m_history.size() - 1) {
                ++m_historyIndex;
                m_inputLine->setText(m_history.at(m_historyIndex));
            } else {
                m_historyIndex = m_history.size();
                m_inputLine->clear();
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
