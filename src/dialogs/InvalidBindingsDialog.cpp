#include "InvalidBindingsDialog.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>

InvalidBindingsDialog::InvalidBindingsDialog(const BoardDef& newBoard,
                                             const QList<InvalidBindingItem>& items,
                                             QWidget* parent)
    : QDialog(parent)
    , m_newBoard(newBoard)
    , m_items(items)
{
    setWindowTitle("Hardware Pin Binding Conflict Warning");
    resize(760, 440);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(Theme::BgBase, Theme::TextPrimary));

    setupUi();
}

void InvalidBindingsDialog::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel("⚠ Target Board Changed — Incompatible Pin Bindings Detected", this);
    QFont tf = font();
    tf.setPointSize(12);
    tf.setBold(true);
    titleLabel->setFont(tf);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::Warning));
    rootLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(
        QString("The new target board '%1' (%2) has a different pin layout than your current design.\n"
                "The bindings listed below cannot be directly mapped. You may select a replacement pin for each component, or keep it as a custom binding.")
            .arg(m_newBoard.displayName.isEmpty() ? m_newBoard.id : m_newBoard.displayName, m_newBoard.family),
        this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    rootLayout->addWidget(descLabel);

    m_table = new QTableWidget(this);
    m_table->setObjectName("invalidBindingsTable");
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Component", "Bound Pin", "Protocol", "Conflict Issue", "Remap on New Board"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setRowCount(m_items.size());
    m_table->setStyleSheet(QString(
        "QTableWidget { background-color: %1; gridline-color: %2; border: 1px solid %2; border-radius: %3px; }"
    ).arg(Theme::BgRaised, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));

    QStringList validPins = m_newBoard.pinMap.keys();
    std::sort(validPins.begin(), validPins.end());

    m_remapCombos.clear();
    for (int r = 0; r < m_items.size(); ++r) {
        const auto& item = m_items[r];

        auto* compItem = new QTableWidgetItem(item.componentName.isEmpty() ? item.componentId : item.componentName);
        compItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 0, compItem);

        auto* pinItem = new QTableWidgetItem(item.currentPin);
        pinItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pinItem->setFont(QFont("Monospace", 9, QFont::Bold));
        m_table->setItem(r, 1, pinItem);

        auto* protoItem = new QTableWidgetItem(item.currentProtocol);
        protoItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(r, 2, protoItem);

        auto* reasonItem = new QTableWidgetItem(item.reason);
        reasonItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        reasonItem->setForeground(Theme::cWarning());
        m_table->setItem(r, 3, reasonItem);

        auto* combo = new QComboBox(m_table);
        combo->setStyleSheet(QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 11px; min-width: 170px; }"
        ).arg(Theme::BgBase, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));

        combo->addItem("[Keep as Custom Binding]", "__KEEP_CUSTOM__");
        for (const QString& p : validPins) {
            PinDef pd = m_newBoard.pinMap.value(p);
            QString label = p;
            if (!pd.function.isEmpty()) label += QString(" (%1)").arg(pd.function);
            if (pd.strapping) label += " [Strapping]";
            else if (pd.reserved) label += " [Reserved]";
            combo->addItem(label, p);
        }
        m_remapCombos.append(combo);
        m_table->setCellWidget(r, 4, combo);
    }
    rootLayout->addWidget(m_table, 1);

    auto* btnLayout = new QHBoxLayout();
    m_btnKeepAll = new QPushButton("Keep All as Custom", this);
    m_btnKeepAll->setObjectName("btnKeepAllCustom");
    m_btnKeepAll->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px 14px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; }"
    ).arg(Theme::BgRaised, Theme::BorderSubtle, Theme::TextSecondary, QString::number(Theme::RadiusSm), Theme::BgOverlay));
    connect(m_btnKeepAll, &QPushButton::clicked, this, &InvalidBindingsDialog::onKeepAllClicked);
    btnLayout->addWidget(m_btnKeepAll);

    btnLayout->addStretch(1);

    m_btnCancel = new QPushButton("Cancel", this);
    m_btnCancel->setObjectName("btnCancelRemap");
    m_btnCancel->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px 14px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: %5; }"
    ).arg(Theme::BgRaised, Theme::BorderSubtle, Theme::TextSecondary, QString::number(Theme::RadiusSm), Theme::BgOverlay));
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_btnCancel);

    m_btnApply = new QPushButton("Apply Remapping", this);
    m_btnApply->setObjectName("btnApplyRemap");
    m_btnApply->setDefault(true);
    m_btnApply->setStyleSheet(QString(
        "QPushButton { background-color: #3b82f6; border: none; color: #ffffff; border-radius: %1px; padding: 6px 16px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: #2563eb; }"
    ).arg(QString::number(Theme::RadiusSm)));
    connect(m_btnApply, &QPushButton::clicked, this, &InvalidBindingsDialog::onApplyClicked);
    btnLayout->addWidget(m_btnApply);

    rootLayout->addLayout(btnLayout);
}

void InvalidBindingsDialog::onKeepAllClicked() {
    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i].keepAsCustom = true;
        m_items[i].remappedPin = m_items[i].currentPin;
    }
    accept();
}

void InvalidBindingsDialog::onApplyClicked() {
    for (int i = 0; i < m_items.size() && i < m_remapCombos.size(); ++i) {
        QString data = m_remapCombos[i]->currentData().toString();
        if (data == "__KEEP_CUSTOM__" || data.isEmpty()) {
            m_items[i].keepAsCustom = true;
            m_items[i].remappedPin = m_items[i].currentPin;
        } else {
            m_items[i].keepAsCustom = false;
            m_items[i].remappedPin = data;
        }
    }
    accept();
}

QList<InvalidBindingItem> InvalidBindingsDialog::resolvedBindings() const {
    return m_items;
}
