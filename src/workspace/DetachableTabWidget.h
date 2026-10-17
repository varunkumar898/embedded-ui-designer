#pragma once

#include <QTabBar>
#include <QTabWidget>
#include <QMainWindow>
#include <QList>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QGuiApplication>

class DetachableTabWidget;

/**
 * @brief DetachableTabBar is a QTabBar subclass that detects dragging tabs
 * outside the tab bar boundary to trigger detachment into a standalone window,
 * and handles context menu actions for multi-monitor window management.
 */
class DetachableTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit DetachableTabBar(QWidget* parent = nullptr);

signals:
    void tabDetachRequested(int index, const QPoint& globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    int m_dragStartIndex = -1;
    QPoint m_dragStartPos;
    bool m_dragInitiated = false;
};

/**
 * @brief DetachedEditorWindow is a top-level container window that hosts a
 * detached editor or canvas tab. It includes multi-monitor movement and
 * automatic re-docking back into the originating tab widget.
 */
class DetachedEditorWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DetachedEditorWindow(QWidget* contentWidget,
                                  const QString& title,
                                  const QIcon& icon,
                                  DetachableTabWidget* sourceTabWidget,
                                  QWidget* parent = nullptr);
    ~DetachedEditorWindow() override;

    QWidget* contentWidget() const { return m_contentWidget; }
    QString title() const { return m_title; }
    QIcon icon() const { return m_icon; }
    DetachableTabWidget* sourceTabWidget() const { return m_sourceTabWidget; }

    int originalIndex() const { return m_originalIndex; }
    void setOriginalIndex(int idx) { m_originalIndex = idx; }

public slots:
    void redock();
    void moveToMonitor(int screenIndex);

signals:
    void windowActivated(DetachedEditorWindow* window);
    void redockRequested(DetachedEditorWindow* window);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void setupMenuAndToolbar();

    QWidget* m_contentWidget = nullptr;
    QString m_title;
    QIcon m_icon;
    QPointer<DetachableTabWidget> m_sourceTabWidget;
    int m_originalIndex = -1;
    bool m_isRedocking = false;
};

/**
 * @brief DetachableTabWidget provides a tab container where any tab can be
 * dragged out into a standalone window and re-docked upon drop or closure.
 */
class DetachableTabWidget : public QTabWidget {
    Q_OBJECT
public:
    explicit DetachableTabWidget(QWidget* parent = nullptr);
    ~DetachableTabWidget() override;

    DetachedEditorWindow* detachTab(int index, const QPoint& globalPos = QPoint());
    bool redockWidget(QWidget* widget, const QString& title = QString(), const QIcon& icon = QIcon(), int preferredIndex = -1);
    void redockAll();

    const QList<DetachedEditorWindow*>& detachedWindows() const { return m_detachedWindows; }
    DetachableTabBar* detachableTabBar() const { return m_tabBar; }

    static QRect clampGeometryToScreens(const QRect& targetRect, const QRect& fallbackRect = QRect());

signals:
    void tabDetached(QWidget* widget, DetachedEditorWindow* window);
    void tabRedocked(QWidget* widget);
    void activeEditorChanged(QWidget* activeWidget);

public slots:
    void onTabDetachRequested(int index, const QPoint& globalPos);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    DetachableTabBar* m_tabBar = nullptr;
    QList<DetachedEditorWindow*> m_detachedWindows;
};
