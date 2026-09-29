#pragma once

#include <QGraphicsObject>
#include <QJsonObject>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>

enum class ResizeHandle {
    None,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Top,
    Bottom,
    Left,
    Right
};

enum class CornerRadiusHandle {
    None,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

class UIComponent : public QGraphicsObject {
    Q_OBJECT

public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    explicit UIComponent(const QString& id, const QString& compType, QGraphicsItem* parent = nullptr);
    virtual ~UIComponent() override = default;

    // Component Identity
    QString componentId() const { return m_id; }
    void setComponentId(const QString& id);

    QString componentType() const { return m_componentType; }

    // Geometry
    qreal compX() const { return pos().x(); }
    qreal compY() const { return pos().y(); }
    qreal compWidth() const { return m_width; }
    qreal compHeight() const { return m_height; }

    void setCompPos(qreal x, qreal y);
    void setCompSize(qreal w, qreal h);

    void recordInitialPosition() { m_initialGeom = QRectF(pos().x(), pos().y(), m_width, m_height); }
    QPointF initialPos() const { return m_initialGeom.topLeft(); }
    QRectF initialGeom() const { return m_initialGeom; }

    // QGraphicsItem Overrides
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    // Serialization
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject& json);

    // Code Generation Hooks
    virtual QString toQmlSnippet(int indentSpaces = 8) const = 0;
    virtual QString toCppSignalSlotStub() const { return QString(); }
    virtual QString toUgfxSnippet(int indentSpaces = 4) const { Q_UNUSED(indentSpaces); return QString(); }

    // Corner Radius interface
    virtual bool hasCornerRadius() const { return false; }
    virtual int cornerRadius() const { return 0; }
    virtual void setCornerRadius(int r) { Q_UNUSED(r); }

    CornerRadiusHandle cornerRadiusHandleAt(const QPointF& pos) const;
    QRectF cornerRadiusHandleRect(CornerRadiusHandle handle) const;
    qreal cornerRadiusHandleOffset() const;

    // Selection & Resize Handle helpers
    static constexpr qreal HANDLE_SIZE = 7.0;

signals:
    void propertyChanged(UIComponent* comp);
    void geometryChangedSignal(UIComponent* comp);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    virtual void paintComponent(QPainter* painter) = 0;
    void paintSelectionHandles(QPainter* painter);

    ResizeHandle handleAt(const QPointF& pos) const;
    QRectF handleRect(ResizeHandle handle) const;
    void updateCursorForHandle(ResizeHandle handle);

    QString m_id;
    QString m_componentType;
    qreal m_width = 120.0;
    qreal m_height = 40.0;

    bool m_resizing = false;
    ResizeHandle m_activeHandle = ResizeHandle::None;
    QPointF m_dragStartPos;
    QRectF m_initialGeom;

    bool m_draggingRadius = false;
    CornerRadiusHandle m_activeRadiusHandle = CornerRadiusHandle::None;
    int m_dragStartRadius = 0;
};
