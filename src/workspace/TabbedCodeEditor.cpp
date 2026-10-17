#include "TabbedCodeEditor.h"
#include "CodeEditor.h"
#include "DetachableTabWidget.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <QTabBar>
#include <QDebug>

TabbedCodeEditor::TabbedCodeEditor(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &TabbedCodeEditor::saveCurrentFile);
}

TabbedCodeEditor::~TabbedCodeEditor() {
}

void TabbedCodeEditor::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top Header / Bar
    auto* topBar = new QWidget(this);
    topBar->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(Theme::BgSurface, Theme::BorderSubtle));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 4, 8, 4);
    topLayout->setSpacing(8);

    m_saveBtn = new QPushButton("Save", topBar);
    m_saveBtn->setToolTip("Save Active File (Ctrl+S)");
    m_saveBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 3px 10px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; color: %6; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::Accent, Theme::AccentText));
    connect(m_saveBtn, &QPushButton::clicked, this, &TabbedCodeEditor::saveCurrentFile);
    topLayout->addWidget(m_saveBtn);

    m_saveAllBtn = new QPushButton("Save All", topBar);
    m_saveAllBtn->setToolTip("Save All Open Files");
    m_saveAllBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 3px 10px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; color: %6; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::Accent, Theme::AccentText));
    connect(m_saveAllBtn, &QPushButton::clicked, this, &TabbedCodeEditor::saveAllFiles);
    topLayout->addWidget(m_saveAllBtn);

    m_filePathLabel = new QLabel(topBar);
    m_filePathLabel->setStyleSheet(QString("color: %1; font-size: 11px; margin-left: 8px;").arg(Theme::TextMuted));
    topLayout->addWidget(m_filePathLabel, 1);

    mainLayout->addWidget(topBar);

    // Tab Widget
    m_tabWidget = new DetachableTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setStyleSheet(QString(
        "QTabWidget::pane { border: none; background: %1; } "
        "QTabBar::tab { "
        "  background: %2; "
        "  color: %3; "
        "  border: 1px solid %4; "
        "  border-bottom: none; "
        "  padding: 6px 14px; "
        "  font-weight: 500; "
        "  font-size: 12px; "
        "  min-width: 80px; "
        "} "
        "QTabBar::tab:selected { "
        "  background: %1; "
        "  color: %5; "
        "  border-top: 2px solid %6; "
        "  font-weight: bold; "
        "} "
        "QTabBar::tab:hover:!selected { "
        "  background: %7; "
        "  color: %5; "
        "} "
    ).arg(Theme::BgBase, Theme::BgRaised, Theme::TextMuted, Theme::BorderSubtle,
          Theme::TextPrimary, Theme::Accent, Theme::BgOverlay));

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TabbedCodeEditor::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TabbedCodeEditor::onCurrentTabChanged);

    mainLayout->addWidget(m_tabWidget, 1);

    // Status Footer
    auto* statusBar = new QWidget(this);
    statusBar->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;")
        .arg(Theme::BgSurface, Theme::BorderSubtle));
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(12, 3, 12, 3);
    statusLayout->setSpacing(12);

    m_statusLabel = new QLabel("Ready", statusBar);
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch(1);

    mainLayout->addWidget(statusBar);
}

bool TabbedCodeEditor::openFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;

    QFileInfo info(filePath);
    QString canonicalPath = info.canonicalFilePath();
    if (canonicalPath.isEmpty()) canonicalPath = info.absoluteFilePath();

    // Check if already open
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(i));
        if (editor) {
            QFileInfo curInfo(editor->filePath());
            if (curInfo.canonicalFilePath() == canonicalPath || curInfo.absoluteFilePath() == canonicalPath) {
                m_tabWidget->setCurrentIndex(i);
                editor->setFocus();
                return true;
            }
        }
    }

    // Open file from disk
    QFile file(canonicalPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file:" << canonicalPath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QString content = QString::fromUtf8(data);

    auto* editor = new CodeEditor(m_tabWidget);
    editor->setPlainText(content);
    editor->setFilePath(canonicalPath);
    editor->document()->setModified(false);

    connect(editor, &QPlainTextEdit::textChanged, this, &TabbedCodeEditor::onEditorTextChanged);
    connect(editor, &CodeEditor::lineColChanged, this, &TabbedCodeEditor::onEditorLineColChanged);

    int index = m_tabWidget->addTab(editor, info.fileName());
    m_tabWidget->setTabToolTip(index, canonicalPath);
    m_dirtyTabs.insert(editor, false);

    m_tabWidget->setCurrentIndex(index);
    editor->setFocus();

    updateStatus(editor);
    return true;
}

bool TabbedCodeEditor::saveCurrentFile() {
    return saveFileAt(m_tabWidget->currentIndex());
}

bool TabbedCodeEditor::saveFileAt(int index) {
    if (index < 0 || index >= m_tabWidget->count()) return false;

    auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
    if (!editor) return false;

    QString path = editor->filePath();
    if (path.isEmpty()) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Save Failed", QString("Failed to save file:\n%1").arg(path));
        return false;
    }

    QByteArray data = editor->toPlainText().toUtf8();
    file.write(data);
    file.close();

    editor->document()->setModified(false);
    setTabDirty(index, false);

    emit fileSaved(path);
    emit fileModified(path, false);

    updateStatus(editor);
    return true;
}

bool TabbedCodeEditor::saveAllFiles() {
    bool allSuccess = true;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (isTabDirty(i)) {
            if (!saveFileAt(i)) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

bool TabbedCodeEditor::closeTab(int index) {
    if (index < 0 || index >= m_tabWidget->count()) return false;

    if (isTabDirty(index)) {
        auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
        QString fileName = editor ? QFileInfo(editor->filePath()).fileName() : "document";
        auto res = QMessageBox::question(this, "Unsaved Changes",
            QString("Do you want to save changes to \"%1\" before closing?").arg(fileName),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (res == QMessageBox::Save) {
            if (!saveFileAt(index)) return false;
        } else if (res == QMessageBox::Cancel) {
            return false;
        }
    }

    QWidget* w = m_tabWidget->widget(index);
    m_dirtyTabs.remove(w);
    m_tabWidget->removeTab(index);
    delete w;

    if (m_tabWidget->count() == 0) {
        m_statusLabel->setText("Ready");
        m_filePathLabel->clear();
    }
    return true;
}

void TabbedCodeEditor::gotoLocation(const QString& filePath, int line, int column) {
    if (!openFile(filePath)) return;

    auto* editor = currentEditor();
    if (editor) {
        editor->gotoLine(line, column);
    }
}

void TabbedCodeEditor::onTabCloseRequested(int index) {
    closeTab(index);
}

void TabbedCodeEditor::onCurrentTabChanged(int index) {
    if (index >= 0) {
        auto* editor = qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
        updateStatus(editor);
        emit currentFileChanged(editor ? editor->filePath() : QString());
    } else {
        m_statusLabel->setText("Ready");
        m_filePathLabel->clear();
        emit currentFileChanged(QString());
    }
}

void TabbedCodeEditor::onEditorTextChanged() {
    auto* editor = qobject_cast<CodeEditor*>(sender());
    if (!editor) return;

    int index = m_tabWidget->indexOf(editor);
    if (index >= 0) {
        bool dirty = editor->document()->isModified();
        setTabDirty(index, dirty);
        emit fileModified(editor->filePath(), dirty);
    }
}

void TabbedCodeEditor::onEditorLineColChanged(int line, int col) {
    auto* editor = qobject_cast<CodeEditor*>(sender());
    if (editor && editor == currentEditor()) {
        m_statusLabel->setText(QString("Ln %1, Col %2    |    UTF-8    |    %3")
            .arg(line).arg(col).arg(editor->detectedLanguage()));
    }
}

void TabbedCodeEditor::updateStatus(CodeEditor* editor) {
    if (!editor) {
        m_statusLabel->setText("Ready");
        m_filePathLabel->clear();
        return;
    }

    m_filePathLabel->setText(editor->filePath());

    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    m_statusLabel->setText(QString("Ln %1, Col %2    |    UTF-8    |    %3")
        .arg(line).arg(col).arg(editor->detectedLanguage()));
}

void TabbedCodeEditor::setTabDirty(int index, bool dirty) {
    if (index < 0 || index >= m_tabWidget->count()) return;

    QWidget* w = m_tabWidget->widget(index);
    m_dirtyTabs[w] = dirty;

    auto* editor = qobject_cast<CodeEditor*>(w);
    QString baseName = editor ? QFileInfo(editor->filePath()).fileName() : "document";

    if (dirty) {
        m_tabWidget->setTabText(index, "● " + baseName);
    } else {
        m_tabWidget->setTabText(index, baseName);
    }
}

bool TabbedCodeEditor::isTabDirty(int index) const {
    if (index < 0 || index >= m_tabWidget->count()) return false;
    QWidget* w = m_tabWidget->widget(index);
    return m_dirtyTabs.value(w, false);
}

int TabbedCodeEditor::openFileCount() const {
    return m_tabWidget->count();
}

int TabbedCodeEditor::currentIndex() const {
    return m_tabWidget ? m_tabWidget->currentIndex() : -1;
}

QString TabbedCodeEditor::currentFilePath() const {
    auto* editor = currentEditor();
    return editor ? editor->filePath() : QString();
}

CodeEditor* TabbedCodeEditor::currentEditor() const {
    return qobject_cast<CodeEditor*>(m_tabWidget->currentWidget());
}

CodeEditor* TabbedCodeEditor::editorAt(int index) const {
    return qobject_cast<CodeEditor*>(m_tabWidget->widget(index));
}
