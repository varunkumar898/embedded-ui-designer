#ifndef COLORPICKERDIALOG_H
#define COLORPICKERDIALOG_H

#include <QDialog>
#include <QColor>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <vector>

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
    QColor m_harmonyColor = QColor("#E1341E");
    qreal m_harmonyHue = 0.02; // 0..1
    QPixmap m_wheelCache;
};

class ColorPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ColorPickerDialog(const QColor& initialColor = QColor("#1ECBE1"), QWidget* parent = nullptr);

    QColor selectedColor() const;
    static QColor getColor(const QColor& initial = QColor("#1ECBE1"), QWidget* parent = nullptr, const QString& title = "Choose Color");
    static QString toRgb565Hex(const QColor& c);

    friend class TestFunctionalRunner;

private slots:
    void onWheelColorChanged(const QColor& color);
    void onHexEdited(const QString& text);
    void onHarmonyModeChanged(int index);
    void selectSwatch(int index);

private:
    void setupUi();
    void updateHarmonySwatches();
    void updateHexAndRgb565();

    QColor m_baseColor;
    QColor m_selectedColor;
    int m_harmonyMode = 0; // 0: Complementary, 1: Monochromatic, 2: Analogous, 3: Triadic, 4: Tetradic
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

    ColorWheelWidget* m_wheelWidget = nullptr;
    QWidget* m_circleSwatch = nullptr;
    QLineEdit* m_hexEdit = nullptr;
    QLabel* m_rgb565Label = nullptr;
    QComboBox* m_harmonyCombo = nullptr;
    QHBoxLayout* m_swatchesBarLayout = nullptr;
    QHBoxLayout* m_swatchesHexLayout = nullptr;
};

#endif // COLORPICKERDIALOG_H
