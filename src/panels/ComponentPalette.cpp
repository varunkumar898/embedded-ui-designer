#include "ComponentPalette.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QIcon>
#include <QPainter>

class DraggableListWidget : public QListWidget {
public:
    explicit DraggableListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setDragEnabled(true);
        setViewMode(QListView::ListMode);
        setIconSize(QSize(28, 28));
        setSpacing(4);
        setStyleSheet(
            "QListWidget { background-color: #252830; color: #E0E5EE; border: none; font-size: 13px; outline: none; }"
            "QListWidget::item { padding: 8px 12px; border-radius: 6px; margin: 2px 6px; background-color: #2F333E; }"
            "QListWidget::item:hover { background-color: #3B404E; color: #FFFFFF; }"
            "QListWidget::item:selected { background-color: #2196F3; color: #FFFFFF; font-weight: bold; }"
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

static QIcon createBadgeIcon(const QString& symbol, const QColor& color) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(2, 2, 28, 28, 6, 6);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRectF(0, 0, 32, 32), Qt::AlignCenter, symbol);
    return QIcon(pixmap);
}

void ComponentPalette::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(6);

    QLabel* title = new QLabel("UI Components", this);
    title->setStyleSheet("color: #9AA5B8; font-size: 11px; font-weight: bold; text-transform: uppercase; padding-left: 8px;");
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
    hint->setStyleSheet("color: #717C8F; font-size: 11px; padding: 4px 8px;");
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
}
