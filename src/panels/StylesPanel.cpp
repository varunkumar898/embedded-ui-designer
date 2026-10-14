#include "StylesPanel.h"
#include "Project.h"
#include "ColorStyle.h"
#include "ColorPickerDialog.h"
#include "theme/Theme.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPainter>
#include <QRegularExpression>

StylesPanel::StylesPanel(Project* project, QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    if (project) {
        setProject(project);
    }
}

void StylesPanel::setProject(Project* project) {
    if (m_project != project) {
        if (m_project) {
            disconnect(m_project, &Project::colorStylesChanged, this, &StylesPanel::refreshStyles);
            disconnect(m_project, &Project::projectLoaded, this, &StylesPanel::refreshStyles);
        }
        m_project = project;
        if (m_project) {
            connect(m_project, &Project::colorStylesChanged, this, &StylesPanel::refreshStyles);
            connect(m_project, &Project::projectLoaded, this, &StylesPanel::refreshStyles);
        }
        refreshStyles();
    }
}

void StylesPanel::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    // Header bar with Title and "+" button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(2, 2, 2, 2);

    QLabel* titleLabel = new QLabel("THEME STYLES", this);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; letter-spacing: 0.5px;").arg(Theme::TextMuted));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    QPushButton* addBtn = new QPushButton("+ Add Style", this);
    addBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; font-size: 11px; font-weight: 600; border: 1px solid %3; border-radius: %4px; padding: 4px 10px; }"
        "QPushButton:hover { background: %5; border-color: %6; color: %7; }"
        "QPushButton:pressed { background: %8; color: %7; }")
        .arg(Theme::BgRaised, Theme::AccentText, Theme::BorderSubtle)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::Accent, Theme::TextPrimary, Theme::AccentPressed)
    );
    connect(addBtn, &QPushButton::clicked, this, &StylesPanel::onAddStyleClicked);
    headerLayout->addWidget(addBtn);

    rootLayout->addLayout(headerLayout);

    // Scroll area for styles list
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    m_listContainer = new QWidget();
    m_listContainer->setStyleSheet("background: transparent;");
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(6);
    m_listLayout->addStretch(1);

    scrollArea->setWidget(m_listContainer);
    rootLayout->addWidget(scrollArea, 1);
}

void StylesPanel::refreshStyles() {
    if (!m_listLayout) return;

    // Remove all previous rows except the bottom stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (!m_project) return;

    const auto styles = m_project->colorStyles();
    for (int i = 0; i < styles.size(); ++i) {
        QWidget* rowWidget = createStyleRow(styles[i]);
        m_listLayout->insertWidget(i, rowWidget);
    }
}

QWidget* StylesPanel::createStyleRow(const ColorStyle& style) {
    QWidget* row = new QWidget(m_listContainer);
    row->setStyleSheet(QString(
        "QWidget { background: %1; border: 1px solid %2; border-radius: %3px; padding: 4px; }"
        "QWidget:hover { border-color: %4; }")
        .arg(Theme::BgRaised, Theme::BorderSubtle)
        .arg(Theme::RadiusMd)
        .arg(Theme::BorderStrong)
    );

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    // Color Swatch Button
    QPushButton* swatchBtn = new QPushButton(row);
    swatchBtn->setFixedSize(28, 28);
    swatchBtn->setCursor(Qt::PointingHandCursor);
    swatchBtn->setToolTip("Click to change color");
    swatchBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 2px solid %2; border-radius: 5px; }"
        "QPushButton:hover { border: 2px solid %3; }"
    ).arg(style.color.name(), Theme::BorderStrong, Theme::Accent));

    connect(swatchBtn, &QPushButton::clicked, this, [this, style]() {
        if (!m_project) return;
        QColor picked = ColorPickerDialog::getColor(style.color, this, QString("Choose color for %1").arg(style.name));
        if (picked.isValid() && picked != style.color) {
            m_project->updateColorStyle(style.name, picked);
        }
    });
    layout->addWidget(swatchBtn);

    // Style Name Edit
    QLineEdit* nameEdit = new QLineEdit(style.name, row);
    nameEdit->setPlaceholderText("Style Name");
    nameEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; font-size: 12px; font-weight: 600; border: 1px solid %3; border-radius: %4px; padding: 4px 6px; }"
        "QLineEdit:focus { border-color: %5; }")
        .arg(Theme::BgBase, Theme::TextPrimary, Theme::BorderSubtle)
        .arg(Theme::RadiusSm)
        .arg(Theme::Accent)
    );
    QString originalName = style.name;
    connect(nameEdit, &QLineEdit::editingFinished, this, [this, nameEdit, originalName, style]() {
        if (!m_project) return;
        QString newName = nameEdit->text().trimmed();
        if (newName.isEmpty() || newName == originalName) return;

        // Rename style in project
        auto styles = m_project->colorStyles();
        for (auto& s : styles) {
            if (s.name == originalName) {
                s.name = newName;
                break;
            }
        }
        m_project->setColorStyles(styles);
    });
    layout->addWidget(nameEdit, 1);

    // Hex Edit
    QLineEdit* hexEdit = new QLineEdit(style.color.name().toUpper(), row);
    hexEdit->setFixedWidth(70);
    hexEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; font-family: monospace; font-size: 11px; border: 1px solid %3; border-radius: %4px; padding: 4px; }"
        "QLineEdit:focus { border-color: %5; color: %6; }")
        .arg(Theme::BgBase, Theme::TextSecondary, Theme::BorderSubtle)
        .arg(Theme::RadiusSm)
        .arg(Theme::Accent, Theme::TextPrimary)
    );
    connect(hexEdit, &QLineEdit::editingFinished, this, [this, hexEdit, style]() {
        if (!m_project) return;
        QString hex = hexEdit->text().trimmed();
        if (!hex.startsWith('#')) hex.prepend('#');
        QColor c(hex);
        if (c.isValid()) {
            m_project->updateColorStyle(style.name, c);
        } else {
            hexEdit->setText(style.color.name().toUpper());
        }
    });
    layout->addWidget(hexEdit);

    // Delete Button
    QPushButton* delBtn = new QPushButton("✕", row);
    delBtn->setFixedSize(22, 22);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setToolTip("Delete style");
    delBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; font-size: 11px; font-weight: 600; border: none; border-radius: 3px; }"
        "QPushButton:hover { background: %2; color: %3; }")
        .arg(Theme::TextMuted, Theme::Danger, Theme::TextPrimary)
    );
    connect(delBtn, &QPushButton::clicked, this, [this, style]() {
        if (m_project) {
            m_project->removeColorStyle(style.name);
        }
    });
    layout->addWidget(delBtn);

    return row;
}

void StylesPanel::onAddStyleClicked() {
    if (!m_project) return;

    bool ok = false;
    QString name = QInputDialog::getText(this, "New Color Style", "Style Name:", QLineEdit::Normal, "Secondary", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        name = name.trimmed();
        if (m_project->hasColorStyle(name)) {
            name += "_new";
        }
        m_project->addColorStyle({name, Theme::cAccent()});
    }
}
