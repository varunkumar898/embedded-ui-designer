#include "CanvasView.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "CircleComponent.h"
#include "CustomComponentInstance.h"
#include "CustomComponentDefinition.h"
#include "CreateCustomComponentDialog.h"
#include "Project.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "MoveComponentCommand.h"
#include <QUndoStack>
#include <QMenu>
#include <QMimeData>
#include <QScrollBar>
#include <QDateTime>
#include <algorithm>

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_canvasScene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);
    setAcceptDrops(true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void CanvasView::zoomIn() {
    applyZoom(1.15);
}

void CanvasView::zoomOut() {
    applyZoom(1.0 / 1.15);
}

void CanvasView::resetZoom() {
    resetTransform();
    m_zoomFactor = 1.0;
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::setZoomFactor(qreal factor) {
    factor = std::clamp(factor, 0.25, 4.0);
    resetTransform();
    scale(factor, factor);
    m_zoomFactor = factor;
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::applyZoom(qreal factor) {
    qreal newZoom = m_zoomFactor * factor;
    if (newZoom >= 0.25 && newZoom <= 4.0) {
        scale(factor, factor);
        m_zoomFactor = newZoom;
        emit zoomChanged(m_zoomFactor);
    }
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QList<UIComponent*> selComps;
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (auto comp = dynamic_cast<UIComponent*>(item)) {
                selComps.append(comp);
            }
        }
        if (!selComps.isEmpty()) {
            if (m_undoStack) {
                m_undoStack->push(new DeleteComponentCommand(m_canvasScene, selComps));
            } else {
                for (UIComponent* comp : selComps) {
                    m_canvasScene->removeUIComponent(comp);
                    delete comp;
                }
            }
            event->accept();
            return;
        }
    }

    // Arrow keys nudge: 1px default, 10px with Shift
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
        event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        qreal step = (event->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0;
        qreal dx = 0.0, dy = 0.0;
        if (event->key() == Qt::Key_Left) dx = -step;
        else if (event->key() == Qt::Key_Right) dx = step;
        else if (event->key() == Qt::Key_Up) dy = -step;
        else if (event->key() == Qt::Key_Down) dy = step;

        QList<UIComponent*> selComps;
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (auto comp = dynamic_cast<UIComponent*>(item)) {
                selComps.append(comp);
            }
        }

        if (!selComps.isEmpty()) {
            QList<MoveComponentCommand::MoveEntry> moves;
            for (UIComponent* comp : selComps) {
                QPointF oldPos = comp->pos();
                QPointF newPos = oldPos + QPointF(dx, dy);
                moves.append({comp, oldPos, newPos});
            }
            if (m_undoStack) {
                m_undoStack->push(new MoveComponentCommand(moves));
            } else {
                for (const auto& m : moves) {
                    m.comp->setCompPos(m.newPos.x(), m.newPos.y());
                }
            }
            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragEnterEvent(event);
    }
}

void CanvasView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragMoveEvent(event);
    }
}

void CanvasView::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        QString compType = QString::fromUtf8(event->mimeData()->data("application/x-embedded-ui-component"));
        QPointF scenePos = mapToScene(event->position().toPoint());
        QPointF snapped = m_canvasScene->snapPoint(scenePos);

        UIComponent* comp = createComponentByType(compType, snapped);
        if (comp) {
            scene()->clearSelection();
            if (m_undoStack) {
                m_undoStack->push(new AddComponentCommand(m_canvasScene, comp));
            } else {
                m_canvasScene->addUIComponent(comp);
            }
            comp->setSelected(true);
            emit statusMessageRequested(QString("Added %1 component (%2)").arg(compType, comp->componentId()));
        }
        event->acceptProposedAction();
    } else {
        QGraphicsView::dropEvent(event);
    }
}

void CanvasView::contextMenuEvent(QContextMenuEvent* event) {
    QGraphicsItem* clickedItem = itemAt(event->pos());
    auto clickedComp = dynamic_cast<UIComponent*>(clickedItem);

    // Collect all selected components
    QList<UIComponent*> selectedComps;
    for (auto item : m_canvasScene->selectedItems()) {
        if (auto c = dynamic_cast<UIComponent*>(item)) {
            selectedComps.append(c);
        }
    }
    if (clickedComp && !selectedComps.contains(clickedComp)) {
        clickedComp->setSelected(true);
        selectedComps.append(clickedComp);
    }

    QMenu menu(this);
    if (!selectedComps.isEmpty()) {
        QAction* actCreateCustom = nullptr;
        if (selectedComps.size() >= 1) {
            actCreateCustom = menu.addAction("Create Component from Selection...");
            menu.addSeparator();
        }

        QAction* actDuplicate = menu.addAction("Duplicate");
        QAction* actToFront = menu.addAction("Bring to Front");
        QAction* actToBack = menu.addAction("Send to Back");
        menu.addSeparator();
        QAction* actDelete = menu.addAction("Delete");

        QAction* selected = menu.exec(event->globalPos());
        if (actCreateCustom && selected == actCreateCustom) {
            CreateCustomComponentDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
                for (auto c : selectedComps) {
                    qreal x = c->pos().x();
                    qreal y = c->pos().y();
                    qreal w = c->compWidth();
                    qreal h = c->compHeight();
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    maxX = std::max(maxX, x + w);
                    maxY = std::max(maxY, y + h);
                }
                qreal totalW = maxX - minX;
                qreal totalH = maxY - minY;
                if (totalW < 10) totalW = 100;
                if (totalH < 10) totalH = 30;

                CustomComponentDefinition def;
                def.id = QString("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());
                def.name = dlg.componentName();
                def.behaviorRole = dlg.behaviorRole();
                def.width = totalW;
                def.height = totalH;
                def.minValue = dlg.minValue();
                def.maxValue = dlg.maxValue();
                def.defaultValue = dlg.defaultValue();

                std::sort(selectedComps.begin(), selectedComps.end(), [](UIComponent* a, UIComponent* b) {
                    return a->zValue() < b->zValue();
                });

                for (auto c : selectedComps) {
                    PrimitiveShapeData prim;
                    prim.shapeType = c->componentType();
                    prim.relX = c->pos().x() - minX;
                    prim.relY = c->pos().y() - minY;
                    prim.relWidth = c->compWidth();
                    prim.relHeight = c->compHeight();

                    if (def.behaviorRole == "Slider" || def.behaviorRole == "ProgressBar") {
                        if (prim.shapeType == "Circle" || (prim.relWidth < totalW * 0.45 && prim.relWidth <= prim.relHeight * 1.5)) {
                            prim.role = "thumb";
                        } else {
                            prim.role = "track";
                        }
                    } else {
                        prim.role = "content";
                    }

                    if (auto rect = dynamic_cast<RectangleComponent*>(c)) {
                        prim.fillColor = rect->fillColor();
                        prim.strokeColor = rect->strokeColor();
                        prim.strokeWidth = rect->strokeWidth();
                        prim.cornerRadius = rect->cornerRadius();
                    } else if (auto circ = dynamic_cast<CircleComponent*>(c)) {
                        prim.fillColor = circ->fillColor();
                        prim.strokeColor = circ->strokeColor();
                        prim.strokeWidth = circ->strokeWidth();
                        prim.cornerRadius = prim.relWidth / 2;
                    } else if (auto btn = dynamic_cast<ButtonComponent*>(c)) {
                        prim.fillColor = btn->backgroundColor();
                        prim.strokeColor = btn->textColor();
                        prim.strokeWidth = 1;
                        prim.cornerRadius = btn->cornerRadius();
                    }
                    def.primitives.append(prim);
                }

                if (m_project) {
                    m_project->addCustomComponentDefinition(def);
                }

                for (auto c : selectedComps) {
                    m_canvasScene->removeUIComponent(c);
                    delete c;
                }

                static int instId = 1;
                auto inst = new CustomComponentInstance(QString("custom_%1").arg(instId++));
                inst->setDefinition(def);
                inst->setCompPos(minX, minY);
                inst->setCompSize(totalW, totalH);
                inst->setValue(def.defaultValue);
                m_canvasScene->addUIComponent(inst);
                inst->setSelected(true);

                emit statusMessageRequested(QString("Created custom component '%1'").arg(def.name));
            }
        } else if (selected == actDelete) {
            if (m_undoStack) {
                m_undoStack->push(new DeleteComponentCommand(m_canvasScene, selectedComps));
            } else {
                for (auto c : selectedComps) {
                    m_canvasScene->removeUIComponent(c);
                    delete c;
                }
            }
        } else if (selected == actDuplicate) {
            for (auto comp : selectedComps) {
                QJsonObject json = comp->toJson();
                UIComponent* dup = createComponentByType(comp->componentType(), comp->pos() + QPointF(20, 20));
                if (dup) {
                    dup->fromJson(json);
                    dup->setComponentId(comp->componentId() + "_copy");
                    dup->setCompPos(comp->pos().x() + 20, comp->pos().y() + 20);
                    if (m_undoStack) {
                        m_undoStack->push(new AddComponentCommand(m_canvasScene, dup));
                    } else {
                        m_canvasScene->addUIComponent(dup);
                    }
                    scene()->clearSelection();
                    dup->setSelected(true);
                }
            }
        } else if (selected == actToFront) {
            qreal maxZ = 0.0;
            for (auto c : m_canvasScene->uiComponents()) {
                maxZ = std::max(maxZ, c->zValue());
            }
            for (auto c : selectedComps) {
                c->setZValue(maxZ + 1.0);
            }
        } else if (selected == actToBack) {
            qreal minZ = 0.0;
            for (auto c : m_canvasScene->uiComponents()) {
                minZ = std::min(minZ, c->zValue());
            }
            for (auto c : selectedComps) {
                c->setZValue(minZ - 1.0);
            }
        }
    } else {
        QAction* actZoomReset = menu.addAction("Reset Zoom (100%)");
        QAction* selected = menu.exec(event->globalPos());
        if (selected == actZoomReset) {
            resetZoom();
        }
    }
}

UIComponent* CanvasView::createComponentByType(const QString& compType, const QPointF& pos) {
    static int idCounter = 1;
    QString id = QString("%1_%2").arg(compType.toLower()).arg(idCounter++);

    if (compType.startsWith("custom:")) {
        QString defId = compType.mid(7);
        if (m_project) {
            CustomComponentDefinition def = m_project->findCustomComponentDefinition(defId);
            if (!def.id.isEmpty()) {
                auto inst = new CustomComponentInstance(QString("custom_%1").arg(idCounter++));
                inst->setDefinition(def);
                inst->setPos(pos);
                inst->setCompSize(def.width, def.height);
                inst->setValue(def.defaultValue);
                return inst;
            }
        }
    }

    UIComponent* comp = Project::createComponentInstance(compType, id);
    if (!comp) {
        if (compType == "Text" || compType == "Label") {
            comp = new LabelComponent(id);
        } else {
            comp = new ButtonComponent(id);
        }
    }

    if (comp) {
        comp->setPos(pos);
    }
    return comp;
}
