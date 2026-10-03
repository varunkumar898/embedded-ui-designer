#pragma once

#include <QWidget>
#include <QListWidget>
#include <QToolButton>

class ComponentPalette : public QWidget {
    Q_OBJECT

public:
    explicit ComponentPalette(QWidget* parent = nullptr);
    QToolButton* shapeToolButton() const { return m_shapeButton; }

signals:
    void componentDoubleClicked(const QString& compType);
    void shapeToolSelected(const QString& shapeType);
    void saveAsComponentRequested();          ///< User clicked "Save selection as component"
    void customComponentDropped(const QString& definitionId); ///< Drag-drop of a custom component

public slots:
    void refreshCustomComponents(const QStringList& definitionNames);

private:
    QListWidget* m_listWidget = nullptr;
    QListWidget* m_customListWidget = nullptr;
    QToolButton* m_shapeButton = nullptr;
    void setupUi();
};
