#include "DriverDiffPreviewDialog.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>

DriverDiffPreviewDialog::DriverDiffPreviewDialog(const QString& protocolName,
                                                 const QString& oldCode,
                                                 const QString& newCode,
                                                 QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString("Driver Code Diff Preview — %1").arg(protocolName));
    resize(720, 520);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(Theme::BgBase, Theme::TextPrimary));

    QString diff = generateDiff(oldCode, newCode);
    setupUi(protocolName, diff);
}

QString DriverDiffPreviewDialog::generateDiff(const QString& oldText, const QString& newText) {
    QStringList oldLines = oldText.split('\n');
    QStringList newLines = newText.split('\n');

    QString diff;
    diff += "--- a/driver_init.c (Previous)\n";
    diff += "+++ b/driver_init.c (Regenerated)\n";
    diff += "@@ -1 +1 @@\n";

    int maxLines = std::max(oldLines.size(), newLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString oldL = (i < oldLines.size()) ? oldLines[i] : QString();
        QString newL = (i < newLines.size()) ? newLines[i] : QString();

        if (i < oldLines.size() && i < newLines.size()) {
            if (oldL == newL) {
                diff += " " + oldL + "\n";
            } else {
                diff += "-" + oldL + "\n";
                diff += "+" + newL + "\n";
            }
        } else if (i < oldLines.size()) {
            diff += "-" + oldL + "\n";
        } else if (i < newLines.size()) {
            diff += "+" + newL + "\n";
        }
    }

    return diff;
}

QString DriverDiffPreviewDialog::formatDiffHtml(const QString& diffText) {
    QString html = "<pre style=\"font-family: monospace; font-size: 11px; margin: 0; line-height: 1.4;\">";
    const QStringList lines = diffText.split('\n');
    for (const QString& line : lines) {
        QString escaped = line.toHtmlEscaped();
        if (line.startsWith("+++") || line.startsWith("---")) {
            html += QString("<span style=\"color: #94a3b8; font-weight: bold;\">%1</span><br/>").arg(escaped);
        } else if (line.startsWith("@@")) {
            html += QString("<span style=\"color: #38bdf8; font-weight: bold;\">%1</span><br/>").arg(escaped);
        } else if (line.startsWith("+")) {
            html += QString("<span style=\"color: #22c55e; background-color: rgba(34, 197, 94, 0.15); display: block;\">%1</span><br/>").arg(escaped);
        } else if (line.startsWith("-")) {
            html += QString("<span style=\"color: #ef4444; background-color: rgba(239, 68, 68, 0.15); display: block;\">%1</span><br/>").arg(escaped);
        } else {
            html += QString("<span style=\"color: #cbd5e1;\">%1</span><br/>").arg(escaped);
        }
    }
    html += "</pre>";
    return html;
}

void DriverDiffPreviewDialog::setupUi(const QString& protocolName, const QString& diffText) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel(QString("Regenerated Driver Initialization: %1").arg(protocolName), this);
    QFont tf = font();
    tf.setPointSize(12);
    tf.setBold(true);
    titleLabel->setFont(tf);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::TextPrimary));
    rootLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel("Review the generated driver initialization code diff before applying protocol changes to hardware.", this);
    descLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    rootLayout->addWidget(descLabel);

    m_diffViewer = new QTextEdit(this);
    m_diffViewer->setObjectName("driverDiffViewer");
    m_diffViewer->setReadOnly(true);
    m_diffViewer->setStyleSheet(QString(
        "QTextEdit { background-color: %1; border: 1px solid %2; border-radius: %3px; padding: 10px; color: %4; font-family: 'JetBrains Mono', 'Courier New', monospace; }"
    ).arg(Theme::BgRaised, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::TextPrimary));
    m_diffViewer->setHtml(formatDiffHtml(diffText));
    rootLayout->addWidget(m_diffViewer, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setObjectName("btnDiffCancel");
    m_btnCancel->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px 14px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; }"
    ).arg(Theme::BgRaised, Theme::BorderSubtle, Theme::TextSecondary, QString::number(Theme::RadiusSm), Theme::BgOverlay));
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_btnCancel);

    m_btnApply = new QPushButton("Apply Driver Changes", this);
    m_btnApply->setObjectName("btnDiffApply");
    m_btnApply->setDefault(true);
    m_btnApply->setStyleSheet(QString(
        "QPushButton { background-color: #10b981; border: none; color: #ffffff; border-radius: %1px; padding: 6px 16px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: #059669; }"
    ).arg(QString::number(Theme::RadiusSm)));
    connect(m_btnApply, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_btnApply);

    rootLayout->addLayout(btnLayout);
}
