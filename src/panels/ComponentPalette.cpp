#include "ComponentPalette.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

class DraggableListWidget : public QListWidget {
public:
    explicit DraggableListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setDragEnabled(true);
        setViewMode(QListView::ListMode);
        setIconSize(QSize(32, 32));
        setSpacing(4);
        setStyleSheet(
            "QListWidget { background-color: #1a1c23; color: #E0E5EE; border: 1px solid #101217; border-top: 1px solid #0d0f14; border-bottom: 1px solid #303746; border-radius: 6px; font-size: 13px; outline: none; padding: 4px; }"
            "QListWidget::item { "
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #323846, stop:0.04 #3b4252, stop:0.5 #2a2f3a, stop:0.96 #21252e, stop:1 #191c22); "
            "  color: #e2e8f2; "
            "  border-top: 1px solid #4d576a; "
            "  border-left: 1px solid #363c49; "
            "  border-right: 1px solid #20232b; "
            "  border-bottom: 2px solid #101217; "
            "  border-radius: 6px; "
            "  padding: 6px 10px; "
            "  margin: 3px 4px; "
            "  font-weight: 600; "
            "}"
            "QListWidget::item:hover { "
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3f4757, stop:0.04 #4a5467, stop:0.5 #333a48, stop:0.96 #282d38, stop:1 #1f232b); "
            "  color: #ffffff; "
            "  border-top: 1px solid #637189; "
            "  border-left: 1px solid #434b5c; "
            "  border-right: 1px solid #242831; "
            "  border-bottom: 2px solid #121419; "
            "}"
            "QListWidget::item:selected { "
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a96ff, stop:0.05 #0a84ed, stop:0.5 #006ecb, stop:0.96 #0054a0, stop:1 #003d78); "
            "  color: #ffffff; "
            "  border-top: 1px solid #82c6ff; "
            "  border-left: 1px solid #369cff; "
            "  border-right: 1px solid #004b91; "
            "  border-bottom: 2px solid #00264d; "
            "  font-weight: bold; "
            "}"
        );
    }

protected:
    void startDrag(Qt::DropActions supportedActions) override {
        Q_UNUSED(supportedActions);
        QListWidgetItem* item = currentItem();
        if (!item) return;

        QString compType = item->data(Qt::UserRole).toString();
        QMimeData* mimeData = new QMimeData();
        mimeData->setData("application/x-embedded-ui-component", compType.toUtf8());

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);
    }
};

ComponentPalette::ComponentPalette(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

static QIcon createBadgeIcon(const QString& symbol, const QColor& baseColor) {
    const int size = 32;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::TextAntialiasing);

    const qreal s = size;
    const qreal margin = 2.0;
    const qreal r = 6.5;
    const QRectF badgeRect(margin, margin + 0.5, s - margin * 2, s - margin * 2);

    // 1. Soft Drop Shadow
    p.save();
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 3; ++i) {
        QRectF shadowRect(margin, margin + 1.0 + i * 0.7, s - margin * 2, s - margin * 2);
        p.setBrush(QColor(0, 0, 0, 35 - i * 10));
        p.drawRoundedRect(shadowRect, r, r);
    }
    p.restore();

    // 2. Outer Bevel Rim
    QLinearGradient rimGrad(0, badgeRect.top(), 0, badgeRect.bottom());
    rimGrad.setColorAt(0.0, QColor(255, 255, 255, 120));
    rimGrad.setColorAt(0.5, QColor(255, 255, 255, 40));
    rimGrad.setColorAt(1.0, QColor(0, 0, 0, 160));
    p.setPen(QPen(QBrush(rimGrad), 1.2));

    // 3. Gel Body Gradient
    QLinearGradient bodyGrad(0, badgeRect.top(), 0, badgeRect.bottom());
    bodyGrad.setColorAt(0.0, baseColor.lighter(135));
    bodyGrad.setColorAt(0.45, baseColor);
    bodyGrad.setColorAt(0.55, baseColor.darker(110));
    bodyGrad.setColorAt(1.0, baseColor.darker(160));
    p.setBrush(bodyGrad);

    p.drawRoundedRect(badgeRect, r, r);

    // 4. Gloss Reflection (Top Aqua Specular Highlight)
    p.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(badgeRect.adjusted(1, 1, -1, -1), r - 1, r - 1);
    p.setClipPath(clipPath);

    QRectF glossRect(badgeRect.left() + 1, badgeRect.top() + 1, badgeRect.width() - 2, badgeRect.height() * 0.48);
    QLinearGradient glossGrad(0, glossRect.top(), 0, glossRect.bottom());
    glossGrad.setColorAt(0.0, QColor(255, 255, 255, 190));
    glossGrad.setColorAt(0.3, QColor(255, 255, 255, 130));
    glossGrad.setColorAt(0.8, QColor(255, 255, 255, 30));
    glossGrad.setColorAt(1.0, QColor(255, 255, 255, 0));

    QPainterPath glossPath;
    glossPath.addRoundedRect(glossRect, r - 1, r - 1);
    p.setPen(Qt::NoPen);
    p.setBrush(glossGrad);
    p.drawPath(glossPath);

    // 5. Bottom subtle reflected glow
    QRectF botGlow(badgeRect.left() + 2, badgeRect.bottom() - 4, badgeRect.width() - 4, 3);
    QLinearGradient botGrad(0, botGlow.top(), 0, botGlow.bottom());
    botGrad.setColorAt(0.0, QColor(255, 255, 255, 0));
    botGrad.setColorAt(1.0, QColor(255, 255, 255, 60));
    p.setBrush(botGrad);
    p.drawEllipse(botGlow);
    p.restore();

    // 6. Embossed 3D Symbol / Letter
    p.save();
    QFont font = p.font();
    font.setPixelSize(13);
    font.setBold(true);
    p.setFont(font);

    // Shadow
    p.setPen(QColor(0, 0, 0, 160));
    p.drawText(badgeRect.translated(0, 1.2), Qt::AlignCenter, symbol);

    // Highlight
    p.setPen(QColor(255, 255, 255, 70));
    p.drawText(badgeRect.translated(0, -0.6), Qt::AlignCenter, symbol);

    // Front text
    p.setPen(QColor(255, 255, 255));
    p.drawText(badgeRect, Qt::AlignCenter, symbol);
    p.restore();

    return QIcon(pixmap);
}

void ComponentPalette::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 8, 6, 6);
    layout->setSpacing(6);

    QLabel* title = new QLabel("UI Components", this);
    title->setStyleSheet("color: #8fa0b8; font-size: 11px; font-weight: bold; text-transform: uppercase; padding-left: 6px; padding-top: 2px;");
    layout->addWidget(title);

    m_listWidget = new DraggableListWidget(this);

    auto addItem = [this](const QString& name, const QString& type, const QString& symbol, const QColor& color) {
        QListWidgetItem* item = new QListWidgetItem(createBadgeIcon(symbol, color), name, m_listWidget);
        item->setData(Qt::UserRole, type);
        item->setToolTip(QString("Drag onto canvas or double click to insert a %1").arg(name));
    };

    addItem("Button", "Button", "B", QColor(33, 150, 243));
    addItem("Text / Label", "Text", "T", QColor(156, 39, 176));
    addItem("Rectangle", "Rectangle", "R", QColor(255, 152, 0));
    addItem("Progress Bar", "ProgressBar", "%", QColor(76, 175, 80));
    addItem("Slider", "Slider", "—", QColor(0, 150, 136));
    addItem("Switch", "Switch", "⏻", QColor(103, 58, 183));
    addItem("Checkbox", "Checkbox", "☑", QColor(0, 188, 212));
    addItem("Text Input", "TextInput", "⌨", QColor(255, 87, 34));
    addItem("Circle", "Circle", "○", QColor(3, 169, 244));

    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            emit componentDoubleClicked(item->data(Qt::UserRole).toString());
        }
    });

    QLabel* hint = new QLabel("Tip: Drag item onto canvas", this);
    hint->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #14161c, stop:1 #1c2028); "
        "border-top: 1px solid #0f1115; border-bottom: 1px solid #303746; border-radius: 4px; "
        "color: #7d8c9e; font-size: 11px; font-weight: 500; padding: 5px 8px;"
    );
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
}
