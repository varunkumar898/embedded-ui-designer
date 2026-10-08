#include "SimulationCanvasView.h"
#include "ButtonComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "SliderComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"
#include "ValueVisualizationComponent.h"
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <algorithm>
#include <cmath>

namespace Simulator {

SimulationCanvasView::SimulationCanvasView(SimulationRuntime* runtime, QWidget* parent)
    : QGraphicsView(parent)
    , m_runtime(runtime)
    , m_simScene(new QGraphicsScene(this))
{
    setScene(m_simScene);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setStyleSheet("background-color: #0b0f19; border: 1px solid #1e293b;");

    if (m_runtime) {
        connect(m_runtime, &SimulationRuntime::activeScreenChanged, this, &SimulationCanvasView::refreshScreen);
        connect(m_runtime, &SimulationRuntime::statusChanged, this, &SimulationCanvasView::refreshScreen);
    }

    refreshScreen();
}

SimulationCanvasView::~SimulationCanvasView() {
    if (m_simScene) {
        for (QGraphicsItem* item : m_simScene->items()) {
            m_simScene->removeItem(item);
        }
    }
}

void SimulationCanvasView::refreshScreen() {
    for (QGraphicsItem* item : m_simScene->items()) {
        m_simScene->removeItem(item);
    }
    m_activePressedComponent = nullptr;

    if (!m_runtime) return;
    Screen* scr = m_runtime->activeScreen();
    if (!scr) return;

    int w = scr->width() > 0 ? scr->width() : 480;
    int h = scr->height() > 0 ? scr->height() : 320;
    m_simScene->setSceneRect(0, 0, w, h);

    // Add all components of active screen to simulation scene
    for (UIComponent* comp : scr->components()) {
        if (!comp) continue;
        comp->setSelected(false);
        if (comp->scene() != m_simScene) {
            m_simScene->addItem(comp);
        }
    }

    if (m_fitToWindow) {
        fitInViewport();
    } else {
        setZoomFactor(m_zoomFactor);
    }
}

void SimulationCanvasView::setZoomFactor(qreal factor) {
    m_zoomFactor = std::clamp(factor, 0.25, 4.0);
    m_fitToWindow = false;
    resetTransform();
    scale(m_zoomFactor, m_zoomFactor);
}

void SimulationCanvasView::fitInViewport() {
    m_fitToWindow = true;
    if (m_simScene->sceneRect().isValid()) {
        fitInView(m_simScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void SimulationCanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (m_fitToWindow) {
        fitInViewport();
    }
}

void SimulationCanvasView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor("#0b0f19"));

    QRectF sceneR = m_simScene->sceneRect();
    if (sceneR.isValid()) {
        QColor scrBg = Qt::white;
        if (m_runtime && m_runtime->activeScreen()) {
            scrBg = m_runtime->activeScreen()->backgroundColor();
        }
        painter->fillRect(sceneR, scrBg);

        // Subtle frame around display
        painter->setPen(QPen(QColor("#38bdf8"), 1.5, Qt::SolidLine));
        painter->drawRect(sceneR);
    }
}

UIComponent* SimulationCanvasView::componentAtPoint(const QPointF& scenePos) const {
    QList<QGraphicsItem*> items = m_simScene->items(scenePos);
    for (QGraphicsItem* item : items) {
        auto* comp = dynamic_cast<UIComponent*>(item);
        if (comp && comp->isVisible()) {
            return comp;
        }
    }
    return nullptr;
}

void SimulationCanvasView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        UIComponent* comp = componentAtPoint(scenePos);
        m_activePressedComponent = comp;

        if (comp) {
            QPointF localPos = comp->mapFromScene(scenePos);

            // 1. Switch
            auto* sw = dynamic_cast<SwitchComponent*>(comp);
            if (sw) {
                bool nextChecked = !sw->isChecked();
                sw->setChecked(nextChecked);
                if (m_runtime) {
                    m_runtime->notifyComponentPropertyChanged(sw, "checked", nextChecked);
                    m_runtime->notifyComponentInteraction(sw, "On Change");
                }
            }

            // 2. Checkbox
            auto* cb = dynamic_cast<CheckboxComponent*>(comp);
            if (cb) {
                bool nextChecked = !cb->isChecked();
                cb->setChecked(nextChecked);
                if (m_runtime) {
                    m_runtime->notifyComponentPropertyChanged(cb, "checked", nextChecked);
                    m_runtime->notifyComponentInteraction(cb, "On Change");
                }
            }

            // 3. Button
            auto* btn = dynamic_cast<ButtonComponent*>(comp);
            if (btn) {
                btn->setCurrentState("pressed");
                if (m_runtime) {
                    m_runtime->notifyComponentInteraction(btn, "On Click");
                }
            }

            // 4. Tab View
            auto* tab = dynamic_cast<TabViewComponent*>(comp);
            if (tab) {
                int count = tab->tabs().size();
                if (count > 0 && localPos.y() <= tab->tabHeight()) {
                    int clickedTab = static_cast<int>((localPos.x() / comp->compWidth()) * count);
                    clickedTab = std::clamp(clickedTab, 0, count - 1);
                    tab->setActiveTabIndex(clickedTab);
                    if (m_runtime) {
                        m_runtime->notifyComponentPropertyChanged(tab, "activeTabIndex", clickedTab);
                        m_runtime->notifyComponentInteraction(tab, "On Tab Changed");
                    }
                }
            }

            // 5. Navigation Bar
            auto* nav = dynamic_cast<NavigationBarComponent*>(comp);
            if (nav) {
                int count = nav->items().size();
                if (count > 0) {
                    int clickedItem = static_cast<int>((localPos.x() / comp->compWidth()) * count);
                    clickedItem = std::clamp(clickedItem, 0, count - 1);
                    nav->setSelectedIndex(clickedItem);
                    if (m_runtime) {
                        m_runtime->notifyComponentPropertyChanged(nav, "selectedIndex", clickedItem);
                        m_runtime->notifyComponentInteraction(nav, "On Nav Selected");
                    }
                }
            }

            // 6. List
            auto* list = dynamic_cast<ListComponent*>(comp);
            if (list) {
                int rowH = list->rowHeight();
                if (rowH > 0) {
                    int row = static_cast<int>(localPos.y() / rowH);
                    if (row >= 0 && row < list->items().size()) {
                        list->setSelectedIndex(row);
                        if (m_runtime) {
                            m_runtime->notifyComponentPropertyChanged(list, "selectedIndex", row);
                            m_runtime->notifyComponentInteraction(list, "On Row Selected");
                        }
                    }
                }
            }

            // 7. Table
            auto* table = dynamic_cast<TableComponent*>(comp);
            if (table) {
                int rowH = table->rowHeight();
                int headerH = table->headerHeight();
                if (localPos.y() > headerH && rowH > 0) {
                    int row = static_cast<int>((localPos.y() - headerH) / rowH);
                    if (row >= 0 && row < table->rows().size()) {
                        table->setSelectedRow(row);
                        if (m_runtime) {
                            m_runtime->notifyComponentPropertyChanged(table, "selectedRow", row);
                            m_runtime->notifyComponentInteraction(table, "On Row Selected");
                        }
                    }
                }
            }

            // 8. Slider
            auto* slider = dynamic_cast<SliderComponent*>(comp);
            if (slider) {
                double ratio = std::clamp(localPos.x() / slider->compWidth(), 0.0, 1.0);
                int val = static_cast<int>(slider->minimum() + ratio * (slider->maximum() - slider->minimum()));
                slider->setValue(val);
                if (m_runtime) {
                    m_runtime->notifyComponentPropertyChanged(slider, "value", val);
                    m_runtime->notifyComponentInteraction(slider, "On Value Changed");
                }
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void SimulationCanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (m_activePressedComponent && (event->buttons() & Qt::LeftButton)) {
        QPointF scenePos = mapToScene(event->pos());
        QPointF localPos = m_activePressedComponent->mapFromScene(scenePos);

        auto* slider = dynamic_cast<SliderComponent*>(m_activePressedComponent.data());
        if (slider) {
            double ratio = std::clamp(localPos.x() / slider->compWidth(), 0.0, 1.0);
            int val = static_cast<int>(slider->minimum() + ratio * (slider->maximum() - slider->minimum()));
            slider->setValue(val);
            if (m_runtime) {
                m_runtime->notifyComponentPropertyChanged(slider, "value", val);
                m_runtime->notifyComponentInteraction(slider, "On Value Changed");
            }
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void SimulationCanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_activePressedComponent) {
        auto* btn = dynamic_cast<ButtonComponent*>(m_activePressedComponent.data());
        if (btn && btn->currentState() == "pressed") {
            btn->setCurrentState("normal");
        }
        m_activePressedComponent = nullptr;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void SimulationCanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            setZoomFactor(m_zoomFactor * 1.15);
        } else {
            setZoomFactor(m_zoomFactor / 1.15);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

} // namespace Simulator
