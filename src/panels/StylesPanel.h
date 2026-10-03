#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

class Project;
struct ColorStyle;

/// StylesPanel — "THEME STYLES" dock.
///
/// Lists the project's named color styles and lets the user add, rename,
/// recolor (via ColorPickerDialog), or delete them. Recoloring a style routes
/// through Project::updateColorStyle, which propagates the new color to every
/// canvas component that references the style — live, without a reload.
class StylesPanel : public QWidget {
    Q_OBJECT

public:
    explicit StylesPanel(Project* project = nullptr, QWidget* parent = nullptr);

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
