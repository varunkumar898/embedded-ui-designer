#include "ProjectExplorerWidget.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QClipboard>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileIconProvider>
#include <QTimer>

class FirmwareFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit FirmwareFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setRootDirectory(const QString& rootDir) {
        m_rootDir = QDir::cleanPath(rootDir);
        invalidate();
    }

    QString rootDirectory() const { return m_rootDir; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (m_rootDir.isEmpty()) return true;
        auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel) return true;

        QModelIndex srcIndex = fsModel->index(sourceRow, 0, sourceParent);
        QString itemPath = QDir::cleanPath(fsModel->filePath(srcIndex));

        // Accept the root directory itself
        if (itemPath == m_rootDir) return true;

        // Accept any descendant of the root directory
        if (itemPath.startsWith(m_rootDir + "/") || itemPath.startsWith(m_rootDir + "\\")) return true;

        // For ancestors of m_rootDir, accept so that the tree can display m_rootDir
        if (itemPath.isEmpty() || itemPath == "/" ||
            m_rootDir.startsWith(itemPath + "/") || m_rootDir.startsWith(itemPath + "\\")) {
            return true;
        }

        return false;
    }

private:
    QString m_rootDir;
};

class ProjectIconProvider : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo& info) const override {
        if (info.isDir()) {
            return makeFolderIcon();
        }
        QString suffix = info.suffix().toLower();
        QString fileName = info.fileName();

        if (suffix == "c" || suffix == "cpp" || suffix == "cc") {
            return makeBadgeIcon("C", QColor("#388bfd")); // VS Code Blue
        } else if (suffix == "h" || suffix == "hpp") {
            return makeBadgeIcon("H", QColor("#a371f7")); // Purple
        } else if (suffix == "s" || suffix == "asm") {
            return makeBadgeIcon("S", QColor("#f0883e")); // Orange
        } else if (suffix == "ld") {
            return makeBadgeIcon("LD", QColor("#e3b341")); // Yellow
        } else if (fileName.compare("Makefile", Qt::CaseInsensitive) == 0 ||
                   fileName.compare("CMakeLists.txt", Qt::CaseInsensitive) == 0) {
            return makeBadgeIcon("MK", QColor("#2ea043")); // Green
        } else if (suffix == "o" || suffix == "elf" || suffix == "bin" || suffix == "hex") {
            return makeBadgeIcon("BIN", QColor("#6e7681")); // Subdued Gray
        } else if (suffix == "map" || suffix == "log" || suffix == "txt" || suffix == "md") {
            return makeBadgeIcon("DOC", QColor("#58a6ff")); // Document Blue
        }
        return QFileIconProvider::icon(info);
    }

private:
    static QIcon makeFolderIcon() {
        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#dcb67a")); // Warm IDE folder yellow
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRect(1, 4, 14, 10), 2, 2);
        p.drawRoundedRect(QRect(1, 2, 6, 4), 1, 1);
        return QIcon(pix);
    }

    static QIcon makeBadgeIcon(const QString& tag, const QColor& color) {
        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRect(0, 1, 16, 14), 2, 2);
        p.setPen(Qt::white);
        QFont f = p.font();
        f.setPixelSize(tag.length() > 2 ? 7 : (tag.length() == 2 ? 8 : 9));
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(0, 1, 16, 14), Qt::AlignCenter, tag);
        return QIcon(pix);
    }
};

class ProjectFileDelegate : public QStyledItemDelegate {
public:
    explicit ProjectFileDelegate(QFileSystemModel* model, QSortFilterProxyModel* proxy, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_model(model), m_proxy(proxy) {}

    void setDirtyFiles(const QSet<QString>& dirty) {
        m_dirtyFiles = dirty;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QModelIndex srcIndex = m_proxy ? m_proxy->mapToSource(index) : index;
        if (index.column() == 0 && m_model && srcIndex.isValid()) {
            QString filePath = QDir::cleanPath(m_model->filePath(srcIndex));
            QFileInfo fi(filePath);
            bool isDir = m_model->isDir(srcIndex);
            bool isGenerated = filePath.contains("/generated/") || filePath.endsWith("/generated");
            bool isMainSource = (fi.fileName().compare("Main.c", Qt::CaseInsensitive) == 0 ||
                                 fi.fileName().compare("main.c", Qt::CaseInsensitive) == 0);
            bool isArtifact = (!isDir && (fi.suffix() == "o" || fi.suffix() == "elf" ||
                                          fi.suffix() == "bin" || fi.suffix() == "map" ||
                                          fi.suffix() == "log"));

            if (isGenerated) {
                opt.palette.setColor(QPalette::Text, QColor("#85929e"));
            } else if (isMainSource) {
                opt.palette.setColor(QPalette::Text, QColor("#ffffff"));
                opt.font.setBold(true);
            } else if (isArtifact) {
                opt.palette.setColor(QPalette::Text, QColor("#8b949e"));
            } else {
                opt.palette.setColor(QPalette::Text, QColor(Theme::TextPrimary));
            }
        }

        QStyledItemDelegate::paint(painter, opt, index);

        // Draw badges & dirty indicator
        if (index.column() == 0 && m_model && srcIndex.isValid()) {
            QString filePath = QDir::cleanPath(m_model->filePath(srcIndex));
            bool isDirty = m_dirtyFiles.contains(filePath);
            bool isGenerated = filePath.contains("/generated/") || filePath.endsWith("/generated");

            if (isDirty) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(QColor("#f39c12")); // Amber dirty dot
                painter->setPen(Qt::NoPen);
                int dotSize = 7;
                int dotY = option.rect.top() + (option.rect.height() - dotSize) / 2;
                int dotX = option.rect.right() - (isGenerated ? 50 : 16);
                painter->drawEllipse(dotX, dotY, dotSize, dotSize);
                painter->restore();
            }

            if (isGenerated) {
                painter->save();
                painter->setFont(QFont("Arial", 8, QFont::Bold));
                painter->setPen(QColor("#5dade2"));
                QRect badgeRect = option.rect.adjusted(option.rect.width() - 48, 2, -4, -2);
                painter->drawText(badgeRect, Qt::AlignRight | Qt::AlignVCenter, "[Gen]");
                painter->restore();
            }
        }
    }

private:
    QFileSystemModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxy = nullptr;
    QSet<QString> m_dirtyFiles;
};

ProjectExplorerWidget::ProjectExplorerWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ProjectExplorerWidget::~ProjectExplorerWidget() {
}

void ProjectExplorerWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Title / Header
    auto* header = new QWidget(this);
    header->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(Theme::BgSurface, Theme::BorderSubtle));
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(10, 8, 10, 6);
    headerLayout->setSpacing(4);

    m_titleLabel = new QLabel("PROJECT EXPLORER", header);
    m_titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 11px; color: %1; letter-spacing: 0.5px;")
        .arg(Theme::TextMuted));
    headerLayout->addWidget(m_titleLabel);

    m_pathLabel = new QLabel(header);
    m_pathLabel->setStyleSheet(QString("font-size: 11px; font-weight: bold; color: %1;").arg(Theme::TextPrimary));
    headerLayout->addWidget(m_pathLabel);

    // Toolbar
    auto* toolLayout = new QHBoxLayout();
    toolLayout->setContentsMargins(0, 4, 0, 0);
    toolLayout->setSpacing(4);

    auto makeBtn = [this](const QString& text, const QString& tooltip) -> QPushButton* {
        auto* btn = new QPushButton(text, this);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 10px; font-weight: bold; } "
            "QPushButton:hover { background: %5; color: %6; } "
        ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::Accent, Theme::AccentText));
        return btn;
    };

    m_newFileBtn = makeBtn("+ File", "New File in Project");
    connect(m_newFileBtn, &QPushButton::clicked, this, &ProjectExplorerWidget::onNewFile);
    toolLayout->addWidget(m_newFileBtn);

    m_newFolderBtn = makeBtn("+ Folder", "New Folder in Project");
    connect(m_newFolderBtn, &QPushButton::clicked, this, &ProjectExplorerWidget::onNewFolder);
    toolLayout->addWidget(m_newFolderBtn);

    m_refreshBtn = makeBtn("⟳", "Refresh File Tree");
    connect(m_refreshBtn, &QPushButton::clicked, this, &ProjectExplorerWidget::refresh);
    toolLayout->addWidget(m_refreshBtn);

    m_openFolderBtn = makeBtn("📂", "Open Firmware Folder...");
    connect(m_openFolderBtn, &QPushButton::clicked, this, &ProjectExplorerWidget::onOpenFolder);
    toolLayout->addWidget(m_openFolderBtn);

    m_syncUiBtn = makeBtn("⚡ Sync UI", "Generate / Synchronize Firmware UI Code into 'generated/'");
    m_syncUiBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 10px; font-weight: bold; } "
        "QPushButton:hover { background: %5; color: %6; } "
    ).arg(Theme::AccentSoft, Theme::Accent, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::Accent, Theme::AccentText));
    connect(m_syncUiBtn, &QPushButton::clicked, this, &ProjectExplorerWidget::syncRequested);
    toolLayout->addWidget(m_syncUiBtn);

    toolLayout->addStretch(1);
    headerLayout->addLayout(toolLayout);

    mainLayout->addWidget(header);

    // File Model & Proxy Model
    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileModel->setReadOnly(false);
    m_fileModel->setIconProvider(new ProjectIconProvider());

    connect(m_fileModel, &QFileSystemModel::directoryLoaded, this, &ProjectExplorerWidget::onDirectoryLoaded);

    auto* proxy = new FirmwareFilterProxyModel(this);
    proxy->setSourceModel(m_fileModel);
    m_proxyModel = proxy;
    connect(m_proxyModel, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int) {
        applyRootIndex();
    });

    // Tree View
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_proxyModel);
    m_treeView->setHeaderHidden(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);

    m_delegate = new ProjectFileDelegate(m_fileModel, m_proxyModel, this);
    m_treeView->setItemDelegate(m_delegate);

    // Hide extra columns (Size, Type, Date)
    for (int col = 1; col < 4; ++col) {
        m_treeView->setColumnHidden(col, true);
    }

    m_treeView->setStyleSheet(QString(
        "QTreeView { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: none; "
        "  font-size: 12px; "
        "} "
        "QTreeView::item { "
        "  padding: 3px 2px; "
        "} "
        "QTreeView::item:hover { "
        "  background-color: %3; "
        "} "
        "QTreeView::item:selected { "
        "  background-color: #1f6feb; "
        "  color: #ffffff; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BgOverlay));

    connect(m_treeView, &QTreeView::doubleClicked, this, &ProjectExplorerWidget::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ProjectExplorerWidget::onCustomContextMenuRequested);

    mainLayout->addWidget(m_treeView, 1);
}

void ProjectExplorerWidget::setRootDirectory(const QString& dirPath) {
    if (dirPath.isEmpty()) return;

    m_rootDirectory = QDir::cleanPath(dirPath);
    static_cast<FirmwareFilterProxyModel*>(m_proxyModel)->setRootDirectory(m_rootDirectory);

    QString parentPath = QFileInfo(m_rootDirectory).absolutePath();
    m_fileModel->setRootPath(parentPath);

    QDir dir(m_rootDirectory);
    m_pathLabel->setText(dir.dirName());
    m_pathLabel->setToolTip(m_rootDirectory);

    applyRootIndex();
    QTimer::singleShot(50, this, &ProjectExplorerWidget::applyRootIndex);
    QTimer::singleShot(200, this, &ProjectExplorerWidget::applyRootIndex);
    QTimer::singleShot(500, this, &ProjectExplorerWidget::applyRootIndex);
}

void ProjectExplorerWidget::applyRootIndex() {
    if (m_rootDirectory.isEmpty()) return;

    QString parentPath = QFileInfo(m_rootDirectory).absolutePath();
    QModelIndex parentSrcIdx = m_fileModel->index(parentPath);
    if (parentSrcIdx.isValid()) {
        QModelIndex proxyParent = m_proxyModel->mapFromSource(parentSrcIdx);
        if (proxyParent.isValid() && m_treeView->rootIndex() != proxyParent) {
            m_treeView->setRootIndex(proxyParent);
        }
    }

    QModelIndex rootSrcIdx = m_fileModel->index(m_rootDirectory);
    if (rootSrcIdx.isValid()) {
        if (m_fileModel->canFetchMore(rootSrcIdx)) {
            m_fileModel->fetchMore(rootSrcIdx);
        }
        QModelIndex proxyRoot = m_proxyModel->mapFromSource(rootSrcIdx);
        if (proxyRoot.isValid()) {
            m_treeView->expand(proxyRoot);

            // Expand immediate child directories (inc, scr, src, startup, build, etc.)
            int rowCount = m_proxyModel->rowCount(proxyRoot);
            for (int r = 0; r < rowCount; ++r) {
                QModelIndex childProxy = m_proxyModel->index(r, 0, proxyRoot);
                QModelIndex childSrc = m_proxyModel->mapToSource(childProxy);
                if (m_fileModel->isDir(childSrc)) {
                    if (m_fileModel->canFetchMore(childSrc)) {
                        m_fileModel->fetchMore(childSrc);
                    }
                    m_treeView->expand(childProxy);
                }
            }
        }
    }
}

void ProjectExplorerWidget::onDirectoryLoaded(const QString& path) {
    QString cleanLoaded = QDir::cleanPath(path);
    QString cleanRoot = QDir::cleanPath(m_rootDirectory);
    QString parentPath = QFileInfo(cleanRoot).absolutePath();

    if (cleanLoaded == parentPath || cleanLoaded == cleanRoot || cleanLoaded.startsWith(cleanRoot + "/")) {
        applyRootIndex();
    }
}

void ProjectExplorerWidget::refresh() {
    if (!m_rootDirectory.isEmpty()) {
        setRootDirectory(m_rootDirectory);
    }
}

void ProjectExplorerWidget::onFileDirtyChanged(const QString& filePath, bool dirty) {
    QString clean = QDir::cleanPath(filePath);
    if (dirty) {
        m_dirtyFiles.insert(clean);
    } else {
        m_dirtyFiles.remove(clean);
    }
    if (m_delegate) {
        m_delegate->setDirtyFiles(m_dirtyFiles);
    }
    m_treeView->viewport()->update();
}

void ProjectExplorerWidget::expandAllFolders() {
    m_treeView->expandAll();
}

void ProjectExplorerWidget::collapseAllFolders() {
    applyRootIndex();
}

void ProjectExplorerWidget::onItemDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;

    QModelIndex srcIndex = m_proxyModel ? m_proxyModel->mapToSource(index) : index;
    if (!m_fileModel->isDir(srcIndex)) {
        QString filePath = m_fileModel->filePath(srcIndex);
        emit fileActivated(filePath);
    }
}

void ProjectExplorerWidget::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    QModelIndex srcIndex = (m_proxyModel && index.isValid()) ? m_proxyModel->mapToSource(index) : index;

    QMenu menu(this);
    if (srcIndex.isValid() && !m_fileModel->isDir(srcIndex)) {
        QAction* openAct = menu.addAction("Open in Editor");
        connect(openAct, &QAction::triggered, this, [this, srcIndex]() {
            emit fileActivated(m_fileModel->filePath(srcIndex));
        });
        menu.addSeparator();
    }

    QString targetDir = m_rootDirectory;
    if (srcIndex.isValid()) {
        if (m_fileModel->isDir(srcIndex)) {
            targetDir = m_fileModel->filePath(srcIndex);
        } else {
            targetDir = QFileInfo(m_fileModel->filePath(srcIndex)).absolutePath();
        }
    }

    QAction* newFileAct = menu.addAction("New File Here...");
    connect(newFileAct, &QAction::triggered, this, [this, targetDir]() {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New File", "Enter file name:", QLineEdit::Normal, "new_file.c", &ok);
        if (ok && !name.trimmed().isEmpty()) {
            QFile file(QDir(targetDir).filePath(name.trimmed()));
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.close();
                refresh();
                emit fileActivated(file.fileName());
            }
        }
    });

    QAction* newFolderAct = menu.addAction("New Folder Here...");
    connect(newFolderAct, &QAction::triggered, this, [this, targetDir]() {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New Folder", "Enter folder name:", QLineEdit::Normal, "new_folder", &ok);
        if (ok && !name.trimmed().isEmpty()) {
            QDir(targetDir).mkdir(name.trimmed());
            refresh();
        }
    });

    if (srcIndex.isValid()) {
        menu.addSeparator();
        QString path = m_fileModel->filePath(srcIndex);

        QAction* copyPathAct = menu.addAction("Copy Path");
        connect(copyPathAct, &QAction::triggered, this, [path]() {
            QApplication::clipboard()->setText(path);
        });

        QAction* copyRelPathAct = menu.addAction("Copy Relative Path");
        connect(copyRelPathAct, &QAction::triggered, this, [this, path]() {
            QApplication::clipboard()->setText(QDir(m_rootDirectory).relativeFilePath(path));
        });

        QAction* renameAct = menu.addAction("Rename");
        connect(renameAct, &QAction::triggered, this, [this, index]() {
            m_treeView->edit(index);
        });

        QAction* deleteAct = menu.addAction("Delete");
        connect(deleteAct, &QAction::triggered, this, [this, path, srcIndex]() {
            auto res = QMessageBox::question(this, "Delete",
                QString("Are you sure you want to delete \"%1\"?").arg(QFileInfo(path).fileName()),
                QMessageBox::Yes | QMessageBox::No);
            if (res == QMessageBox::Yes) {
                m_fileModel->remove(srcIndex);
            }
        });
    }

    menu.addSeparator();
    QAction* openFolderAct = menu.addAction("Open Different Firmware Folder...");
    connect(openFolderAct, &QAction::triggered, this, &ProjectExplorerWidget::onOpenFolder);

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void ProjectExplorerWidget::onNewFile() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New File", "Enter file name:", QLineEdit::Normal, "main.c", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString dir = m_rootDirectory.isEmpty() ? QDir::currentPath() : m_rootDirectory;
        QString path = QDir(dir).filePath(name.trimmed());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.close();
            refresh();
            emit fileActivated(path);
        }
    }
}

void ProjectExplorerWidget::onNewFolder() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Folder", "Enter folder name:", QLineEdit::Normal, "src", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString dir = m_rootDirectory.isEmpty() ? QDir::currentPath() : m_rootDirectory;
        QDir(dir).mkdir(name.trimmed());
        refresh();
    }
}

void ProjectExplorerWidget::onOpenFolder() {
    QString startDir = m_rootDirectory.isEmpty() ? QDir::homePath() : m_rootDirectory;
    QString dir = QFileDialog::getExistingDirectory(this, "Select Firmware Project Directory", startDir);
    if (!dir.isEmpty()) {
        setRootDirectory(dir);
        emit firmwareDirectorySelected(dir);
        emit directorySelected(dir);
    }
}
