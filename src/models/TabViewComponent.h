#pragma once

#include "UIComponent.h"
#include <QStringList>
#include <QColor>

class TabViewComponent : public UIComponent {
    Q_OBJECT

public:
    explicit TabViewComponent(const QString& id = "tab_view", QGraphicsItem* parent = nullptr);
    ~TabViewComponent() override = default;

    QStringList tabs() const { return m_tabs; }
    void setTabs(const QStringList& tabs);
    void addTab(const QString& title);
    void removeTab(int index);
    void setTabTitle(int index, const QString& title);

    int activeTabIndex() const { return m_activeTabIndex; }
    void setActiveTabIndex(int index);

    int tabHeight() const { return m_tabHeight; }
    void setTabHeight(int h);

    QString tabPosition() const { return m_tabPosition; }
    void setTabPosition(const QString& pos); // "Top" or "Bottom"

    QColor tabColor() const { return m_tabColor; }
    void setTabColor(const QColor& color);

    QColor activeTabColor() const { return m_activeTabColor; }
    void setActiveTabColor(const QColor& color);

    QColor activeTabTextColor() const { return m_activeTabTextColor; }
    void setActiveTabTextColor(const QColor& color);

    QColor inactiveTabTextColor() const { return m_inactiveTabTextColor; }
    void setInactiveTabTextColor(const QColor& color);

    QColor contentAreaColor() const { return m_contentAreaColor; }
    void setContentAreaColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QStringList m_tabs = {"General", "Display", "Network"};
    int m_activeTabIndex = 0;
    int m_tabHeight = 30;
    QString m_tabPosition = "Top"; // "Top", "Bottom"

    QColor m_tabColor = QColor(24, 28, 36);            // Dark slate (#181c24)
    QColor m_activeTabColor = QColor(37, 99, 235);      // Blue-600 (#2563eb)
    QColor m_activeTabTextColor = QColor(255, 255, 255);
    QColor m_inactiveTabTextColor = QColor(148, 163, 184); // Slate-400
    QColor m_contentAreaColor = QColor(15, 23, 42);    // Slate-900 (#0f172a)
    QColor m_borderColor = QColor(51, 65, 85);          // Slate-700
    int m_borderWidth = 1;
};
