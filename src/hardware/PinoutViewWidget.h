#pragma once

#include <QWidget>
#include <QMap>
#include "HardwareModel.h"
#include "PinMuxEngine.h"

namespace Hardware {

class PinoutViewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PinoutViewWidget(QWidget* parent = nullptr);

    void setEngine(PinMuxEngine* engine);
    void setSelectedPin(const QString& pinName);
    QString selectedPin() const { return m_selectedPin; }

signals:
    void pinClicked(const QString& pinName);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    struct PinRect {
        QString pinName;
        QRectF rect;
        int pinNumber = 0;
        bool isHeader = false;
    };

    QColor pinColor(const QString& pinName) const;
    void computeLayout();

    PinMuxEngine* m_engine = nullptr;
    QString m_selectedPin;
    QString m_hoverPin;
    QList<PinRect> m_pinRects;
};

} // namespace Hardware
