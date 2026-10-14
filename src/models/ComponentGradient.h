#pragma once

#include <QColor>
#include <QList>
#include <QRectF>
#include <QBrush>
#include <QGradient>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct GradientStop {
    qreal position = 0.0; // 0.0 to 1.0
    QColor color = QColor("#1ECBE1");

    GradientStop() = default;
    GradientStop(qreal pos, const QColor& col) : position(pos), color(col) {}

    bool operator==(const GradientStop& other) const {
        return std::abs(position - other.position) < 0.001 && color == other.color;
    }
};

class ComponentGradient {
public:
    enum class Type {
        Linear,
        Radial
    };

    ComponentGradient() {
        // Default 2-stop cyan to blue linear gradient (90 deg: top-to-bottom)
        stops.append(GradientStop(0.0, QColor("#1ECBE1")));
        stops.append(GradientStop(1.0, QColor("#0284C7")));
    }

    ComponentGradient(Type t, const QList<GradientStop>& s, qreal a = 90.0)
        : type(t), angle(a), stops(s), m_enabled(true) {}

    Type type = Type::Linear;
    qreal angle = 90.0;    // Degrees: 0° L->R, 90° T->B, 180° R->L, 270° B->T
    qreal centerX = 0.5;  // 0.0 to 1.0
    qreal centerY = 0.5;  // 0.0 to 1.0
    qreal radius = 0.5;   // 0.0 to 1.0
    QList<GradientStop> stops;

    bool isEnabled() const { return m_enabled && isValid(); }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    bool isValid() const { return stops.size() >= 2; }

    void addStop(qreal pos, const QColor& col) {
        stops.append(GradientStop(std::clamp(pos, 0.0, 1.0), col));
        sortStops();
    }

    void removeStop(int index) {
        if (stops.size() > 2 && index >= 0 && index < stops.size()) {
            stops.removeAt(index);
        }
    }

    void sortStops() {
        std::sort(stops.begin(), stops.end(), [](const GradientStop& a, const GradientStop& b) {
            return a.position < b.position;
        });
    }

    QColor interpolateAt(qreal pos) const {
        if (stops.isEmpty()) return QColor("#1ECBE1");
        if (stops.size() == 1) return stops.first().color;

        QList<GradientStop> sorted = stops;
        std::sort(sorted.begin(), sorted.end(), [](const GradientStop& a, const GradientStop& b) {
            return a.position < b.position;
        });

        if (pos <= sorted.first().position) return sorted.first().color;
        if (pos >= sorted.last().position) return sorted.last().color;

        for (int i = 0; i < sorted.size() - 1; ++i) {
            if (pos >= sorted[i].position && pos <= sorted[i + 1].position) {
                qreal span = sorted[i + 1].position - sorted[i].position;
                if (span <= 0.0001) return sorted[i].color;
                qreal t = (pos - sorted[i].position) / span;
                const QColor& c1 = sorted[i].color;
                const QColor& c2 = sorted[i + 1].color;
                int r = static_cast<int>(std::round(c1.red() + t * (c2.red() - c1.red())));
                int g = static_cast<int>(std::round(c1.green() + t * (c2.green() - c1.green())));
                int b = static_cast<int>(std::round(c1.blue() + t * (c2.blue() - c1.blue())));
                int a = static_cast<int>(std::round(c1.alpha() + t * (c2.alpha() - c1.alpha())));
                return QColor(r, g, b, a);
            }
        }
        return sorted.last().color;
    }

    QBrush toQBrush(const QRectF& targetRect) const {
        if (targetRect.isEmpty() || !isValid()) {
            return QBrush(stops.isEmpty() ? QColor("#1ECBE1") : stops.first().color);
        }

        if (type == Type::Linear) {
            qreal rad = angle * M_PI / 180.0;
            qreal dx = std::cos(rad) * (targetRect.width() / 2.0);
            qreal dy = std::sin(rad) * (targetRect.height() / 2.0);
            QPointF c = targetRect.center();
            QLinearGradient lg(c.x() - dx, c.y() - dy, c.x() + dx, c.y() + dy);
            for (const auto& s : stops) {
                lg.setColorAt(std::clamp(s.position, 0.0, 1.0), s.color);
            }
            return QBrush(lg);
        } else {
            QPointF c(targetRect.left() + centerX * targetRect.width(),
                      targetRect.top() + centerY * targetRect.height());
            qreal r = std::max(targetRect.width(), targetRect.height()) * radius;
            QRadialGradient rg(c, std::max<qreal>(1.0, r));
            for (const auto& s : stops) {
                rg.setColorAt(std::clamp(s.position, 0.0, 1.0), s.color);
            }
            return QBrush(rg);
        }
    }

    QJsonObject toJson() const {
        QJsonObject json;
        json["enabled"] = m_enabled;
        json["type"] = (type == Type::Linear) ? "linear" : "radial";
        json["angle"] = angle;
        json["centerX"] = centerX;
        json["centerY"] = centerY;
        json["radius"] = radius;

        QJsonArray stopArr;
        for (const auto& s : stops) {
            QJsonObject sObj;
            sObj["position"] = s.position;
            sObj["color"] = s.color.name(QColor::HexArgb);
            stopArr.append(sObj);
        }
        json["stops"] = stopArr;
        return json;
    }

    static ComponentGradient fromJson(const QJsonObject& json) {
        ComponentGradient grad;
        grad.m_enabled = json.value("enabled").toBool(true);
        QString typeStr = json.value("type").toString("linear");
        grad.type = (typeStr == "radial") ? Type::Radial : Type::Linear;
        grad.angle = json.value("angle").toDouble(90.0);
        grad.centerX = json.value("centerX").toDouble(0.5);
        grad.centerY = json.value("centerY").toDouble(0.5);
        grad.radius = json.value("radius").toDouble(0.5);

        if (json.contains("stops") && json.value("stops").isArray()) {
            grad.stops.clear();
            for (const QJsonValue& val : json.value("stops").toArray()) {
                QJsonObject sObj = val.toObject();
                qreal pos = sObj.value("position").toDouble(0.0);
                QColor col(sObj.value("color").toString("#1ECBE1"));
                grad.stops.append(GradientStop(pos, col));
            }
        }
        grad.sortStops();
        return grad;
    }

    // Presets
    static ComponentGradient presetSunset() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#FF512F")}, {1.0, QColor("#DD2476")} };
        return g;
    }

    static ComponentGradient presetOcean() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#2B5876")}, {0.5, QColor("#4E4376")}, {1.0, QColor("#00C6FF")} };
        return g;
    }

    static ComponentGradient presetForest() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#134E5E")}, {1.0, QColor("#71B280")} };
        return g;
    }

    static ComponentGradient presetSky() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#00C6FF")}, {1.0, QColor("#0072FF")} };
        return g;
    }

    static ComponentGradient presetWarmSpectrum() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#F12711")}, {1.0, QColor("#F5AF19")} };
        return g;
    }

    static ComponentGradient presetCoolSpectrum() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#6A11CB")}, {1.0, QColor("#2575FC")} };
        return g;
    }

    static ComponentGradient presetNeonGlow() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 90.0;
        g.stops = { {0.0, QColor("#00F2FE")}, {1.0, QColor("#4FACFE")} };
        return g;
    }

    static ComponentGradient presetCustom() {
        ComponentGradient g;
        g.type = Type::Linear;
        g.angle = 45.0;
        g.stops = {
            {0.0, QColor("#2196F3")},
            {0.35, QColor("#00E5FF")},
            {0.70, QColor("#4CAF50")},
            {1.0, QColor("#FF9800")}
        };
        return g;
    }

private:
    bool m_enabled = false;
};
