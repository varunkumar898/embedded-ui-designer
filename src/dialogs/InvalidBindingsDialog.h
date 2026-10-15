#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include "hardware/BoardRegistry.h"

class UIComponent;

struct InvalidBindingItem {
    UIComponent* component = nullptr;
    QString componentId;
    QString componentName;
    QString currentPin;
    QString currentProtocol;
    QString reason;
    QString remappedPin;
    bool keepAsCustom = false;
};

class InvalidBindingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit InvalidBindingsDialog(const BoardDef& newBoard,
                                   const QList<InvalidBindingItem>& items,
                                   QWidget* parent = nullptr);
    ~InvalidBindingsDialog() override = default;

    QList<InvalidBindingItem> resolvedBindings() const;
    QTableWidget* table() const { return m_table; }
    QPushButton* applyButton() const { return m_btnApply; }
    QPushButton* keepAllButton() const { return m_btnKeepAll; }

private slots:
    void onKeepAllClicked();
    void onApplyClicked();

private:
    void setupUi();

    BoardDef m_newBoard;
    QList<InvalidBindingItem> m_items;
    QTableWidget* m_table = nullptr;
    QPushButton* m_btnApply = nullptr;
    QPushButton* m_btnKeepAll = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QList<QComboBox*> m_remapCombos;
};
