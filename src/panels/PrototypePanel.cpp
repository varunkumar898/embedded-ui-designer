#include "PrototypePanel.h"

#include "CanvasScene.h"
#include "UIComponent.h"
#include "CheckboxComponent.h"
#include "SwitchComponent.h"
#include "SliderComponent.h"
#include "ProgressBarComponent.h"
#include "LabelComponent.h"
#include "ButtonComponent.h"
#include "TextInputComponent.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QSignalBlocker>

PrototypePanel::PrototypePanel(CanvasScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_scene(scene)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("INTERACTIONS", this);
    title->setStyleSheet("color: #8490a1; font-size: 10px; font-weight: bold;");
    auto* addButton = new QPushButton("+ Add Interaction", this);
    addButton->setObjectName("addInteractionButton");
    addButton->setToolTip("Add a same-screen interaction");
    addButton->setStyleSheet("QPushButton { padding: 5px 8px; border-radius: 4px; }");
    header->addWidget(title);
    header->addStretch(1);
    header->addWidget(addButton);
    root->addLayout(header);
    connect(addButton, &QPushButton::clicked, this, &PrototypePanel::addInteraction);

    m_emptyState = new QLabel("Select a component to add interactions.", this);
    static_cast<QLabel*>(m_emptyState)->setAlignment(Qt::AlignCenter);
    static_cast<QLabel*>(m_emptyState)->setStyleSheet("color: #697384; padding: 18px 8px;");
    root->addWidget(m_emptyState);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_rowsWidget = new QWidget(scroll);
    m_rowsLayout = new QVBoxLayout(m_rowsWidget);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(6);
    m_rowsLayout->addStretch(1);
    scroll->setWidget(m_rowsWidget);
    root->addWidget(scroll, 1);

    if (m_scene) {
        connect(m_scene, &CanvasScene::componentAdded, this, &PrototypePanel::refreshTargets);
        connect(m_scene, &CanvasScene::componentRemoved, this, &PrototypePanel::refreshTargets);
    }
    setTargetComponent(nullptr);
}

void PrototypePanel::setTargetComponent(UIComponent* component) {
    if (m_source) disconnect(m_source, nullptr, this, nullptr);
    m_source = component;
    if (m_source) {
        connect(m_source, &UIComponent::interactionTriggered, this, &PrototypePanel::runInteractions);
    }
    m_emptyState->setVisible(!m_source);
    rebuildRows();
}

void PrototypePanel::addInteraction() {
    if (!m_source) return;
    QJsonArray interactions = m_source->interactions();
    QString targetId;
    for (UIComponent* component : m_scene->uiComponents()) {
        if (component != m_source) {
            targetId = component->componentId();
            break;
        }
    }
    QJsonObject interaction;
    interaction["trigger"] = "On Click";
    interaction["target"] = targetId;
    interaction["action"] = "Toggle";
    interaction["value"] = "true";
    interaction["transition"] = "Slide";
    interaction["duration"] = 500;
    interactions.append(interaction);
    m_source->setInteractions(interactions);
    rebuildRows();
}

void PrototypePanel::rebuildRows() {
    while (m_rowsLayout->count() > 0) {
        QLayoutItem* item = m_rowsLayout->takeAt(0);
        if (QWidget* widget = item->widget()) delete widget;
        delete item;
    }
    m_targetCombos.clear();
    if (!m_source) return;

    const QJsonArray interactions = m_source->interactions();
    for (int index = 0; index < interactions.size(); ++index) {
        const QJsonObject data = interactions.at(index).toObject();
        auto* row = new QFrame(m_rowsWidget);
        row->setObjectName("interactionRow");
        row->setStyleSheet("QFrame#interactionRow { background: #171a20; border: 1px solid #303642; border-radius: 4px; }");
        auto* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(8, 8, 8, 8);
        rowLayout->setSpacing(5);

        auto* deleteRow = new QHBoxLayout();
        auto* rowTitle = new QLabel(QString("Interaction %1").arg(index + 1), row);
        rowTitle->setStyleSheet("color: #9ba6b6; font-size: 11px; font-weight: bold;");
        auto* removeButton = new QPushButton("×", row);
        removeButton->setFixedSize(22, 22);
        removeButton->setToolTip("Remove interaction");
        deleteRow->addWidget(rowTitle);
        deleteRow->addStretch(1);
        deleteRow->addWidget(removeButton);
        rowLayout->addLayout(deleteRow);

        auto* form = new QFormLayout();
        form->setContentsMargins(0, 0, 0, 0);
        form->setSpacing(4);
        auto* trigger = new QComboBox(row);
        trigger->setObjectName("trigger");
        trigger->addItems({"On Click", "On Change", "On Value Changed"});
        auto* target = new QComboBox(row);
        target->setObjectName("target");
        m_targetCombos.append(target);
        auto* action = new QComboBox(row);
        action->setObjectName("action");
        action->addItems({"Set Value", "Toggle", "Show", "Hide"});
        auto* value = new QLineEdit(row);
        value->setObjectName("value");
        value->setPlaceholderText("Value");
        auto* transition = new QComboBox(row);
        transition->setObjectName("transition");
        transition->addItems({"Instant", "Dissolve", "Slide"});
        auto* duration = new QSpinBox(row);
        duration->setObjectName("duration");
        duration->setRange(0, 10000);
        duration->setSingleStep(50);
        duration->setSuffix(" ms");

        form->addRow("Trigger", trigger);
        form->addRow("Target", target);
        form->addRow("Action", action);
        form->addRow("Value", value);
        form->addRow("Transition", transition);
        form->addRow("Duration", duration);
        rowLayout->addLayout(form);

        trigger->setCurrentText(data.value("trigger").toString("On Click"));
        action->setCurrentText(data.value("action").toString("Toggle"));
        value->setText(data.value("value").toString("true"));
        transition->setCurrentText(data.value("transition").toString("Instant"));
        duration->setValue(data.value("duration").toInt(250));
        value->setVisible(action->currentText() == "Set Value");
        form->labelForField(value)->setVisible(action->currentText() == "Set Value");
        refreshTargets();
        target->setCurrentIndex(qMax(0, target->findData(data.value("target").toString())));

        auto save = [this]() { saveRows(); };
        connect(trigger, &QComboBox::currentTextChanged, this, save);
        connect(target, &QComboBox::currentIndexChanged, this, save);
        connect(action, &QComboBox::currentTextChanged, this, [this, action, value, form, save]() {
            const bool setValue = action->currentText() == "Set Value";
            value->setVisible(setValue);
            form->labelForField(value)->setVisible(setValue);
            save();
        });
        connect(value, &QLineEdit::textChanged, this, save);
        connect(transition, &QComboBox::currentTextChanged, this, save);
        connect(duration, QOverload<int>::of(&QSpinBox::valueChanged), this, save);
        connect(removeButton, &QPushButton::clicked, this, [this, index]() { removeInteraction(index); });
        m_rowsLayout->addWidget(row);
    }
    m_rowsLayout->addStretch(1);
}

void PrototypePanel::refreshTargets() {
    if (!m_source) return;
    for (QComboBox* target : m_targetCombos) {
        const QString selectedId = target->currentData().toString();
        const QSignalBlocker blocker(target);
        target->clear();
        if (m_scene) {
            for (UIComponent* component : m_scene->uiComponents()) {
                if (component != m_source) {
                    target->addItem(QString("%1 (%2)").arg(component->componentId(), component->componentType()),
                                    component->componentId());
                }
            }
        }
        const int index = target->findData(selectedId);
        if (index >= 0) target->setCurrentIndex(index);
    }
}

void PrototypePanel::saveRows() {
    if (!m_source) return;
    QJsonArray interactions;
    for (int index = 0; index < m_rowsLayout->count(); ++index) {
        QWidget* row = m_rowsLayout->itemAt(index)->widget();
        if (!row) continue;
        const auto trigger = row->findChild<QComboBox*>("trigger");
        const auto target = row->findChild<QComboBox*>("target");
        const auto action = row->findChild<QComboBox*>("action");
        const auto value = row->findChild<QLineEdit*>("value");
        const auto transition = row->findChild<QComboBox*>("transition");
        const auto duration = row->findChild<QSpinBox*>("duration");
        if (!trigger || !target || !action || !value || !transition || !duration) continue;
        QJsonObject interaction;
        interaction["trigger"] = trigger->currentText();
        interaction["target"] = target->currentData().toString();
        interaction["action"] = action->currentText();
        interaction["value"] = value->text();
        interaction["transition"] = transition->currentText();
        interaction["duration"] = duration->value();
        interactions.append(interaction);
    }
    m_source->setInteractions(interactions);
}

void PrototypePanel::removeInteraction(int index) {
    if (!m_source) return;
    QJsonArray interactions = m_source->interactions();
    if (index < 0 || index >= interactions.size()) return;
    interactions.removeAt(index);
    m_source->setInteractions(interactions);
    rebuildRows();
}

void PrototypePanel::runInteractions(const QString& trigger) {
    if (!m_source) return;
    for (const QJsonValue& value : m_source->interactions()) {
        const QJsonObject interaction = value.toObject();
        if (interaction.value("trigger").toString() == trigger) applyInteraction(interaction);
    }
}

void PrototypePanel::applyInteraction(const QJsonObject& interaction) {
    if (!m_scene) return;
    UIComponent* target = nullptr;
    const QString targetId = interaction.value("target").toString();
    for (UIComponent* component : m_scene->uiComponents()) {
        if (component->componentId() == targetId) {
            target = component;
            break;
        }
    }
    if (!target) return;

    const QString action = interaction.value("action").toString();
    const QString transition = interaction.value("transition").toString("Instant");
    const int duration = interaction.value("duration").toInt();
    const QString value = interaction.value("value").toString();
    if (action == "Show" || action == "Hide") {
        target->setVisible(action == "Show");
    } else if (auto sw = dynamic_cast<SwitchComponent*>(target)) {
        if (action == "Toggle") sw->animateChecked(!sw->isChecked(), transition, duration);
        else if (action == "Set Value") sw->animateChecked(value.compare("true", Qt::CaseInsensitive) == 0 || value == "1", transition, duration);
    } else if (auto checkbox = dynamic_cast<CheckboxComponent*>(target)) {
        if (action == "Toggle") checkbox->setChecked(!checkbox->isChecked());
        else if (action == "Set Value") checkbox->setChecked(value.compare("true", Qt::CaseInsensitive) == 0 || value == "1");
    } else if (action == "Toggle") {
        target->setVisible(!target->isVisible());
    } else if (action == "Set Value") {
        if (auto slider = dynamic_cast<SliderComponent*>(target)) slider->setValue(value.toInt());
        else if (auto progress = dynamic_cast<ProgressBarComponent*>(target)) progress->setValue(value.toDouble());
        else if (auto label = dynamic_cast<LabelComponent*>(target)) label->setText(value);
        else if (auto button = dynamic_cast<ButtonComponent*>(target)) button->setText(value);
        else if (auto input = dynamic_cast<TextInputComponent*>(target)) input->setText(value);
    }
}