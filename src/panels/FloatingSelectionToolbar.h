#pragma once

#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QMenu>
#include <QList>
#include "UIComponent.h"

class CanvasScene;
class CanvasView;
class QUndoStack;

class FloatingSelectionToolbar : public QWidget {
    Q_OBJECT

public:
    explicit FloatingSelectionToolbar(CanvasView* view, CanvasScene* scene, QUndoStack* undoStack, QWidget* parent = nullptr);
    ~FloatingSelectionToolbar() override = default;

    void updateSelection(const QList<UIComponent*>& selected);
    void reposition();

signals:
    void openEffectsRequested();
    void replaceImageRequested();

private slots:
    void onLockUnlockClicked();
    void onDuplicateClicked();
    void onDeleteClicked();
    void onGroupClicked();
    void onUngroupClicked();
    void onFlipHorizontal();
    void onFlipVertical();
    void onBringToFront();
    void onSendToBack();
    void onBringForward();
    void onSendBackward();

private:
    void setupUi();
    void updateButtons();

    CanvasView* m_view = nullptr;
    CanvasScene* m_scene = nullptr;
    QUndoStack* m_undoStack = nullptr;
    QList<UIComponent*> m_selectedComps;

    QToolButton* m_lockBtn = nullptr;
    QToolButton* m_duplicateBtn = nullptr;
    QToolButton* m_deleteBtn = nullptr;
    QToolButton* m_groupBtn = nullptr;
    QToolButton* m_ungroupBtn = nullptr;
    QToolButton* m_moreBtn = nullptr;
    QMenu* m_moreMenu = nullptr;

    QAction* m_flipHAction = nullptr;
    QAction* m_flipVAction = nullptr;
    QAction* m_bringFrontAction = nullptr;
    QAction* m_sendBackAction = nullptr;
    QAction* m_bringForwardAction = nullptr;
    QAction* m_sendBackwardAction = nullptr;
};
