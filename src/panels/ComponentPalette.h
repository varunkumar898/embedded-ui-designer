#pragma once

#include <QWidget>
#include <QListWidget>

class ComponentPalette : public QWidget {
    Q_OBJECT

public:
    explicit ComponentPalette(QWidget* parent = nullptr);

signals:
    void componentDoubleClicked(const QString& compType);

private:
    QListWidget* m_listWidget = nullptr;
    void setupUi();
};
