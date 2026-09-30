#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "Project.h"

class StylesPanel : public QWidget {
    Q_OBJECT

public:
    explicit StylesPanel(QWidget* parent = nullptr);

    void setProject(Project* project);
    Project* project() const { return m_project; }

public slots:
    void refreshStyles();

private slots:
    void onAddStyleClicked();

private:
    Project* m_project = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QWidget* m_listContainer = nullptr;

    void setupUi();
    QWidget* createStyleRow(const ColorStyle& style);
};
