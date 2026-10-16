#include "ew_hydrate.h"
#include <QFileInfo>
#include <QTextStream>
#include <cstring>

namespace EmbedWeb {

IRHydrator::IRHydrator() {}

bool IRHydrator::isSupportedComponentType(const QString& rawType) {
    QString t = rawType.trimmed().toLower();
    if (t == "container" || t == "group" || t == "box" || t == "panel") return true;
    if (t == "rectangle" || t == "rect") return true;
    if (t == "label" || t == "text" || t == "p" || t == "span" || t == "h1" || t == "h2" || t == "h3") return true;
    if (t == "button") return true;
    if (t == "progressbar" || t == "linearprogress" || t == "progress") return true;
    if (t == "image" || t == "img") return true;
    return false;
}

ew_widget_type_t IRHydrator::mapWidgetType(const QString& rawType) {
    QString t = rawType.trimmed().toLower();
    if (t == "container" || t == "group" || t == "box" || t == "panel") return EW_WIDGET_CONTAINER;
    if (t == "rectangle" || t == "rect") return EW_WIDGET_RECTANGLE;
    if (t == "label" || t == "text" || t == "p" || t == "span" || t == "h1" || t == "h2" || t == "h3") return EW_WIDGET_LABEL;
    if (t == "button") return EW_WIDGET_BUTTON;
    if (t == "progressbar" || t == "linearprogress" || t == "progress") return EW_WIDGET_PROGRESS_BAR;
    if (t == "image" || t == "img") return EW_WIDGET_IMAGE;
    return EW_WIDGET_UNSUPPORTED;
}

QString IRHydrator::escapeCStringLiteral(const QString& s) {
    QString out;
    out.reserve(s.size() * 2);
    for (int i = 0; i < s.size(); ++i) {
        QChar c = s.at(i);
        ushort u = c.unicode();
        if (c == QLatin1Char('\\')) {
            out += "\\\\";
        } else if (c == QLatin1Char('"')) {
            out += "\\\"";
        } else if (c == QLatin1Char('\n')) {
            out += "\\n";
        } else if (c == QLatin1Char('\r')) {
            out += "\\r";
        } else if (c == QLatin1Char('\t')) {
            out += "\\t";
        } else if (u < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\%03o", (unsigned int)u);
            out += QString::fromLatin1(buf);
        } else {
            out += c;
        }
    }
    return out;
}

static QString escapeCStringLiteral(const QString& s) {
    return IRHydrator::escapeCStringLiteral(s);
}

static int countWidgetsRecursive(const QList<IR::IRWidget>& widgets) {
    int count = 0;
    for (const auto& w : widgets) {
        count += 1;
        if (!w.children.isEmpty()) {
            count += countWidgetsRecursive(w.children);
        }
    }
    return count;
}

static const IR::IRAsset* findAsset(const IR::IRProject& project, const QString& ref) {
    if (ref.isEmpty()) return nullptr;
    for (const auto& a : project.assets) {
        if (a.id.toString() == ref ||
            a.filePath == ref ||
            a.originalPath == ref ||
            a.name == ref ||
            QFileInfo(a.filePath).fileName() == ref) {
            return &a;
        }
    }
    return nullptr;
}

static QString extractImageReference(const IR::IRWidget& w) {
    if (w.properties.contains("imagePath")) return w.properties["imagePath"].toString();
    if (w.properties.contains("assetId")) return w.properties["assetId"].toString();
    if (w.properties.contains("src")) return w.properties["src"].toString();
    if (w.properties.contains("asset")) return w.properties["asset"].toString();
    return QString();
}

static bool validateWidgetTree(const IR::IRProject& project, const IR::IRWidget& w, QList<HydrationDiagnostic>& diags) {
    bool ok = true;
    if (!IRHydrator::isSupportedComponentType(w.componentType)) {
        HydrationDiagnostic d;
        d.level = HydrationDiagnostic::Level::Error;
        d.widgetId = w.id.toString();
        d.widgetType = w.componentType;
        d.message = QString("Unsupported component type '%1' (widget '%2') for EmbedWeb runtime. EmbedWeb supports Container, Rectangle, Label, Button, ProgressBar, and Image.")
                    .arg(w.componentType, w.name.isEmpty() ? w.id.toString() : w.name);
        diags.append(d);
        ok = false;
    } else {
        ew_widget_type_t t = IRHydrator::mapWidgetType(w.componentType);
        if (t == EW_WIDGET_IMAGE) {
            QString ref = extractImageReference(w);
            if (ref.isEmpty() || !findAsset(project, ref)) {
                HydrationDiagnostic d;
                d.level = HydrationDiagnostic::Level::Error;
                d.widgetId = w.id.toString();
                d.widgetType = w.componentType;
                d.message = QString("Image widget '%1' references missing or unpackaged asset '%2'.")
                            .arg(w.name.isEmpty() ? w.id.toString() : w.name, ref);
                diags.append(d);
                ok = false;
            }
        }
    }

    for (const auto& child : w.children) {
        if (!validateWidgetTree(project, child, diags)) {
            ok = false;
        }
    }
    return ok;
}

static bool populateNodeFromWidget(ew_tree_t* tree, int16_t parent_idx, const IR::IRProject& project, const IR::IRWidget& w, QList<HydrationDiagnostic>& diags) {
    ew_widget_type_t type = IRHydrator::mapWidgetType(w.componentType);
    int16_t idx = ew_tree_alloc_node(tree, type);
    if (idx < 0) {
        HydrationDiagnostic d;
        d.level = HydrationDiagnostic::Level::Error;
        d.message = "Failed to allocate runtime tree node slot: node pool exhausted.";
        diags.append(d);
        return false;
    }

    ew_node_t* node = ew_tree_get_node(tree, idx);
    if (!node) return false;

    // Attach to parent
    if (parent_idx >= 0) {
        if (!ew_tree_add_child(tree, parent_idx, idx)) {
            HydrationDiagnostic d;
            d.level = HydrationDiagnostic::Level::Error;
            d.message = QString("Failed to attach child node %1 to parent %2.").arg(idx).arg(parent_idx);
            diags.append(d);
            return false;
        }
    }

    // Geometry & Layout
    int16_t req_w = -1;
    if (w.properties.contains("width")) req_w = (int16_t)w.properties["width"].toInt();
    else if (w.properties.contains("w")) req_w = (int16_t)w.properties["w"].toInt();

    int16_t req_h = -1;
    if (w.properties.contains("height")) req_h = (int16_t)w.properties["height"].toInt();
    else if (w.properties.contains("h")) req_h = (int16_t)w.properties["h"].toInt();

    int16_t x = 0;
    if (w.layout.hasLeft) x = (int16_t)w.layout.left;
    else if (w.properties.contains("x")) x = (int16_t)w.properties["x"].toInt();

    int16_t y = 0;
    if (w.layout.hasTop) y = (int16_t)w.layout.top;
    else if (w.properties.contains("y")) y = (int16_t)w.properties["y"].toInt();

    node->req_w = req_w;
    node->req_h = req_h;
    node->rect.x = x;
    node->rect.y = y;
    node->rect.w = (req_w > 0) ? req_w : 0;
    node->rect.h = (req_h > 0) ? req_h : 0;

    // Flex layout attributes
    if (w.layout.type == IR::IRLayoutType::Flex) {
        node->flex_direction = (w.layout.flexDirection == IR::IRFlexDirection::Column) ? EW_FLEX_DIR_COLUMN : EW_FLEX_DIR_ROW;
        if (w.layout.justifyContent == IR::IRJustifyContent::Center) node->justify = EW_JUSTIFY_CENTER;
        else if (w.layout.justifyContent == IR::IRJustifyContent::FlexEnd) node->justify = EW_JUSTIFY_END;
        else if (w.layout.justifyContent == IR::IRJustifyContent::SpaceBetween) node->justify = EW_JUSTIFY_SPACE_BETWEEN;
        else if (w.layout.justifyContent == IR::IRJustifyContent::SpaceAround) node->justify = EW_JUSTIFY_SPACE_AROUND;
        else node->justify = EW_JUSTIFY_START;

        if (w.layout.alignItems == IR::IRAlignItems::Center) node->align = EW_ALIGN_CENTER;
        else if (w.layout.alignItems == IR::IRAlignItems::FlexEnd) node->align = EW_ALIGN_END;
        else if (w.layout.alignItems == IR::IRAlignItems::Stretch) node->align = EW_ALIGN_STRETCH;
        else node->align = EW_ALIGN_START;

        node->style.gap = (uint8_t)qBound(0, (int)w.layout.gap, 255);
    }

    // Styles
    if (w.effects.backgroundColor.isValid() && !w.effects.backgroundColor.isTransparent()) {
        node->style.has_bg = 1;
        QColor c = w.effects.backgroundColor.solidColor;
        node->style.bg_color = ew_color_rgb565(c.red(), c.green(), c.blue());
    }

    if (w.effects.border.width > 0) {
        node->style.has_border = 1;
        node->style.border_width = (uint8_t)qBound(1, (int)w.effects.border.width, 255);
        if (w.effects.border.color.isValid()) {
            QColor c = w.effects.border.color.solidColor;
            node->style.border_color = ew_color_rgb565(c.red(), c.green(), c.blue());
        }
    }

    node->style.border_radius = (uint8_t)qBound(0, w.effects.borderRadius, 255);
    node->style.opacity = (uint8_t)(qBound(0.0, w.effects.opacity, 1.0) * 255.0);

    // Specific widget payloads
    if (type == EW_WIDGET_BUTTON || type == EW_WIDGET_LABEL) {
        QString txt = !w.label.isEmpty() ? w.label : w.properties["text"].toString();
        if (!txt.isEmpty()) {
            std::strncpy(node->text, txt.toUtf8().constData(), EW_MAX_TEXT_LEN - 1);
            node->text[EW_MAX_TEXT_LEN - 1] = '\0';
        }
        if (w.textStyle.color.isValid()) {
            QColor c = w.textStyle.color.solidColor;
            node->style.text_color = ew_color_rgb565(c.red(), c.green(), c.blue());
        } else {
            node->style.text_color = EW_COLOR_WHITE;
        }
        if (type == EW_WIDGET_BUTTON) {
            node->touchable = 1;
        }
    } else if (type == EW_WIDGET_PROGRESS_BAR) {
        int val = 0;
        if (w.properties.contains("value")) val = w.properties["value"].toInt();
        else if (w.properties.contains("progress")) val = w.properties["progress"].toInt();
        node->progress_value = (int16_t)qBound(0, val, 100);
    } else if (type == EW_WIDGET_IMAGE) {
        // Asset resolved during validation
    }

    // Children
    for (const auto& child : w.children) {
        if (!populateNodeFromWidget(tree, idx, project, child, diags)) {
            return false;
        }
    }

    return true;
}

HydrationResult IRHydrator::hydrate(const IR::IRProject& project, int screenIndex) const {
    HydrationResult result;
    ew_tree_init(&result.tree);

    // 1. Run Universal IR validation
    IR::IRDiagnostics irDiags = project.validate();
    for (const auto& d : irDiags) {
        if (d.severity == IR::IRDiagnostic::Severity::Error) {
            HydrationDiagnostic hd;
            hd.level = HydrationDiagnostic::Level::Error;
            hd.message = QString("IR validation error: %1").arg(d.message);
            result.diagnostics.append(hd);
        }
    }
    if (!result.errorMessages().isEmpty()) {
        result.success = false;
        return result;
    }

    // 2. Select target screen
    if (project.screens.isEmpty()) {
        HydrationDiagnostic hd;
        hd.level = HydrationDiagnostic::Level::Error;
        hd.message = "Project contains no screens for hydration.";
        result.diagnostics.append(hd);
        result.success = false;
        return result;
    }

    const IR::IRScreen* targetScreen = nullptr;
    if (screenIndex >= 0 && screenIndex < project.screens.size()) {
        targetScreen = &project.screens[screenIndex];
    } else {
        // Match activeScreenId if set, otherwise first screen
        for (const auto& s : project.screens) {
            if (s.id == project.activeScreenId) {
                targetScreen = &s;
                break;
            }
        }
        if (!targetScreen) targetScreen = &project.screens.first();
    }

    // 3. Check node capacity (Root container + all widgets)
    int totalNodes = 1 + countWidgetsRecursive(targetScreen->widgets);
    if (totalNodes > EW_MAX_NODES) {
        HydrationDiagnostic hd;
        hd.level = HydrationDiagnostic::Level::Error;
        hd.message = QString("Widget count (%1) exceeds maximum bounded node pool capacity of %2.")
                     .arg(totalNodes).arg(EW_MAX_NODES);
        result.diagnostics.append(hd);
        result.success = false;
        return result;
    }

    // 4. Validate all components and assets in tree
    for (const auto& w : targetScreen->widgets) {
        if (!validateWidgetTree(project, w, result.diagnostics)) {
            result.success = false;
        }
    }
    if (!result.errorMessages().isEmpty()) {
        result.success = false;
        return result;
    }

    // 5. Build Root Node for Screen
    int16_t rootIdx = ew_tree_alloc_node(&result.tree, EW_WIDGET_CONTAINER);
    if (rootIdx < 0) {
        HydrationDiagnostic hd;
        hd.level = HydrationDiagnostic::Level::Error;
        hd.message = "Failed to allocate root screen node in runtime tree.";
        result.diagnostics.append(hd);
        result.success = false;
        return result;
    }

    ew_node_t* rootNode = ew_tree_get_node(&result.tree, rootIdx);
    rootNode->req_w = (int16_t)targetScreen->width;
    rootNode->req_h = (int16_t)targetScreen->height;
    rootNode->rect.x = 0;
    rootNode->rect.y = 0;
    rootNode->rect.w = (int16_t)targetScreen->width;
    rootNode->rect.h = (int16_t)targetScreen->height;

    if (targetScreen->backgroundColor.isValid() && !targetScreen->backgroundColor.isTransparent()) {
        rootNode->style.has_bg = 1;
        QColor c = targetScreen->backgroundColor.solidColor;
        rootNode->style.bg_color = ew_color_rgb565(c.red(), c.green(), c.blue());
    }

    // 6. Populate child widgets
    for (const auto& w : targetScreen->widgets) {
        if (!populateNodeFromWidget(&result.tree, rootIdx, project, w, result.diagnostics)) {
            result.success = false;
            return result;
        }
    }

    // 7. Compute layout
    ew_layout_compute(&result.tree, rootIdx, (int16_t)targetScreen->width, (int16_t)targetScreen->height);

    result.success = true;
    return result;
}

QString IRHydrator::generateStaticTreeHeader(const IR::IRProject& project, int screenIndex) const {
    Q_UNUSED(screenIndex);
    QString code;
    QTextStream out(&code);

    out << "/**\n"
        << " * @file ui_tree.h\n"
        << " * @brief Auto-generated EmbedWeb static UI tree declaration\n"
        << " * Target: " << (project.targetProfile.mcu.isEmpty() ? "Embedded MCU" : project.targetProfile.mcu) << "\n"
        << " * Zero dynamic allocation; zero runtime JSON parsing.\n"
        << " */\n"
        << "#ifndef UI_TREE_H\n"
        << "#define UI_TREE_H\n\n"
        << "#include \"ew_types.h\"\n"
        << "#include \"ew_tree.h\"\n"
        << "#include \"ew_layout.h\"\n\n"
        << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"
        << "#define UI_SCREEN_WIDTH  " << project.displayWidth << "\n"
        << "#define UI_SCREEN_HEIGHT " << project.displayHeight << "\n\n"
        << "/**\n"
        << " * @brief Initialize static EmbedWeb UI tree.\n"
        << " * Instantiates bounded POD tree and computes resolved geometry.\n"
        << " */\n"
        << "void ui_tree_init(ew_tree_t* tree);\n\n"
        << "#ifdef __cplusplus\n}\n#endif\n\n"
        << "#endif /* UI_TREE_H */\n";

    return code;
}

static void emitWidgetCode(QTextStream& out, const ew_tree_t& tree, int16_t idx) {
    if (idx < 0 || idx >= tree.count) return;
    const ew_node_t* n = &tree.nodes[idx];

    out << "    /* Node " << idx << ": type=" << (int)n->type << ", text=\"" << n->text << "\" */\n"
        << "    {\n"
        << "        ew_node_t* n = &tree->nodes[" << idx << "];\n"
        << "        n->id = " << n->id << ";\n"
        << "        n->type = " << (int)n->type << ";\n"
        << "        n->visible = " << (int)n->visible << ";\n"
        << "        n->rect.x = " << n->rect.x << "; n->rect.y = " << n->rect.y << ";\n"
        << "        n->rect.w = " << n->rect.w << "; n->rect.h = " << n->rect.h << ";\n"
        << "        n->req_w = " << n->req_w << "; n->req_h = " << n->req_h << ";\n"
        << "        n->flex_direction = " << (int)n->flex_direction << ";\n"
        << "        n->justify = " << (int)n->justify << ";\n"
        << "        n->align = " << (int)n->align << ";\n"
        << "        n->parent_idx = " << n->parent_idx << ";\n"
        << "        n->first_child_idx = " << n->first_child_idx << ";\n"
        << "        n->next_sibling_idx = " << n->next_sibling_idx << ";\n"
        << "        n->style.bg_color = 0x" << QString::number(n->style.bg_color, 16).toUpper() << ";\n"
        << "        n->style.border_color = 0x" << QString::number(n->style.border_color, 16).toUpper() << ";\n"
        << "        n->style.text_color = 0x" << QString::number(n->style.text_color, 16).toUpper() << ";\n"
        << "        n->style.border_width = " << (int)n->style.border_width << ";\n"
        << "        n->style.border_radius = " << (int)n->style.border_radius << ";\n"
        << "        n->style.opacity = " << (int)n->style.opacity << ";\n"
        << "        n->style.has_bg = " << (int)n->style.has_bg << ";\n"
        << "        n->style.has_border = " << (int)n->style.has_border << ";\n"
        << "        n->style.gap = " << (int)n->style.gap << ";\n"
        << "        n->progress_value = " << n->progress_value << ";\n"
        << "        n->touchable = " << (int)n->touchable << ";\n";
    if (n->text[0] != '\0') {
        out << "        strncpy(n->text, \"" << escapeCStringLiteral(QString::fromUtf8(n->text)) << "\", EW_MAX_TEXT_LEN);\n";
    }
    out << "    }\n";
}

QString IRHydrator::generateStaticTreeSource(const IR::IRProject& project, int screenIndex, QString* outError) const {
    HydrationResult hr = hydrate(project, screenIndex);
    if (!hr.success) {
        if (outError) *outError = hr.errorMessages().join("; ");
        return QString();
    }

    QString code;
    QTextStream out(&code);

    out << "/**\n"
        << " * @file ui_tree.c\n"
        << " * @brief Auto-generated static UI tree implementation\n"
        << " * Bounded static memory footprint; zero runtime allocations.\n"
        << " */\n"
        << "#include \"ui_tree.h\"\n"
        << "#include <string.h>\n\n"
        << "void ui_tree_init(ew_tree_t* tree) {\n"
        << "    if (!tree) return;\n"
        << "    ew_tree_init(tree);\n"
        << "    tree->count = " << hr.tree.count << ";\n"
        << "    tree->root_idx = " << hr.tree.root_idx << ";\n\n";

    for (int16_t i = 0; i < hr.tree.count; i++) {
        emitWidgetCode(out, hr.tree, i);
    }

    out << "\n    /* Run layout engine to finalize geometry */\n"
        << "    ew_layout_compute(tree, tree->root_idx, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);\n"
        << "}\n";

    return code;
}

QString IRHydrator::generateStaticAssetsHeader(const IR::IRProject& project) const {
    QString code;
    QTextStream out(&code);

    out << "/**\n"
        << " * @file ui_assets.h\n"
        << " * @brief Packaged embedded image and font asset declarations\n"
        << " */\n"
        << "#ifndef UI_ASSETS_H\n"
        << "#define UI_ASSETS_H\n\n"
        << "#include \"ew_types.h\"\n\n"
        << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";

    int imgIdx = 0;
    for (const auto& a : project.assets) {
        if (a.type == IR::IRAssetType::Image) {
            QString sym = QString("img_asset_%1").arg(imgIdx++);
            out << "extern const ew_image_asset_t " << sym << ";\n";
        }
    }

    out << "\n#ifdef __cplusplus\n}\n#endif\n\n"
        << "#endif /* UI_ASSETS_H */\n";

    return code;
}

QString IRHydrator::generateStaticAssetsSource(const IR::IRProject& project) const {
    QString code;
    QTextStream out(&code);

    out << "/**\n"
        << " * @file ui_assets.c\n"
        << " * @brief Packaged embedded image assets stored in Flash ROM (.rodata)\n"
        << " */\n"
        << "#include \"ui_assets.h\"\n\n";

    int imgIdx = 0;
    for (const auto& a : project.assets) {
        if (a.type == IR::IRAssetType::Image) {
            QString pixSym = QString("img_pixels_%1").arg(imgIdx);
            QString assetSym = QString("img_asset_%1").arg(imgIdx);
            imgIdx++;

            int w = (a.width > 0) ? a.width : 16;
            int h = (a.height > 0) ? a.height : 16;

            out << "/* Asset: " << a.name << " (" << w << "x" << h << ") */\n"
                << "static const uint16_t " << pixSym << "[" << (w * h) << "] = {\n    0x0000\n};\n\n"
                << "const ew_image_asset_t " << assetSym << " = {\n"
                << "    .width = " << w << ",\n"
                << "    .height = " << h << ",\n"
                << "    .format = EW_IMAGE_FORMAT_RGB565,\n"
                << "    .chroma_key = 0x0001,\n"
                << "    .pixels = " << pixSym << "\n"
                << "};\n\n";
        }
    }

    return code;
}

} // namespace EmbedWeb
