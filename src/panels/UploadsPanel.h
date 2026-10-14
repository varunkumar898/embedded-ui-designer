#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QList>

class CanvasScene;
class QUndoStack;
class Project;

struct UploadedAsset {
    QString id;
    QString fileName;
    QString filePath;
    QSize dimensions;
    qint64 fileSize = 0;
};

class UploadsPanel : public QWidget {
    Q_OBJECT

public:
    explicit UploadsPanel(CanvasScene* scene, QUndoStack* undoStack, Project* project, QWidget* parent = nullptr);
    ~UploadsPanel() override = default;

    void addAsset(const QString& filePath);
    void refreshAssetGrid();

signals:
    void assetSelectedForCanvas(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onUploadButtonClicked();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUi();

    CanvasScene* m_scene = nullptr;
    QUndoStack* m_undoStack = nullptr;
    Project* m_project = nullptr;

    QPushButton* m_uploadBtn = nullptr;
    QListWidget* m_assetList = nullptr;
    QLabel* m_dropHintLabel = nullptr;

    QList<UploadedAsset> m_assets;
};
