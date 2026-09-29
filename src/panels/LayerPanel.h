#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "CanvasScene.h"

class LayerPanel : public QWidget {
    Q_OBJECT

public:
    explicit LayerPanel(CanvasScene* scene, QWidget* parent = nullptr);

    void refreshLayers();

signals:
    void componentSelected(UIComponent* comp);

private slots:
    void onSelectionChanged();
    void onMoveUp();
    void onMoveDown();
    void onDelete();

    friend class TestFunctionalRunner;

private:
    CanvasScene* m_scene = nullptr;
    QListWidget* m_listWidget = nullptr;
    QPushButton* m_btnUp = nullptr;
    QPushButton* m_btnDown = nullptr;
    QPushButton* m_btnDelete = nullptr;

    bool m_syncing = false;
    void setupUi();
};
