#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include "CustomComponentDefinition.h"

class ComponentPalette : public QWidget {
    Q_OBJECT

public:
    explicit ComponentPalette(QWidget* parent = nullptr);

    void setCustomComponents(const QList<CustomComponentDefinition>& defs);

signals:
    void componentDoubleClicked(const QString& compType);

private:
    QListWidget* m_listWidget = nullptr;
    QListWidget* m_myComponentsList = nullptr;
    QLabel* m_myComponentsTitle = nullptr;
    QLabel* m_emptyNotice = nullptr;

    void setupUi();
};
