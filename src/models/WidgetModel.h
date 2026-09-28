#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QVariantMap>

/**
 * @brief Core DOM Entity: Represents a single UI component on the target embedded canvas.
 * 
 * Exposes all geometric and visual properties to QML through the Qt Meta-Object System (Q_PROPERTY).
 * Any property mutation triggers the corresponding notify signal for reactive, bi-directional QML bindings.
 */
class WidgetModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString targetScreenId READ targetScreenId WRITE setTargetScreenId NOTIFY targetScreenIdChanged)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)
    Q_PROPERTY(QVariantMap customProperties READ customProperties WRITE setCustomProperties NOTIFY customPropertiesChanged)

public:
    explicit WidgetModel(QObject* parent = nullptr);
    WidgetModel(const QString& id, const QString& type, qreal x, qreal y, qreal width, qreal height, const QColor& color, QObject* parent = nullptr);
    virtual ~WidgetModel() override = default;

    // Getters
    QString id() const { return m_id; }
    QString type() const { return m_type; }
    qreal x() const { return m_x; }
    qreal y() const { return m_y; }
    qreal width() const { return m_width; }
    qreal height() const { return m_height; }
    QColor color() const { return m_color; }
    QString text() const { return m_text; }
    QString targetScreenId() const { return m_targetScreenId; }
    QString imagePath() const { return m_imagePath; }
    QVariantMap customProperties() const { return m_customProperties; }

    // Setters
    void setId(const QString& id);
    void setType(const QString& type);
    void setX(qreal x);
    void setY(qreal y);
    void setWidth(qreal width);
    void setHeight(qreal height);
    void setColor(const QColor& color);
    void setText(const QString& text);
    void setTargetScreenId(const QString& targetScreenId);
    void setImagePath(const QString& path);
    void setCustomProperties(const QVariantMap& props);

    Q_INVOKABLE void setGeometry(qreal x, qreal y, qreal width, qreal height);
    Q_INVOKABLE void setCustomProperty(const QString& key, const QVariant& value);
    Q_INVOKABLE QVariant getCustomProperty(const QString& key, const QVariant& defaultValue = QVariant()) const;

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

    // Deep clone helper
    WidgetModel* clone(QObject* parent = nullptr) const;

signals:
    void idChanged(const QString& id);
    void typeChanged(const QString& type);
    void xChanged(qreal x);
    void yChanged(qreal y);
    void widthChanged(qreal width);
    void heightChanged(qreal height);
    void colorChanged(const QColor& color);
    void textChanged(const QString& text);
    void targetScreenIdChanged(const QString& targetScreenId);
    void imagePathChanged(const QString& path);
    void customPropertiesChanged();
    void geometryChanged();

private:
    QString m_id = "widget";
    QString m_type = "Rectangle"; // "Button", "Label", "Rectangle", "ProgressBar", "Image", etc.
    qreal m_x = 0.0;
    qreal m_y = 0.0;
    qreal m_width = 100.0;
    qreal m_height = 40.0;
    QColor m_color = QColor(33, 150, 243);
    QString m_text;
    QString m_targetScreenId;
    QString m_imagePath;
    QVariantMap m_customProperties;
};
