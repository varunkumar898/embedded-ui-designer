#pragma once

#include "UIComponent.h"
#include <QString>
#include <QColor>

class ValueVisualizationComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ValueVisualizationComponent(const QString& id, const QString& compType, QGraphicsItem* parent = nullptr);
    ~ValueVisualizationComponent() override = default;

    // ── Value & Range ────────────────────────────────────────────────────────
    virtual double value() const { return m_value; }
    virtual void setValue(double val);

    double minimum() const { return m_minimum; }
    virtual void setMinimum(double min);

    double maximum() const { return m_maximum; }
    virtual void setMaximum(double max);

    double step() const { return m_step; }
    virtual void setStep(double step);

    QString unit() const { return m_unit; }
    virtual void setUnit(const QString& unit);

    int precision() const { return m_precision; }
    virtual void setPrecision(int prec);

    // Normalized ratio 0.0 to 1.0 based on [minimum, maximum]
    double normalizedValue() const;

    // Formatted strings
    QString formattedValue() const;
    QString formattedValueWithUnit() const;

    // ── Thresholds & Auto-State ──────────────────────────────────────────────
    double warningThreshold() const { return m_warningThreshold; }
    virtual void setWarningThreshold(double thresh);

    double criticalThreshold() const { return m_criticalThreshold; }
    virtual void setCriticalThreshold(double thresh);

    bool useThresholdStates() const { return m_useThresholdStates; }
    virtual void setUseThresholdStates(bool enable);

    // ── Visual Display Properties ────────────────────────────────────────────
    bool showValueText() const { return m_showValueText; }
    virtual void setShowValueText(bool show);

    bool showUnit() const { return m_showUnit; }
    virtual void setShowUnit(bool show);

    bool showTicks() const { return m_showTicks; }
    virtual void setShowTicks(bool show);

    bool showLabels() const { return m_showLabels; }
    virtual void setShowLabels(bool show);

    // ── Colors & Styles ──────────────────────────────────────────────────────
    QColor valueColor() const { return m_valueColor; }
    virtual void setValueColor(const QColor& color);

    QColor trackColor() const { return m_trackColor; }
    virtual void setTrackColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    virtual void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    virtual void setBorderWidth(int width);

    QColor warningColor() const { return m_warningColor; }
    virtual void setWarningColor(const QColor& color);

    QColor criticalColor() const { return m_criticalColor; }
    virtual void setCriticalColor(const QColor& color);

    QColor textColor() const { return m_textColor; }
    virtual void setTextColor(const QColor& color);

    // Compute active color based on threshold or component state
    virtual QColor effectiveValueColor() const;

    // ── Serialization & Styling Overrides ────────────────────────────────────
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    void applyColorStyle(const QString& styleName, const QColor& color) override;

    // ── Data Binding Support ─────────────────────────────────────────────────
    // Updates value from bound DataSource
    virtual void applyBoundValue(const QVariant& val);

protected:
    void evaluateThresholdState();

    double m_value = 50.0;
    double m_minimum = 0.0;
    double m_maximum = 100.0;
    double m_step = 1.0;
    QString m_unit = "";
    int m_precision = 0;

    double m_warningThreshold = 80.0;
    double m_criticalThreshold = 95.0;
    bool m_useThresholdStates = true;

    bool m_showValueText = true;
    bool m_showUnit = true;
    bool m_showTicks = true;
    bool m_showLabels = true;

    QColor m_valueColor = QColor(16, 185, 129);   // Modern Emerald (#10b981)
    QColor m_trackColor = QColor(28, 32, 40);     // Dark Slate (#1c2028)
    QColor m_borderColor = QColor(45, 52, 65);    // Border (#2d3441)
    int m_borderWidth = 1;

    QColor m_warningColor = QColor(245, 158, 11);  // Amber (#f59e0b)
    QColor m_criticalColor = QColor(239, 68, 68);  // Red (#ef4444)
    QColor m_textColor = QColor(241, 245, 249);    // Slate 100 (#f1f5f9)
};
