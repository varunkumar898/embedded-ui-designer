#pragma once

#include <QDialog>
#include <QStringList>
#include <QJsonObject>
#include <QList>
#include "analyzer/ai/AIProvider.h"

class QTextEdit;
class QPushButton;

namespace AI {

class AIProposedChangesDialog : public QDialog {
    Q_OBJECT

public:
    explicit AIProposedChangesDialog(const QList<ToolCall>& proposedCalls, QWidget* parent = nullptr);
    ~AIProposedChangesDialog() override = default;

private:
    void setupUI(const QList<ToolCall>& calls);
    QString formatSummary(const QList<ToolCall>& calls);

    QTextEdit* m_summaryEdit = nullptr;
    QPushButton* m_applyButton = nullptr;
    QPushButton* m_rejectButton = nullptr;
};

} // namespace AI
