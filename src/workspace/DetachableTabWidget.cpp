#include "DetachableTabWidget.h"
#include "theme/Theme.h"

#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDrag>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QStatusBar>
#include <QMenuBar>
#include <QDebug>

// ============================================================================
// DetachableTabBar Implementation
// ============================================================================

DetachableTabBar::DetachableTabBar(QWidget* parent)
    : QTabBar(parent)
{
    setMovable(true);
    setTabsClosable(true);
    setAcceptDrops(true);
}

void DetachableTabBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartIndex = tabAt(event->pos());
        m_dragStartPos = event->pos();
        m_dragInitiated = false;
    }
    QTabBar::mousePressEvent(event);
}

void DetachableTabBar::mouseMoveEvent(QMouseEvent* event) {
    QTabBar::mouseMoveEvent(event);
    if ((event->buttons() & Qt::LeftButton) && m_dragStartIndex >= 0 && !m_dragInitiated) {
        if ((event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()) {
            // Check if cursor moved outside tab bar boundary
            if (!rect().adjusted(-20, -20, 20, 20).contains(event->pos())) {
                m_dragInitiated = true;
                int idx = m_dragStartIndex;
                m_dragStartIndex = -1;
                emit tabDetachRequested(idx, event->globalPosition().toPoint());
            }
        }
    }
}

void DetachableTabBar::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragStartIndex >= 0 && !m_dragInitiated && !rect().contains(event->pos())) {
        int idx = m_dragStartIndex;
        m_dragStartIndex = -1;
        emit tabDetachRequested(idx, event->globalPosition().toPoint());
        return;
    }
    m_dragStartIndex = -1;
    m_dragInitiated = false;
    QTabBar::mouseReleaseEvent(event);
}

void DetachableTabBar::contextMenuEvent(QContextMenuEvent* event) {
    int tabIdx = tabAt(event->pos());
    if (tabIdx < 0) {
        return;
    }

    QMenu menu(this);
    QAction* detachAct = menu.addAction("Detach to Window");
    connect(detachAct, &QAction::triggered, this, [this, tabIdx, event]() {
        emit tabDetachRequested(tabIdx, event->globalPos());
    });

    QMenu* monitorMenu = menu.addMenu("Move to Monitor");
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QString label = QString("Monitor %1: %2x%3")
            .arg(i + 1)
            .arg(screens[i]->geometry().width())
            .arg(screens[i]->geometry().height());
        if (screens[i] == QGuiApplication::primaryScreen()) {
            label += " (Primary)";
        }
        QAction* mAct = monitorMenu->addAction(label);
        connect(mAct, &QAction::triggered, this, [this, tabIdx, scr = screens[i]]() {
            QRect avail = scr->availableGeometry();
            QPoint pos = avail.topLeft() + QPoint(60, 60);
            emit tabDetachRequested(tabIdx, pos);
        });
    }

    if (tabsClosable()) {
        menu.addSeparator();
        QAction* closeAct = menu.addAction("Close Tab");
        connect(closeAct, &QAction::triggered, this, [this, tabIdx]() {
            emit tabCloseRequested(tabIdx);
        });
    }

    menu.exec(event->globalPos());
}

void DetachableTabBar::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-tab")) {
        event->acceptProposedAction();
    } else {
        QTabBar::dragEnterEvent(event);
    }
}

void DetachableTabBar::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-tab")) {
        event->acceptProposedAction();
    } else {
        QTabBar::dropEvent(event);
    }
}

// ============================================================================
// DetachedEditorWindow Implementation
// ============================================================================

DetachedEditorWindow::DetachedEditorWindow(QWidget* contentWidget,
                                           const QString& title,
                                           const QIcon& icon,
                                           DetachableTabWidget* sourceTabWidget,
                                           QWidget* parent)
    : QMainWindow(parent, Qt::Window)
    , m_contentWidget(contentWidget)
    , m_title(title)
    , m_icon(icon)
    , m_sourceTabWidget(sourceTabWidget)
{
    setObjectName(QString("detachedWindow_%1").arg(title));
    setWindowTitle(QString("%1 — Embedded UI Designer").arg(title));
    if (!icon.isNull()) {
        setWindowIcon(icon);
    } else {
        setWindowIcon(QIcon(":/packaging/embedded-ui-designer.png"));
    }

    if (m_contentWidget) {
        setCentralWidget(m_contentWidget);
        m_contentWidget->show();
    }

    setupMenuAndToolbar();
    resize(860, 620);
}

DetachedEditorWindow::~DetachedEditorWindow() {
}

void DetachedEditorWindow::setupMenuAndToolbar() {
    // Menu Bar
    QMenu* winMenu = menuBar()->addMenu("Window");
    QMenu* monitorMenu = winMenu->addMenu("Move to Monitor");
    connect(monitorMenu, &QMenu::aboutToShow, this, [this, monitorMenu]() {
        monitorMenu->clear();
        const auto screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            QString label = QString("Monitor %1: %2x%3")
                .arg(i + 1)
                .arg(screens[i]->geometry().width())
                .arg(screens[i]->geometry().height());
            if (screens[i] == QGuiApplication::primaryScreen()) {
                label += " (Primary)";
            }
            QAction* act = monitorMenu->addAction(label);
            connect(act, &QAction::triggered, this, [this, i]() {
                moveToMonitor(i);
            });
        }
    });

    winMenu->addSeparator();
    QAction* redockAct = winMenu->addAction("Re-dock to Main Window");
    connect(redockAct, &QAction::triggered, this, &DetachedEditorWindow::redock);

    // Toolbar
    QToolBar* tb = addToolBar("Window Controls");
    tb->setMovable(false);
    tb->setStyleSheet(QString(
        "QToolBar { background-color: %1; border-bottom: 1px solid %2; padding: 2px 8px; }"
    ).arg(Theme::BgSurface, Theme::BorderSubtle));

    QLabel* titleLbl = new QLabel(QString("<b>%1</b>").arg(m_title), tb);
    titleLbl->setStyleSheet(QString("color: %1; font-size: 11px; margin-right: 12px;").arg(Theme::TextPrimary));
    tb->addWidget(titleLbl);

    QPushButton* redockBtn = new QPushButton("⤓ Re-dock", tb);
    redockBtn->setToolTip("Re-dock tab back into the main window");
    redockBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 3px 10px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; color: %6; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::Accent, Theme::AccentText));
    connect(redockBtn, &QPushButton::clicked, this, &DetachedEditorWindow::redock);
    tb->addWidget(redockBtn);
}

void DetachedEditorWindow::redock() {
    if (m_isRedocking) return;
    m_isRedocking = true;

    emit redockRequested(this);

    if (m_sourceTabWidget && m_contentWidget) {
        // Detach from central widget before re-docking
        takeCentralWidget();
        m_sourceTabWidget->redockWidget(m_contentWidget, m_title, m_icon, m_originalIndex);
    }
    close();
    deleteLater();
}

void DetachedEditorWindow::moveToMonitor(int screenIndex) {
    const auto screens = QGuiApplication::screens();
    if (screenIndex < 0 || screenIndex >= screens.size()) return;

    QScreen* scr = screens[screenIndex];
    QRect avail = scr->availableGeometry();
    QSize sz = size();
    int x = avail.x() + qMax(0, (avail.width() - sz.width()) / 2);
    int y = avail.y() + qMax(0, (avail.height() - sz.height()) / 2);
    move(x, y);
}

void DetachedEditorWindow::closeEvent(QCloseEvent* event) {
    if (!m_isRedocking && m_sourceTabWidget && m_contentWidget) {
        m_isRedocking = true;
        takeCentralWidget();
        m_sourceTabWidget->redockWidget(m_contentWidget, m_title, m_icon, m_originalIndex);
    }
    QMainWindow::closeEvent(event);
}

void DetachedEditorWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
        emit windowActivated(this);
    }
}

// ============================================================================
// DetachableTabWidget Implementation
// ============================================================================

DetachableTabWidget::DetachableTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    m_tabBar = new DetachableTabBar(this);
    setTabBar(m_tabBar);
    setMovable(true);
    setTabsClosable(true);
    setDocumentMode(true);
    setAcceptDrops(true);

    connect(m_tabBar, &DetachableTabBar::tabDetachRequested, this, &DetachableTabWidget::onTabDetachRequested);
}

DetachableTabWidget::~DetachableTabWidget() {
    redockAll();
}

DetachedEditorWindow* DetachableTabWidget::detachTab(int index, const QPoint& globalPos) {
    if (index < 0 || index >= count()) {
        return nullptr;
    }

    QString title = tabText(index);
    QIcon icon = tabIcon(index);
    QWidget* widget = this->widget(index);

    removeTab(index);
    widget->setParent(nullptr);

    DetachedEditorWindow* win = new DetachedEditorWindow(widget, title, icon, this);
    win->setOriginalIndex(index);
    m_detachedWindows.append(win);

    connect(win, &DetachedEditorWindow::windowActivated, this, [this](DetachedEditorWindow* w) {
        if (w && w->contentWidget()) {
            emit activeEditorChanged(w->contentWidget());
        }
    });

    connect(win, &QObject::destroyed, this, [this, win]() {
        m_detachedWindows.removeAll(win);
    });

    if (!globalPos.isNull()) {
        QRect targetRect(globalPos, QSize(860, 620));
        QRect clamped = clampGeometryToScreens(targetRect);
        win->setGeometry(clamped);
    } else {
        if (window()) {
            QRect targetRect(window()->pos() + QPoint(40, 40), QSize(860, 620));
            QRect clamped = clampGeometryToScreens(targetRect);
            win->setGeometry(clamped);
        }
    }

    win->show();
    win->raise();
    win->activateWindow();

    emit tabDetached(widget, win);
    emit activeEditorChanged(widget);
    return win;
}

bool DetachableTabWidget::redockWidget(QWidget* widget, const QString& title, const QIcon& icon, int preferredIndex) {
    if (!widget) return false;

    for (int i = 0; i < m_detachedWindows.size(); ++i) {
        if (m_detachedWindows[i]->contentWidget() == widget) {
            DetachedEditorWindow* win = m_detachedWindows.takeAt(i);
            win->deleteLater();
            break;
        }
    }

    QString tabTitle = !title.isEmpty() ? title : widget->objectName();
    int insertIdx = (preferredIndex >= 0 && preferredIndex <= count()) ? preferredIndex : count();
    int newIdx = insertTab(insertIdx, widget, icon, tabTitle);
    setCurrentIndex(newIdx);

    emit tabRedocked(widget);
    emit activeEditorChanged(widget);
    return true;
}

void DetachableTabWidget::redockAll() {
    const auto windows = m_detachedWindows;
    for (DetachedEditorWindow* win : windows) {
        if (win && win->contentWidget()) {
            win->redock();
        }
    }
}

void DetachableTabWidget::onTabDetachRequested(int index, const QPoint& globalPos) {
    detachTab(index, globalPos);
}

void DetachableTabWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-tab")) {
        event->acceptProposedAction();
    } else {
        QTabWidget::dragEnterEvent(event);
    }
}

void DetachableTabWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-tab")) {
        event->acceptProposedAction();
    } else {
        QTabWidget::dropEvent(event);
    }
}

QRect DetachableTabWidget::clampGeometryToScreens(const QRect& targetRect, const QRect& fallbackRect) {
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return targetRect.isValid() ? targetRect : fallbackRect;
    }

    // Check if targetRect overlaps with any active screen by at least a visible portion
    for (QScreen* screen : screens) {
        QRect avail = screen->availableGeometry();
        QRect intersection = avail.intersected(targetRect);
        if (intersection.width() >= 50 && intersection.height() >= 30) {
            return targetRect;
        }
    }

    // Target rectangle is completely outside all connected monitors
    // Clamp to primary screen's available geometry
    QScreen* primary = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen() : screens.first();
    QRect primaryAvail = primary->availableGeometry();

    QRect clamped = targetRect;
    if (clamped.width() <= 0 || clamped.width() > primaryAvail.width()) {
        clamped.setWidth(qMin(860, primaryAvail.width() - 40));
    }
    if (clamped.height() <= 0 || clamped.height() > primaryAvail.height()) {
        clamped.setHeight(qMin(620, primaryAvail.height() - 40));
    }

    int x = qBound(primaryAvail.left(), clamped.left(), primaryAvail.right() - clamped.width());
    int y = qBound(primaryAvail.top(), clamped.top(), primaryAvail.bottom() - clamped.height());
    clamped.moveTo(x, y);
    return clamped;
}
