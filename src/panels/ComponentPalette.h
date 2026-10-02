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

private:
    QListWidget* m_listWidget = nullptr;
    QToolButton* m_shapeButton = nullptr;
    void setupUi();
};
