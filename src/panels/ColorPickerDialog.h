#ifndef COLORPICKERDIALOG_H
#define COLORPICKERDIALOG_H

#include <QDialog>
#include <QColor>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <vector>
#include "ComponentGradient.h"
#include "theme/Theme.h"

class ColorWheelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ColorWheelWidget(QWidget* parent = nullptr);
    void setColor(const QColor& color);
    QColor color() const;
    void setHarmonyColor(const QColor& color);

signals:
    void colorChanged(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateFromPoint(const QPointF& pos);
    void renderWheelPixmap();

    qreal m_hue = 0.52;        // 0..1 (default ~187 deg cyan)
    qreal m_sat = 0.85;        // 0..1
    qreal m_val = 0.90;        // 0..1
    QColor m_harmonyColor = QColor(Theme::Danger);
    qreal m_harmonyHue = 0.02; // 0..1
    QPixmap m_wheelCache;
};

class GradientPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit GradientPreviewWidget(QWidget* parent = nullptr);
    void setGradient(const ComponentGradient& grad);
    ComponentGradient gradient() const { return m_gradient; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ComponentGradient m_gradient;
};

class GradientStopBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit GradientStopBarWidget(QWidget* parent = nullptr);
    void setGradient(const ComponentGradient& grad);
    ComponentGradient gradient() const { return m_gradient; }

    int selectedStopIndex() const { return m_selectedIndex; }
    void setSelectedStopIndex(int idx);

signals:
    void stopSelected(int index);
    void stopMoved(int index, qreal newPos);
    void stopAddRequested(qreal pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    int stopIndexAt(const QPointF& pos) const;

    ComponentGradient m_gradient;
    int m_selectedIndex = 0;
    bool m_isDragging = false;
    int m_draggedIndex = -1;
};

class ColorPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ColorPickerDialog(QWidget* parent = nullptr);
    explicit ColorPickerDialog(const QColor& initialColor, QWidget* parent = nullptr);

    bool isGradientMode() const { return m_isGradientMode; }
    void setGradientMode(bool gradMode);

    QColor selectedColor() const;
    void setSelectedColor(const QColor& c);
    ComponentGradient selectedGradient() const { return m_gradient; }
    void setGradient(const ComponentGradient& grad);

    static QColor getColor(const QColor& initial = QColor(Theme::Accent), QWidget* parent = nullptr, const QString& title = "Choose Color");
    static QString toRgb565Hex(const QColor& c);

    friend class TestFunctionalRunner;
    friend class TestAllCases;

private slots:
    // Mode switcher
    void onModeSolidClicked();
    void onModeGradientClicked();

    // Solid mode slots
    void onWheelColorChanged(const QColor& color);
    void onHexEdited(const QString& text);
    void onHarmonyModeChanged(int index);
    void selectSwatch(int index);

    // Gradient mode slots
    void onGradientTypeChanged(int index);
    void onGradientAngleChanged(int angle);
    void onPresetDirectionClicked(int angle);
    void onRadialCenterChanged();
    void onStopBarSelected(int index);
    void onStopBarMoved(int index, qreal pos);
    void onAddStopClicked();
    void onRemoveStopClicked();
    void onStopPosSpinChanged(int posPercent);
    void onGradientPresetChanged(int index);

private:
    void setupUi();
    void updateHarmonySwatches();
    void updateHexAndRgb565();
    void updateGradientUi();
    void refreshGradientWidgets();

    bool m_isGradientMode = false;
    QColor m_baseColor;
    QColor m_selectedColor;
    ComponentGradient m_gradient;
    int m_harmonyMode = 0;
    int m_activeSwatchIndex = 0;

    struct SwatchData {
        QColor color;
        QString name;
        QString hex;
        QWidget* wrapper = nullptr;
        QPushButton* swatchBtn = nullptr;
        QLabel* pill = nullptr;
    };
    std::vector<SwatchData> m_swatches;

    // Mode Buttons
    QPushButton* m_btnModeSolid = nullptr;
    QPushButton* m_btnModeGradient = nullptr;

    // Central stack
    QStackedWidget* m_modeStack = nullptr;
    QWidget* m_solidContainer = nullptr;
    QWidget* m_gradientContainer = nullptr;

    // Solid controls
    ColorWheelWidget* m_wheelWidget = nullptr;
    QWidget* m_circleSwatch = nullptr;
    QLineEdit* m_hexEdit = nullptr;
    QLabel* m_rgb565Label = nullptr;
    QComboBox* m_harmonyCombo = nullptr;
    QHBoxLayout* m_swatchesBarLayout = nullptr;
    QHBoxLayout* m_swatchesHexLayout = nullptr;

    // Gradient controls
    GradientPreviewWidget* m_gradientPreview = nullptr;
    GradientStopBarWidget* m_gradientStopBar = nullptr;
    QComboBox* m_gradientTypeCombo = nullptr;
    QSlider* m_angleSlider = nullptr;
    QSpinBox* m_angleSpin = nullptr;
    QWidget* m_linearControlsWidget = nullptr;
    QWidget* m_radialControlsWidget = nullptr;
    QSlider* m_radialCenterXSlider = nullptr;
    QSlider* m_radialCenterYSlider = nullptr;
    QSlider* m_radialRadiusSlider = nullptr;
    QSpinBox* m_stopPosSpin = nullptr;
    QWidget* m_stopColorSwatch = nullptr;
    QLineEdit* m_stopHexEdit = nullptr;
    QPushButton* m_btnRemoveStop = nullptr;
    QComboBox* m_gradientPresetCombo = nullptr;
    ColorWheelWidget* m_gradientWheelWidget = nullptr;
};

#endif // COLORPICKERDIALOG_H
