#include "ComponentPalette.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QToolButton>

class DraggableListWidget : public QListWidget {
public:
    explicit DraggableListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setDragEnabled(true);
        setViewMode(QListView::ListMode);
        setIconSize(QSize(28, 28));
        setSpacing(4);
        setStyleSheet(
            "QListWidget { background-color: transparent; color: #e4ecf7; border: none; outline: none; padding: 0px; }"
            "QListWidget::item { "
            "  background: #1c1f26; "
            "  color: #e4ecf7; "
            "  border: 1px solid #252933; "
            "  border-radius: 10px; "
            "  padding: 7px 12px; "
            "  margin: 3px 2px; "
            "  font-size: 12.5px; "
            "  font-weight: 500; "
            "}"
            "QListWidget::item:hover { "
            "  background: #232731; "
            "  border-color: #323746; "
            "  color: #ffffff; "
            "}"
            "QListWidget::item:selected { "
            "  background: #1a73e8; "
            "  border-color: #4285f4; "
            "  color: #ffffff; "
            "  font-weight: 600; "
            "}"
        );
    }

protected:
    void startDrag(Qt::DropActions supportedActions) override {
        Q_UNUSED(supportedActions);
        QListWidgetItem* item = currentItem();
        if (!item) return;

        QString compType = item->data(Qt::UserRole).toString();
        if (compType == "Shape") return;
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

static QIcon createBadgeIcon(const QString& type, const QString& symbol, const QColor& baseColor) {
    const int size = 28;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::TextAntialiasing);

    const qreal s = size;
    const qreal margin = 1.0;
    const qreal r = 6.0;
    const QRectF badgeRect(margin, margin, s - margin * 2, s - margin * 2);

    // Subtle base gradient
    QLinearGradient bodyGrad(0, badgeRect.top(), 0, badgeRect.bottom());
    bodyGrad.setColorAt(0.0, baseColor.lighter(112));
    bodyGrad.setColorAt(0.5, baseColor);
    bodyGrad.setColorAt(1.0, baseColor.darker(120));
    p.setBrush(bodyGrad);
    p.setPen(QPen(QColor(255, 255, 255, 30), 0.8));
    p.drawRoundedRect(badgeRect, r, r);

    // Subtle specular highlight line on top edge
    p.setPen(QPen(QColor(255, 255, 255, 80), 1.0));
    p.drawLine(QPointF(badgeRect.left() + 4, badgeRect.top() + 1.0),
               QPointF(badgeRect.right() - 4, badgeRect.top() + 1.0));

    if (type == "Slider") {
        // Horizontal bar with vertical tick knob
        p.setPen(QPen(QColor(255, 255, 255), 1.8, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(badgeRect.left() + 5.5, badgeRect.center().y()),
                   QPointF(badgeRect.right() - 5.5, badgeRect.center().y()));
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        QRectF knob(badgeRect.center().x() - 2.5, badgeRect.center().y() - 4.5, 5.0, 9.0);
        p.drawRoundedRect(knob, 1.5, 1.5);
    } else if (type == "Checkbox") {
        // Clean checkmark
        QPainterPath path;
        path.moveTo(badgeRect.left() + 7, badgeRect.center().y());
        path.lineTo(badgeRect.left() + 11.5, badgeRect.bottom() - 7.5);
        path.lineTo(badgeRect.right() - 6.5, badgeRect.top() + 7.5);
        p.strokePath(path, QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else if (type == "Circle") {
        p.setPen(QPen(Qt::white, 2.4));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(badgeRect.center(), 5.5, 5.5);
    } else if (type == "TextInput") {
        // TI with I-beam style
        QFont font = p.font();
        font.setPixelSize(11);
        font.setBold(true);
        p.setFont(font);
        p.setPen(QColor(255, 255, 255));
        p.drawText(badgeRect.translated(-3, 0), Qt::AlignCenter, "T");
        p.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(badgeRect.center().x() + 4, badgeRect.top() + 8),
                   QPointF(badgeRect.center().x() + 4, badgeRect.bottom() - 8));
        p.drawLine(QPointF(badgeRect.left() + 6, badgeRect.bottom() - 5),
                   QPointF(badgeRect.right() - 6, badgeRect.bottom() - 5));
    } else if (type == "Switch") {
        // Power icon
        p.setPen(QPen(Qt::white, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        QRectF arcRect(badgeRect.center().x() - 4.5, badgeRect.center().y() - 4.0, 9.0, 9.0);
        p.drawArc(arcRect, 120 * 16, -240 * 16);
        p.drawLine(QPointF(badgeRect.center().x(), badgeRect.center().y() - 5.5),
                   QPointF(badgeRect.center().x(), badgeRect.center().y() - 0.5));
    } else {
        // Letter / Symbol
        p.save();
        QFont font = p.font();
        font.setPixelSize(12);
        font.setBold(true);
        p.setFont(font);

        p.setPen(QColor(0, 0, 0, 120));
        p.drawText(badgeRect.translated(0, 1.0), Qt::AlignCenter, symbol);

        p.setPen(QColor(255, 255, 255));
        p.drawText(badgeRect, Qt::AlignCenter, symbol);
        p.restore();
    }

    return QIcon(pixmap);
}

void ComponentPalette::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    QLabel* title = new QLabel("UI COMPONENTS", this);
    title->setStyleSheet("color: #5a6475; font-size: 10px; font-weight: bold; letter-spacing: 0.5px; padding-left: 8px; padding-top: 6px; padding-bottom: 2px;");
    layout->addWidget(title);

    m_listWidget = new DraggableListWidget(this);

    auto addItem = [this](const QString& name, const QString& type, const QString& symbol, const QColor& color) {
        QListWidgetItem* item = new QListWidgetItem(createBadgeIcon(type, symbol, color), name, m_listWidget);
        item->setData(Qt::UserRole, type);
        item->setToolTip(QString("Drag onto canvas or double click to insert a %1").arg(name));
    };

    addItem("Button", "Button", "B", QColor(26, 115, 232));
    addItem("Text / Label", "Text", "T", QColor(156, 39, 176));
    addItem("Rectangle", "Rectangle", "R", QColor(230, 81, 0));

    QListWidgetItem* shapeItem = new QListWidgetItem(m_listWidget);
    shapeItem->setData(Qt::UserRole, "Shape");
    shapeItem->setFlags(Qt::ItemIsEnabled);
    shapeItem->setSizeHint(QSize(180, 46));
    m_shapeButton = new QToolButton(m_listWidget);
    m_shapeButton->setObjectName("shapeToolButton");
    m_shapeButton->setText("Shape");
    m_shapeButton->setIcon(createBadgeIcon("Shape", "S", QColor(0, 137, 123)));
    m_shapeButton->setIconSize(QSize(28, 28));
    m_shapeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_shapeButton->setPopupMode(QToolButton::InstantPopup);
    m_shapeButton->setCursor(Qt::PointingHandCursor);
    m_shapeButton->setToolTip("Choose a shape drawing tool");
    m_shapeButton->setStyleSheet(
        "QToolButton { background: #1c1f26; color: #e4ecf7; border: 1px solid #252933; "
        "border-radius: 10px; padding: 6px 10px; text-align: left; font-size: 12.5px; font-weight: 500; }"
        "QToolButton:hover { background: #232731; border-color: #323746; color: #ffffff; }"
        "QToolButton::menu-indicator { image: url(:/combo_arrow.png); subcontrol-origin: padding; "
        "subcontrol-position: top right; width: 26px; height: 7px; }"
    );
    QMenu* shapeMenu = new QMenu(m_shapeButton);
    const QStringList shapes = {"Circle", "Triangle", "Square", "Rectangle", "Custom"};
    for (const QString& shape : shapes) {
        QAction* action = shapeMenu->addAction(shape);
        connect(action, &QAction::triggered, this, [this, shape]() {
            emit shapeToolSelected(shape);
        });
    }
    m_shapeButton->setMenu(shapeMenu);
    m_listWidget->setItemWidget(shapeItem, m_shapeButton);

    addItem("Progress Bar", "ProgressBar", "%", QColor(46, 125, 50));
    addItem("Slider", "Slider", "—", QColor(0, 137, 123));
    addItem("Switch", "Switch", "⏻", QColor(123, 31, 162));
    addItem("Checkbox", "Checkbox", "✓", QColor(0, 168, 120));
    addItem("Text Input", "TextInput", "TI", QColor(229, 57, 53));
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item && item->data(Qt::UserRole).toString() != "Shape") {
            emit componentDoubleClicked(item->data(Qt::UserRole).toString());
        }
    });

    // ── My Components section ──────────────────────────────────────────────
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color:#252a35; margin: 6px 0;");
    layout->addWidget(separator);

    QWidget* myCompHeader = new QWidget(this);
    QHBoxLayout* myCompHeaderLayout = new QHBoxLayout(myCompHeader);
    myCompHeaderLayout->setContentsMargins(6, 0, 6, 0);
    myCompHeaderLayout->setSpacing(4);

    QLabel* myCompTitle = new QLabel("MY COMPONENTS", this);
    myCompTitle->setStyleSheet("color:#5a6475;font-size:10px;font-weight:bold;letter-spacing:0.5px;");
    myCompHeaderLayout->addWidget(myCompTitle, 1);

    QPushButton* saveAsBtn = new QPushButton("+ Save", this);
    saveAsBtn->setToolTip("Save selected canvas component as a reusable component");
    saveAsBtn->setCursor(Qt::PointingHandCursor);
    saveAsBtn->setStyleSheet(
        "QPushButton { background:#1c2a1e; color:#34a853; border:1px solid #2e4a32; "
        "border-radius:5px; padding:2px 8px; font-size:10px; font-weight:bold; }"
        "QPushButton:hover { background:#243628; }"
    );
    connect(saveAsBtn, &QPushButton::clicked, this, &ComponentPalette::saveAsComponentRequested);
    myCompHeaderLayout->addWidget(saveAsBtn);
    layout->addWidget(myCompHeader);

    m_customListWidget = new DraggableListWidget(this);
    layout->addWidget(m_customListWidget);
    connect(m_customListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) emit componentDoubleClicked(item->data(Qt::UserRole).toString());
    });

    QLabel* hint = new QLabel("Tip: Drag item onto canvas", this);
    hint->setStyleSheet("color:#566070;font-size:11px;padding:10px 4px 6px 4px;");
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
}

void ComponentPalette::refreshCustomComponents(const QStringList& definitionNames) {
    if (!m_customListWidget) return;
    m_customListWidget->clear();
    for (const QString& name : definitionNames) {
        QListWidgetItem* item = new QListWidgetItem(
            createBadgeIcon("CustomInstance", "C", QColor(100, 60, 200)),
            name, m_customListWidget);
        item->setData(Qt::UserRole, QString("CustomInstance::%1").arg(name));
        item->setToolTip(QString("Double-click or drag to place \"%1\"").arg(name));
    }
    if (definitionNames.isEmpty()) {
        QListWidgetItem* placeholder = new QListWidgetItem("No custom components yet", m_customListWidget);
        placeholder->setFlags(Qt::NoItemFlags);
        placeholder->setForeground(QColor("#4a5568"));
    }
}
