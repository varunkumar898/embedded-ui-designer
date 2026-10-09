#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "analyzer/ResourceAnalyzer.h"

class Project;

class ResourceAnalyzerPanel : public QWidget {
    Q_OBJECT

public:
    explicit ResourceAnalyzerPanel(Project* project, QWidget* parent = nullptr);
    ~ResourceAnalyzerPanel() override = default;

    void setProject(Project* project);

public slots:
    void refreshAnalysis();

private:
    void setupUi();
    void updateUi(const ResourceReport& report);
    void formatProgressBar(QProgressBar* bar, double percent);

    Project* m_project = nullptr;
    ResourceAnalyzer m_analyzer;

    QLabel* m_targetLabel = nullptr;
    QLabel* m_resolutionLabel = nullptr;

    QProgressBar* m_flashBar = nullptr;
    QLabel* m_flashDetailsLabel = nullptr;

    QProgressBar* m_ramBar = nullptr;
    QLabel* m_ramDetailsLabel = nullptr;

    QLabel* m_framebufferDetailsLabel = nullptr;

    QTreeWidget* m_breakdownTree = nullptr;
    QListWidget* m_warningsList = nullptr;
    QPushButton* m_refreshBtn = nullptr;
};
