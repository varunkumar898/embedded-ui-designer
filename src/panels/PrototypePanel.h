#pragma once

#include <QWidget>
#include <QJsonArray>

class CanvasScene;
class UIComponent;
class QVBoxLayout;
class QComboBox;

class PrototypePanel : public QWidget {
    Q_OBJECT

public:
    explicit PrototypePanel(CanvasScene* scene, QWidget* parent = nullptr);

public slots:
    void setTargetComponent(UIComponent* component);

private slots:
    void addInteraction();
    void refreshTargets();
    void runInteractions(const QString& trigger);

private:
    CanvasScene* m_scene = nullptr;
    UIComponent* m_source = nullptr;
    QWidget* m_emptyState = nullptr;
    QWidget* m_rowsWidget = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
    QList<QComboBox*> m_targetCombos;

    void rebuildRows();
    void saveRows();
    void removeInteraction(int index);
    void applyInteraction(const QJsonObject& interaction);
};