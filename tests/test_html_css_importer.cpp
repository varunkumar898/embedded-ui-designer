#include <QTest>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include "importers/HtmlCssImporter.h"
#include "ir/IRAdapter.h"
#include "ir/IRSerialization.h"
#include "project/Project.h"

using namespace Importers;
using namespace IR;

class TestHtmlCssImporter : public QObject {
    Q_OBJECT

private:
    QString fixturePath(const QString& rel) {
        QStringList candidates = {
            QDir::current().filePath("tests/fixtures/html/" + rel),
            QDir::current().filePath("../tests/fixtures/html/" + rel),
            QDir::current().filePath("../../tests/fixtures/html/" + rel),
            QString("/home/cherry/embedded-ui-designer/tests/fixtures/html/" + rel)
        };
        for (const QString& p : candidates) {
            if (QFileInfo::exists(p)) return p;
        }
        return candidates.last();
    }

private slots:
    void testSimpleButton();
    void testFlexboxLayout();
    void testGridLayout();
    void testTextInput();
    void testImageGallery();
    void testComplexDashboard();
    void testUnsupportedFeatures();
    void testCssOnly();
    void testMalformedHtml();
    void testMissingAttributes();
    void testRoundTripHtmlToIRToProject();
};

void TestHtmlCssImporter::testSimpleButton() {
    QString path = fixturePath("simple_button.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    QCOMPARE(result.ir.screens.size(), 1);

    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& btn = screen.widgets.first();
    QCOMPARE(btn.componentType, QString("Button"));
    QCOMPARE(btn.label, QString("Click me"));

    // Verify button color matches CSS #007acc
    QVERIFY(btn.effects.backgroundColor.isValid());
    QCOMPARE(btn.effects.backgroundColor.solidColor, QColor("#007acc"));
}

void TestHtmlCssImporter::testFlexboxLayout() {
    QString path = fixturePath("flexbox_layout.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    QCOMPARE(result.ir.screens.size(), 1);

    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& container = screen.widgets.first();
    QCOMPARE(container.componentType, QString("Container"));
    QCOMPARE(container.layout.type, IRLayoutType::Flex);
    QCOMPARE(container.layout.flexDirection, IRFlexDirection::Row);
    QCOMPARE(container.children.size(), 3);

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(container.children[i].componentType, QString("Button"));
    }
}

void TestHtmlCssImporter::testGridLayout() {
    QString path = fixturePath("grid_layout.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    QCOMPARE(result.ir.screens.size(), 1);

    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& grid = screen.widgets.first();
    QCOMPARE(grid.componentType, QString("Container"));
    QCOMPARE(grid.layout.type, IRLayoutType::Grid);
    QCOMPARE(grid.properties["columns"].toInt(), 2);
    QCOMPARE(grid.properties["rows"].toInt(), 2);
    QCOMPARE(grid.children.size(), 4);
}

void TestHtmlCssImporter::testTextInput() {
    QString path = fixturePath("text_input.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& input = screen.widgets.first();
    QCOMPARE(input.componentType, QString("TextInput"));
    QCOMPARE(input.label, QString("Enter username..."));
    QCOMPARE(input.properties["width"].toDouble(), 220.0);
    QCOMPARE(input.properties["height"].toDouble(), 36.0);
}

void TestHtmlCssImporter::testImageGallery() {
    QString path = fixturePath("image_gallery.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& gallery = screen.widgets.first();
    QCOMPARE(gallery.componentType, QString("Container"));
    QCOMPARE(gallery.layout.type, IRLayoutType::Flex);
    QCOMPARE(gallery.children.size(), 4);

    for (int i = 0; i < 4; ++i) {
        const IRWidget& img = gallery.children[i];
        QCOMPARE(img.componentType, QString("Image"));
        QCOMPARE(img.properties["imagePath"].toString(), QString("images/img%1.png").arg(i + 1));
    }
}

void TestHtmlCssImporter::testComplexDashboard() {
    QString path = fixturePath("complex_dashboard.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& dash = screen.widgets.first();
    QCOMPARE(dash.componentType, QString("Container"));
    QVERIFY(dash.children.size() >= 2); // header and content

    const IRWidget& content = dash.children[1];
    QCOMPARE(content.componentType, QString("Container"));

    // Check that button, input, image, and table are recognized
    QStringList types;
    for (const IRWidget& w : content.children) {
        types << w.componentType;
    }
    QVERIFY(types.contains("Button"));
    QVERIFY(types.contains("TextInput"));
    QVERIFY(types.contains("Image"));
    QVERIFY(types.contains("Table"));
}

void TestHtmlCssImporter::testUnsupportedFeatures() {
    QString path = fixturePath("unsupported_features.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    // Graceful degradation: success is still true
    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));

    // Warnings must be recorded for canvas, video, transform
    QVERIFY(result.warnings.size() > 0);

    // Supported button element should still be converted
    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);
    const IRWidget& box = screen.widgets.first();

    bool foundButton = false;
    for (const IRWidget& w : box.children) {
        if (w.componentType == "Button" && w.label == "Still Works") {
            foundButton = true;
            break;
        }
    }
    QVERIFY(foundButton);
}

void TestHtmlCssImporter::testCssOnly() {
    QString path = fixturePath("css_only.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    QVERIFY2(result.success, qPrintable(result.errors.join("; ")));
    const IRScreen& screen = result.ir.screens.first();
    QCOMPARE(screen.widgets.size(), 1);

    const IRWidget& btn = screen.widgets.first();
    QCOMPARE(btn.componentType, QString("Button"));
    QCOMPARE(btn.label, QString("Styled via Tag Rule"));
    QCOMPARE(btn.effects.backgroundColor.solidColor, QColor("#28a745"));
}

void TestHtmlCssImporter::testMalformedHtml() {
    QString path = fixturePath("malformed.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    // No crash, success true with graceful recovery
    QVERIFY(result.success);
    QVERIFY(result.ir.screens.size() >= 1);
}

void TestHtmlCssImporter::testMissingAttributes() {
    QString path = fixturePath("errors/missing_attributes.html");
    ImportResult result = HtmlCssImporter::importHtml(path);

    // Components created with defaults, no crash
    QVERIFY(result.success);
    const IRScreen& screen = result.ir.screens.first();
    QVERIFY(screen.widgets.size() >= 1);
}

void TestHtmlCssImporter::testRoundTripHtmlToIRToProject() {
    QString path = fixturePath("complex_dashboard.html");
    ImportResult result = HtmlCssImporter::importHtml(path);
    QVERIFY(result.success);

    // HTML -> IR -> Project
    std::unique_ptr<Project> project = IRToProjectAdapter::convert(result.ir);
    QVERIFY(project != nullptr);
    QCOMPARE(project->screenCount(), 1);

    Screen* screen = project->activeScreen();
    QVERIFY(screen != nullptr);
    QCOMPARE(screen->components().size(), 1);

    // Round-trip back to IR
    IRProject irBack = ProjectToIRAdapter::convert(*project);
    QCOMPARE(irBack.screens.size(), 1);
    QCOMPARE(irBack.screens[0].widgets.size(), 1);
}

QTEST_MAIN(TestHtmlCssImporter)
#include "test_html_css_importer.moc"
