#pragma once

#include <QWidget>
#include <QVector>

#include "diagnostics/DiagnosticsService.h"

class QLabel;
class QTableWidget;
class QToolButton;

/**
 * @brief Bottom dock panel listing project diagnostics from DiagnosticsService.
 *
 * Columns: Severity (icon + text), Message, Source, Location (screen / component).
 * Double-clicking a row asks the window to navigate to the component.
 */
class ProblemsPanel : public QWidget {
    Q_OBJECT

public:
    enum class Filter { All, Errors, Warnings, Info };

    explicit ProblemsPanel(Diagnostics::DiagnosticsService* service, QWidget* parent = nullptr);

    Filter filter() const { return m_filter; }
    void setFilter(Filter filter);

    int visibleRowCount() const;
    int countForSeverity(Diagnostics::Diagnostic::Severity severity) const;

signals:
    void navigateRequested(const QString& screenId, const QString& componentId);

public slots:
    void refreshFromService();

private:
    void rebuild();
    void updateFilterButtons();
    void handleDoubleClick(int row, int column);
    QString locationText(const Diagnostics::Diagnostic& diagnostic) const;

    Diagnostics::DiagnosticsService* m_service = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QToolButton* m_filterAllButton = nullptr;
    QToolButton* m_filterErrorsButton = nullptr;
    QToolButton* m_filterWarningsButton = nullptr;
    QToolButton* m_filterInfoButton = nullptr;
    QVector<Diagnostics::Diagnostic> m_visible;
    Filter m_filter = Filter::All;
};
