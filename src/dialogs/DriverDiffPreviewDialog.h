#pragma once

#include <QDialog>
#include <QString>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

class DriverDiffPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit DriverDiffPreviewDialog(const QString& protocolName,
                                    const QString& oldCode,
                                    const QString& newCode,
                                    QWidget* parent = nullptr);
    ~DriverDiffPreviewDialog() override = default;

    static QString generateDiff(const QString& oldText, const QString& newText);
    static QString formatDiffHtml(const QString& diffText);

    QTextEdit* diffViewer() const { return m_diffViewer; }
    QPushButton* applyButton() const { return m_btnApply; }
    QPushButton* cancelButton() const { return m_btnCancel; }

private:
    void setupUi(const QString& protocolName, const QString& diffText);

    QTextEdit* m_diffViewer = nullptr;
    QPushButton* m_btnApply = nullptr;
    QPushButton* m_btnCancel = nullptr;
};
