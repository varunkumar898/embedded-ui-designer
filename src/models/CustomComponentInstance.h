#pragma once

#include "UIComponent.h"
#include "CustomComponentDefinition.h"

class Project;

class CustomComponentInstance : public UIComponent {
    Q_OBJECT

public:
    explicit CustomComponentInstance(const QString& id, const QString& defId = QString(), Project* project = nullptr, QGraphicsItem* parent = nullptr);

    QString definitionId() const { return m_definitionId; }
    void setDefinitionId(const QString& defId);
    void setDefinition(const CustomComponentDefinition& def);

    void setProject(Project* project) { m_project = project; }
    Project* project() const { return m_project; }

    CustomComponentDefinition definition() const;
    QString behaviorRole() const;

    double value() const { return m_value; }
    void setValue(double v);

    double minValue() const { return m_minValue; }
    void setMinValue(double min);

    double maxValue() const { return m_maxValue; }
    void setMaxValue(double max);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString m_definitionId;
    CustomComponentDefinition m_cachedDefinition;
    Project* m_project = nullptr;

    double m_value = 50.0;
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    bool m_isDraggingThumb = false;

    QRectF computeThumbRect(const PrimitiveShapeData& thumbPrim, const PrimitiveShapeData& trackPrim) const;
};
