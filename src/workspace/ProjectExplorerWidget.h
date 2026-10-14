#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QPushButton>
#include <QSet>

class ProjectFileDelegate;

class ProjectExplorerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProjectExplorerWidget(QWidget *parent = nullptr);
    ~ProjectExplorerWidget() override;

    void setRootDirectory(const QString& dirPath);
    QString rootDirectory() const { return m_rootDirectory; }
    void applyRootIndex();
    QTreeView* treeView() const { return m_treeView; }
    QFileSystemModel* fileModel() const { return m_fileModel; }

signals:
    void fileActivated(const QString& filePath);
    void syncRequested();
    void firmwareDirectorySelected(const QString& dirPath);
    void directorySelected(const QString& dirPath);

public slots:
    void refresh();
    void onFileDirtyChanged(const QString& filePath, bool dirty);
    void expandAllFolders();
    void collapseAllFolders();

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onNewFile();
    void onNewFolder();
    void onOpenFolder();
    void onDirectoryLoaded(const QString& path);

private:
    void setupUi();

    QString m_rootDirectory;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_pathLabel = nullptr;
    QTreeView *m_treeView = nullptr;
    QFileSystemModel *m_fileModel = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;
    ProjectFileDelegate *m_delegate = nullptr;

    QPushButton *m_newFileBtn = nullptr;
    QPushButton *m_newFolderBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QPushButton *m_syncUiBtn = nullptr;
    QPushButton *m_openFolderBtn = nullptr;

    QSet<QString> m_dirtyFiles;
};
