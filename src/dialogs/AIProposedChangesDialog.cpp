#include "AIProposedChangesDialog.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

namespace AI {

AIProposedChangesDialog::AIProposedChangesDialog(const QList<ToolCall>& proposedCalls, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("AI Proposed Project Changes");
    resize(520, 380);
    setupUI(proposedCalls);
}

void AIProposedChangesDialog::setupUI(const QList<ToolCall>& calls) {
    auto mainLayout = new QVBoxLayout(this);

    auto headerLabel = new QLabel("<b>Review AI Proposed Modifications:</b>", this);
    mainLayout->addWidget(headerLabel);

    m_summaryEdit = new QTextEdit(this);
    m_summaryEdit->setReadOnly(true);
    QFont font("Monospace", 9);
    font.setStyleHint(QFont::TypeWriter);
    m_summaryEdit->setFont(font);
    m_summaryEdit->setPlainText(formatSummary(calls));
    mainLayout->addWidget(m_summaryEdit);

    auto btnRow = new QHBoxLayout();
    btnRow->addStretch();

    m_rejectButton = new QPushButton("Reject Changes", this);
    connect(m_rejectButton, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(m_rejectButton);

    m_applyButton = new QPushButton("Apply Changes", this);
    m_applyButton->setStyleSheet(QString("QPushButton { font-weight: bold; background-color: %1; color: %2; padding: 6px 14px; border-radius: %3px; }").arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusSm)));
    connect(m_applyButton, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(m_applyButton);

    mainLayout->addLayout(btnRow);
}

QString AIProposedChangesDialog::formatSummary(const QList<ToolCall>& calls) {
    QStringList creates;
    QStringList dataSources;
    QStringList bindings;
    QStringList moves;
    QStringList others;

    for (const auto& call : calls) {
        QString name = call.name;
        if (name == "create_component") {
            creates.append(QString("  - %1 (type: %2, id: %3)")
                .arg(call.arguments["type"].toString(),
                     call.arguments["type"].toString(),
                     call.arguments["requestedId"].toString(call.arguments["id"].toString())));
        } else if (name == "create_data_source") {
            dataSources.append(QString("  - %1 (type: %2)")
                .arg(call.arguments["id"].toString(),
                     call.arguments["type"].toString()));
        } else if (name == "create_binding") {
            bindings.append(QString("  - %1.%2 <- %3")
                .arg(call.arguments["component_id"].toString(),
                     call.arguments["property"].toString(),
                     call.arguments["source_id"].toString()));
        } else if (name == "move_component") {
            moves.append(QString("  - %1 to (%2, %3)")
                .arg(call.arguments["id"].toString())
                .arg(call.arguments["x"].toDouble())
                .arg(call.arguments["y"].toDouble()));
        } else {
            others.append(QString("  - %1 (%2)")
                .arg(name, QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact))));
        }
    }

    QString out = "========================================\n";
    out += "AI PROPOSED CHANGES\n";
    out += "========================================\n\n";

    if (!creates.isEmpty()) {
        out += QString("CREATE COMPONENTS (%1):\n%2\n\n").arg(creates.size()).arg(creates.join("\n"));
    }
    if (!dataSources.isEmpty()) {
        out += QString("CREATE DATASOURCES (%1):\n%2\n\n").arg(dataSources.size()).arg(dataSources.join("\n"));
    }
    if (!bindings.isEmpty()) {
        out += QString("CREATE BINDINGS (%1):\n%2\n\n").arg(bindings.size()).arg(bindings.join("\n"));
    }
    if (!moves.isEmpty()) {
        out += QString("MOVE COMPONENTS (%1):\n%2\n\n").arg(moves.size()).arg(moves.join("\n"));
    }
    if (!others.isEmpty()) {
        out += QString("OTHER ACTIONS (%1):\n%2\n\n").arg(others.size()).arg(others.join("\n"));
    }

    return out;
}

} // namespace AI
