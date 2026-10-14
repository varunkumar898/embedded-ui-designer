#pragma once

#include "UIComponent.h"
#include <QList>

class GroupComponent : public UIComponent {
    Q_OBJECT

public:
    explicit GroupComponent(const QString& id = "group", QGraphicsItem* parent = nullptr);
    ~GroupComponent() override;

    void addChild(UIComponent* child);
    void removeChild(UIComponent* child);
    QList<UIComponent*> children() const { return m_children; }
    void clearChildren();

    void recalculateBounds();

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QList<UIComponent*> m_children;
};
