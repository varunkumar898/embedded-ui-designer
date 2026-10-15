#include "ProblemsPanel.h"

#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "theme/Theme.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStyle>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

using Diagnostics::Diagnostic;

ProblemsPanel::ProblemsPanel(Diagnostics::DiagnosticsService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service) {
    setObjectName("ProblemsPanel");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(6);

    auto makeFilterButton = [this](const QString& objectName) {
        auto* button = new QToolButton(this);
        button->setObjectName(objectName);
        button->setCheckable(true);
        button->setAutoRaise(true);
        return button;
    };

    m_filterAllButton = makeFilterButton("filterAllButton");
    m_filterErrorsButton = makeFilterButton("filterErrorsButton");
    m_filterWarningsButton = makeFilterButton("filterWarningsButton");
    m_filterInfoButton = makeFilterButton("filterInfoButton");

    filterRow->addWidget(m_filterAllButton);
    filterRow->addWidget(m_filterErrorsButton);
    filterRow->addWidget(m_filterWarningsButton);
    filterRow->addWidget(m_filterInfoButton);
    filterRow->addStretch();
    layout->addLayout(filterRow);

    m_emptyLabel = new QLabel("No problems detected.", this);
    m_emptyLabel->setObjectName("problemsEmptyLabel");
    m_emptyLabel->setStyleSheet(QString("color: %1; font-style: italic;").arg(Theme::TextMuted));
    layout->addWidget(m_emptyLabel);

    m_table = new QTableWidget(this);
    m_table->setObjectName("problemsTable");
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({ "Severity", "Message", "Source", "Location" });
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setWordWrap(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(m_table, 1);

    connect(m_filterAllButton, &QToolButton::clicked, this, [this]() { setFilter(Filter::All); });
    connect(m_filterErrorsButton, &QToolButton::clicked, this, [this]() { setFilter(Filter::Errors); });
    connect(m_filterWarningsButton, &QToolButton::clicked, this, [this]() { setFilter(Filter::Warnings); });
    connect(m_filterInfoButton, &QToolButton::clicked, this, [this]() { setFilter(Filter::Info); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ProblemsPanel::handleDoubleClick);

    if (m_service) {
        connect(m_service, &Diagnostics::DiagnosticsService::diagnosticsChanged,
                this, &ProblemsPanel::refreshFromService);
        refreshFromService();
    } else {
        updateFilterButtons();
        rebuild();
    }
}

void ProblemsPanel::setFilter(Filter filter) {
    m_filter = filter;
    updateFilterButtons();
    rebuild();
}

int ProblemsPanel::visibleRowCount() const {
    return m_visible.size();
}

int ProblemsPanel::countForSeverity(Diagnostic::Severity severity) const {
    if (!m_service) return 0;
    int count = 0;
    for (const auto& diagnostic : m_service->diagnostics()) {
        if (diagnostic.severity == severity) ++count;
    }
    return count;
}

void ProblemsPanel::refreshFromService() {
    updateFilterButtons();
    rebuild();
}

void ProblemsPanel::updateFilterButtons() {
    if (m_service) {
        m_filterAllButton->setText(QString("All (%1)").arg(m_service->totalCount()));
        m_filterErrorsButton->setText(QString("Errors (%1)").arg(countForSeverity(Diagnostic::Severity::Error)));
        m_filterWarningsButton->setText(QString("Warnings (%1)").arg(countForSeverity(Diagnostic::Severity::Warning)));
        m_filterInfoButton->setText(QString("Info (%1)").arg(countForSeverity(Diagnostic::Severity::Info)));
    } else {
        m_filterAllButton->setText("All (0)");
        m_filterErrorsButton->setText("Errors (0)");
        m_filterWarningsButton->setText("Warnings (0)");
        m_filterInfoButton->setText("Info (0)");
    }
    m_filterAllButton->setChecked(m_filter == Filter::All);
    m_filterErrorsButton->setChecked(m_filter == Filter::Errors);
    m_filterWarningsButton->setChecked(m_filter == Filter::Warnings);
    m_filterInfoButton->setChecked(m_filter == Filter::Info);
}

void ProblemsPanel::rebuild() {
    m_table->setRowCount(0);
    m_visible.clear();

    const QVector<Diagnostic>& diagnostics = m_service
        ? m_service->diagnostics()
        : QVector<Diagnostic>();

    for (const auto& diagnostic : diagnostics) {
        switch (m_filter) {
            case Filter::Errors:
                if (diagnostic.severity != Diagnostic::Severity::Error) continue;
                break;
            case Filter::Warnings:
                if (diagnostic.severity != Diagnostic::Severity::Warning) continue;
                break;
            case Filter::Info:
                if (diagnostic.severity != Diagnostic::Severity::Info) continue;
                break;
            case Filter::All:
                break;
        }
        m_visible.append(diagnostic);
    }

    m_emptyLabel->setVisible(m_visible.isEmpty());
    m_table->setVisible(!m_visible.isEmpty());

    for (const auto& diagnostic : m_visible) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        QString color = Theme::Info;
        QString background = Theme::InfoBg;
        QStyle::StandardPixmap icon = QStyle::SP_MessageBoxInformation;
        if (diagnostic.severity == Diagnostic::Severity::Error) {
            color = Theme::Danger;
            background = Theme::DangerBg;
            icon = QStyle::SP_MessageBoxCritical;
        } else if (diagnostic.severity == Diagnostic::Severity::Warning) {
            color = Theme::Warning;
            background = Theme::WarningBg;
            icon = QStyle::SP_MessageBoxWarning;
        }

        auto* severityItem = new QTableWidgetItem(
            QApplication::style()->standardIcon(icon),
            Diagnostics::DiagnosticsService::severityLabel(diagnostic.severity));
        severityItem->setForeground(QColor(color));
        severityItem->setBackground(QColor(background));
        m_table->setItem(row, 0, severityItem);

        auto* messageItem = new QTableWidgetItem(diagnostic.message);
        messageItem->setToolTip(diagnostic.message);
        m_table->setItem(row, 1, messageItem);

        auto* sourceItem = new QTableWidgetItem(diagnostic.source);
        sourceItem->setForeground(QColor(Theme::TextSecondary));
        m_table->setItem(row, 2, sourceItem);

        auto* locationItem = new QTableWidgetItem(locationText(diagnostic));
        locationItem->setForeground(QColor(Theme::TextMuted));
        m_table->setItem(row, 3, locationItem);
    }
}

void ProblemsPanel::handleDoubleClick(int row, int column) {
    Q_UNUSED(column);
    if (row < 0 || row >= m_visible.size()) return;

    const Diagnostic& diagnostic = m_visible.at(row);
    if (diagnostic.componentId.isEmpty()) return;

    emit navigateRequested(diagnostic.screenId, diagnostic.componentId);
}

QString ProblemsPanel::locationText(const Diagnostic& diagnostic) const {
    QString screenName = diagnostic.screenId;
    const QString componentText = diagnostic.componentId;
    const Project* project = m_service ? m_service->project() : nullptr;

    if (project) {
        if (Screen* screen = project->findScreen(diagnostic.screenId)) {
            screenName = screen->name();
        }
        if (!componentText.isEmpty() && screenName.isEmpty()) {
            for (Screen* screen : project->screens()) {
                if (screen && screen->findComponentById(componentText)) {
                    screenName = screen->name();
                    break;
                }
            }
        }
    }

    if (screenName.isEmpty() && componentText.isEmpty()) return QString();
    if (componentText.isEmpty()) return screenName;
    if (screenName.isEmpty()) return componentText;
    return screenName + " / " + componentText;
}
