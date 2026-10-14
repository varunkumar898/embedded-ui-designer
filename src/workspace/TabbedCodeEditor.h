#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QMap>

class CodeEditor;

class TabbedCodeEditor : public QWidget {
    Q_OBJECT

public:
    explicit TabbedCodeEditor(QWidget *parent = nullptr);
    ~TabbedCodeEditor() override;

    bool openFile(const QString& filePath);
    bool saveCurrentFile();
    bool saveFileAt(int index);
    bool saveAllFiles();
    bool closeTab(int index);

    void gotoLocation(const QString& filePath, int line, int column = 1);

    int openFileCount() const;
    int currentIndex() const;
    bool isTabDirty(int index) const;
    QString currentFilePath() const;
    CodeEditor* currentEditor() const;
    CodeEditor* editorAt(int index) const;

signals:
    void fileSaved(const QString& filePath);
    void fileModified(const QString& filePath, bool dirty);
    void currentFileChanged(const QString& filePath);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onEditorTextChanged();
    void onEditorLineColChanged(int line, int col);

private:
    void setupUi();
    void updateStatus(CodeEditor* editor);
    void setTabDirty(int index, bool dirty);

    QTabWidget *m_tabWidget = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_filePathLabel = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_saveAllBtn = nullptr;

    // Track original clean content or dirty flags per tab
    QMap<QWidget*, bool> m_dirtyTabs;
};
