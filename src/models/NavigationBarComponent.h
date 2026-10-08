#pragma once

#include "UIComponent.h"
#include <QList>
#include <QString>
#include <QColor>

struct NavItem {
    QString label;
    QString icon;
    QString targetScreenId;
    bool enabled = true;

    QJsonObject toJson() const {
        QJsonObject o;
        o["label"] = label;
        o["icon"] = icon;
        o["targetScreenId"] = targetScreenId;
        o["enabled"] = enabled;
        return o;
    }

    static NavItem fromJson(const QJsonObject& o) {
        NavItem it;
        it.label = o.value("label").toString("Item");
        it.icon = o.value("icon").toString("");
        it.targetScreenId = o.value("targetScreenId").toString("");
        it.enabled = o.value("enabled").toBool(true);
        return it;
    }
};

class NavigationBarComponent : public UIComponent {
    Q_OBJECT

public:
    explicit NavigationBarComponent(const QString& id = "nav_bar", QGraphicsItem* parent = nullptr);
    ~NavigationBarComponent() override = default;

    QList<NavItem> items() const { return m_items; }
    void setItems(const QList<NavItem>& items);
    void addItem(const QString& label, const QString& targetScreenId = "", const QString& icon = "");
    void removeItem(int index);
    void setItemLabel(int index, const QString& label);
    void setItemTargetScreen(int index, const QString& screenId);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    QString orientation() const { return m_orientation == Qt::Vertical ? "Vertical" : "Horizontal"; }
    void setOrientation(const QString& orient);
    Qt::Orientation orientationEnum() const { return m_orientation; }

    int itemSpacing() const { return m_itemSpacing; }
    void setItemSpacing(int sp);

    QColor barColor() const { return m_barColor; }
    void setBarColor(const QColor& color);

    QColor activeItemColor() const { return m_activeItemColor; }
    void setActiveItemColor(const QColor& color);

    QColor activeTextColor() const { return m_activeTextColor; }
    void setActiveTextColor(const QColor& color);

    QColor inactiveTextColor() const { return m_inactiveTextColor; }
    void setInactiveTextColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

signals:
    void navigateToScreenRequested(const QString& screenId);

protected:
    void paintComponent(QPainter* painter) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QList<NavItem> m_items;
    int m_selectedIndex = 0;
    Qt::Orientation m_orientation = Qt::Horizontal;
    int m_itemSpacing = 4;

    QColor m_barColor = QColor(15, 23, 42);          // Slate-900 (#0f172a)
    QColor m_activeItemColor = QColor(30, 41, 59);    // Slate-800 (#1e293b)
    QColor m_activeTextColor = QColor(56, 189, 248);  // Cyan-400 (#38bdf8)
    QColor m_inactiveTextColor = QColor(148, 163, 184); // Slate-400
    QColor m_borderColor = QColor(51, 65, 85);        // Slate-700
    int m_borderWidth = 1;
};
