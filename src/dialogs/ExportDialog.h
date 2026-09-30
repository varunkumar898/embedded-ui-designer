#pragma once

#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget* parent = nullptr);

    QString selectedTarget() const;    // "ugfx" | "qul" | "lvgl"
    QString outputFolder() const;

private slots:
    void onBrowseFolder();

private:
    QRadioButton* m_ugfxBtn = nullptr;
    QRadioButton* m_qulBtn  = nullptr;
    QRadioButton* m_lvglBtn = nullptr;
    QLineEdit*    m_folderEdit = nullptr;

    void setupUi();
};
