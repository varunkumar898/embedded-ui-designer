#pragma once

#include <QGraphicsObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
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
    bool isResizing() const { return m_resizing; }

    bool isComponentVisible() const { return isVisible(); }
    void setComponentVisible(bool visible);

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
    QJsonArray interactions() const { return m_interactions; }
    void setInteractions(const QJsonArray& interactions);

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

    // ── Named Color Style References ──────────────────────────────────────
    /// Bind a color property (e.g. "fillColor") to a named style. Empty name
    /// clears the binding and reverts to a direct color.
    void setColorStyleRef(const QString& propertyKey, const QString& styleName);
    QString colorStyleRef(const QString& propertyKey) const;
    bool hasColorStyleRef(const QString& propertyKey) const;
    void clearColorStyleRef(const QString& propertyKey);
    const QMap<QString, QString>& colorStyleRefs() const { return m_colorStyleRefs; }

    /// Re-apply a style's color to this component if any property references it.
    /// Components override this; the base is a no-op.
    virtual void applyColorStyle(const QString& styleName, const QColor& color);

    // ── Hardware Protocol & Pin Binding ──────────────────────────────────
    QString protocol() const { return m_protocol; }
    void setProtocol(const QString& proto);

    QMap<QString, QString> protocolPins() const { return m_protocolPins; }
    QString protocolPin(const QString& role, const QString& defaultVal = QString()) const { return m_protocolPins.value(role, defaultVal); }
    void setProtocolPin(const QString& role, const QString& pin);
    void setProtocolPins(const QMap<QString, QString>& pins);
    void clearProtocol();

    /// Serialize a color as either a plain hex string (no ref) or a
    /// {"styleRef": "<name>"} object. Deserialization restores both.
    static QJsonValue serializeColor(const QColor& color, const QString& styleRef = QString());
    static void deserializeColor(const QJsonValue& val, QColor& colorOut, QString& styleRefOut);

signals:
    void propertyChanged(UIComponent* comp);
    void geometryChangedSignal(UIComponent* comp);
    void interactionTriggered(const QString& trigger);

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
    QJsonArray m_interactions;
    qreal m_width = 120.0;
    qreal m_height = 40.0;

    bool m_resizing = false;
    ResizeHandle m_activeHandle = ResizeHandle::None;
    QPointF m_dragStartPos;
    QRectF m_initialGeom;

    bool m_draggingRadius = false;
    CornerRadiusHandle m_activeRadiusHandle = CornerRadiusHandle::None;
    int m_dragStartRadius = 0;

    QMap<QString, QString> m_colorStyleRefs;
    QString m_protocol = "None";
    QMap<QString, QString> m_protocolPins;
};
