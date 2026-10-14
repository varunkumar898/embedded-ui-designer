#include "ProblemsWidget.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>

ProblemsWidget::ProblemsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ProblemsWidget::~ProblemsWidget() {
}

void ProblemsWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // Toolbar
    auto* toolBarLayout = new QHBoxLayout();
    m_summaryLabel = new QLabel("No problems detected", this);
    m_summaryLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(Theme::TextSecondary));
    toolBarLayout->addWidget(m_summaryLabel);
    toolBarLayout->addStretch(1);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 8px; font-size: 10px; } "
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
    connect(m_clearBtn, &QPushButton::clicked, this, &ProblemsWidget::clearDiagnostics);
    toolBarLayout->addWidget(m_clearBtn);

    mainLayout->addLayout(toolBarLayout);

    // Diagnostics Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(QStringList() << "Severity" << "File" << "Line" << "Col" << "Message");
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QString(
        "QTableWidget { "
        "  background-color: %1; "
        "  alternate-background-color: %2; "
        "  color: %3; "
        "  border: 1px solid %4; "
        "  border-radius: %5px; "
        "  font-size: 11px; "
        "} "
    ).arg(Theme::BgBase, Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ProblemsWidget::onCellDoubleClicked);

    mainLayout->addWidget(m_table, 1);
}

void ProblemsWidget::addDiagnostic(const CompilerDiagnostic& diag) {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    QString sevText = "Info";
    QColor sevColor = QColor(Theme::Info);
    if (diag.severity == DiagnosticSeverity::Error) {
        sevText = "✖ Error";
        sevColor = QColor(Theme::Danger);
        ++m_errorCount;
    } else if (diag.severity == DiagnosticSeverity::Warning) {
        sevText = "▲ Warning";
        sevColor = QColor(Theme::Warning);
        ++m_warningCount;
    }

    auto* sevItem = new QTableWidgetItem(sevText);
    sevItem->setForeground(sevColor);
    sevItem->setFont(QFont("Arial", 9, QFont::Bold));
    m_table->setItem(row, 0, sevItem);

    auto* fileItem = new QTableWidgetItem(QFileInfo(diag.filePath).fileName());
    fileItem->setToolTip(diag.filePath);
    fileItem->setData(Qt::UserRole, diag.filePath);
    m_table->setItem(row, 1, fileItem);

    m_table->setItem(row, 2, new QTableWidgetItem(QString::number(diag.line)));
    m_table->setItem(row, 3, new QTableWidgetItem(QString::number(diag.column)));

    auto* msgItem = new QTableWidgetItem(diag.message);
    m_table->setItem(row, 4, msgItem);

    m_summaryLabel->setText(QString("%1 Errors, %2 Warnings").arg(m_errorCount).arg(m_warningCount));
    emit countsChanged(m_table->rowCount(), m_errorCount, m_warningCount);
}

void ProblemsWidget::clearDiagnostics() {
    m_table->setRowCount(0);
    m_errorCount = 0;
    m_warningCount = 0;
    m_summaryLabel->setText("No problems detected");
    emit countsChanged(0, 0, 0);
}

int ProblemsWidget::count() const {
    return m_table->rowCount();
}

void ProblemsWidget::onCellDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    auto* fileItem = m_table->item(row, 1);
    auto* lineItem = m_table->item(row, 2);
    auto* colItem = m_table->item(row, 3);
    if (!fileItem) return;

    QString path = fileItem->data(Qt::UserRole).toString();
    int line = lineItem ? lineItem->text().toInt() : 1;
    int colNum = colItem ? colItem->text().toInt() : 1;

    emit problemDoubleClicked(path, line, colNum);
}
