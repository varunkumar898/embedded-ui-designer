#include "UploadsPanel.h"
#include "theme/Theme.h"
#include "CanvasScene.h"
#include "Project.h"
#include "ImageComponent.h"
#include "AddComponentCommand.h"
#include <QUndoStack>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUuid>
#include <QDir>

UploadsPanel::UploadsPanel(CanvasScene* scene, QUndoStack* undoStack, Project* project, QWidget* parent)
    : QWidget(parent)
    , m_scene(scene)
    , m_undoStack(undoStack)
    , m_project(project)
{
    setupUi();
    setAcceptDrops(true);
}

void UploadsPanel::setupUi() {
    setObjectName("UploadsPanel");
    setStyleSheet(QString(
        "#UploadsPanel { background-color: %1; color: %2; }"
        "QPushButton { background: %3; color: %4; border: none; border-radius: %5px; padding: 8px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background: %6; }"
        "QListWidget { background-color: %7; border: 1px solid %8; border-radius: %5px; color: %2; }"
        "QListWidget::item { padding: 4px; border-radius: %9px; margin: 3px; }"
        "QListWidget::item:hover { background-color: %10; }"
        "QListWidget::item:selected { background-color: %11; color: %4; }"
        "QLabel { color: %12; font-size: 11px; }"
    ).arg(Theme::BgSurface, Theme::TextPrimary, Theme::Accent, Theme::BgBase)
     .arg(Theme::RadiusMd)
     .arg(Theme::AccentHover, Theme::BgBase, Theme::BorderSubtle)
     .arg(Theme::RadiusSm)
     .arg(Theme::BgOverlay, Theme::AccentSoft, Theme::TextMuted));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_uploadBtn = new QPushButton("☁ Upload Images", this);
    connect(m_uploadBtn, &QPushButton::clicked, this, &UploadsPanel::onUploadButtonClicked);
    layout->addWidget(m_uploadBtn);

    m_dropHintLabel = new QLabel("Drag & drop PNG, JPG, WebP, SVG here", this);
    m_dropHintLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_dropHintLabel);

    m_assetList = new QListWidget(this);
    m_assetList->setViewMode(QListView::IconMode);
    m_assetList->setIconSize(QSize(64, 64));
    m_assetList->setGridSize(QSize(90, 95));
    m_assetList->setResizeMode(QListWidget::Adjust);
    m_assetList->setWordWrap(true);
    connect(m_assetList, &QListWidget::itemDoubleClicked, this, &UploadsPanel::onItemDoubleClicked);
    connect(m_assetList, &QListWidget::itemClicked, this, &UploadsPanel::onItemClicked);
    layout->addWidget(m_assetList);
}

void UploadsPanel::onUploadButtonClicked() {
    QString filter = "Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.svg);;PNG (*.png);;JPEG (*.jpg *.jpeg);;WebP (*.webp);;All Files (*)";
    QStringList files = QFileDialog::getOpenFileNames(this, "Upload Images into Project", QString(), filter);
    for (const QString& file : files) {
        addAsset(file);
    }
}

void UploadsPanel::addAsset(const QString& filePath) {
    QFileInfo fi(filePath);
    if (!fi.exists()) return;

    // Validate format
    QImageReader reader(filePath);
    QSize sz = reader.size();
    if (sz.isEmpty() || !sz.isValid()) {
        QImage img(filePath);
        if (!img.isNull()) {
            sz = img.size();
        } else {
            QMessageBox::warning(this, "Upload Error", QString("Could not read image file:\n%1").arg(filePath));
            return;
        }
    }

    UploadedAsset asset;
    asset.id = "asset_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    asset.fileName = fi.fileName();
    asset.filePath = filePath;
    asset.dimensions = sz;
    asset.fileSize = fi.size();

    m_assets.append(asset);
    refreshAssetGrid();
}

void UploadsPanel::refreshAssetGrid() {
    m_assetList->clear();
    for (const UploadedAsset& asset : m_assets) {
        QPixmap pm(asset.filePath);
        if (pm.isNull()) {
            pm = QPixmap(64, 64);
            pm.fill(Theme::cBgRaised());
        } else {
            pm = pm.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        QString labelText = QString("%1\n%2×%3").arg(asset.fileName).arg(asset.dimensions.width()).arg(asset.dimensions.height());
        QListWidgetItem* item = new QListWidgetItem(QIcon(pm), labelText, m_assetList);
        item->setData(Qt::UserRole, asset.filePath);
        item->setToolTip(QString("%1\nSize: %2 KB\nPath: %3").arg(asset.fileName).arg(asset.fileSize / 1024).arg(asset.filePath));
    }
}

void UploadsPanel::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();

    // Check if an ImageComponent is selected
    if (m_scene) {
        for (QGraphicsItem* it : m_scene->selectedItems()) {
            if (auto img = dynamic_cast<ImageComponent*>(it)) {
                img->setImagePath(path);
                return;
            }
        }
    }
}

void UploadsPanel::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item || !m_scene) return;
    QString path = item->data(Qt::UserRole).toString();

    // Insert new ImageComponent at center of canvas
    QString id = "img_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(6);
    UIComponent* comp = Project::createComponentInstance("Image", id);
    if (auto img = dynamic_cast<ImageComponent*>(comp)) {
        img->setImagePath(path);
        QImage loaded = img->loadedImage();
        if (!loaded.isNull()) {
            qreal w = std::min(240.0, static_cast<qreal>(loaded.width()));
            qreal h = (w / loaded.width()) * loaded.height();
            img->setCompSize(w, h);
        }
        QPointF center = m_scene->displayRect().center();
        img->setPos(m_scene->snapPoint(center - QPointF(img->compWidth() / 2.0, img->compHeight() / 2.0)));
        m_scene->clearSelection();
        if (m_undoStack) {
            m_undoStack->push(new AddComponentCommand(m_scene, img));
        } else {
            m_scene->addUIComponent(img);
            img->setSelected(true);
        }
    }
}

void UploadsPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void UploadsPanel::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            addAsset(url.toLocalFile());
        }
    }
    event->acceptProposedAction();
}
