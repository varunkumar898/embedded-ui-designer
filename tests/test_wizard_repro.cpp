// Temporary reproduction harness for the "choose a board, press Next" crash.
// Walks every board in BoardRegistry, every device in CapabilityDatabase, and
// every row of each wizard table through the select-then-Next path under
// ASan/UBSan.
#include <QTest>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>

#include "dialogs/HardwareWizardDialog.h"
#include "hardware/BoardRegistry.h"
#include "hardware/DeviceDatabase.h"
#include "analyzer/CapabilityDatabase.h"

class TestWizardRepro : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void everyWizardBoardRow();
    void everyWizardDeviceRow();
    void everyWizardAllRow();
    void everyRegistryBoard();
    void everyCapabilityDevice();
    void everyWizardBoardFullFlow();
    void everyWizardDeviceFullFlow();
    void everyWizardAllFullFlow();
};

void TestWizardRepro::initTestCase() {
    Hardware::DeviceDatabase::instance().initialize();
    BoardRegistry::instance().loadFromBoardsJson();
}

static QPushButton* nextButton(HardwareWizardDialog* wizard) {
    for (QPushButton* b : wizard->findChildren<QPushButton*>()) {
        if (b->text().contains("Next")) return b;
    }
    return nullptr;
}

static QStackedWidget* stepStack(HardwareWizardDialog* wizard) {
    return wizard->findChild<QStackedWidget*>(QString(), Qt::FindDirectChildrenOnly);
}

static void pressNext(HardwareWizardDialog& wizard, const QString& what) {
    QPushButton* next = nextButton(&wizard);
    QVERIFY2(next, qPrintable("no Next button for " + what));
    next->click();
    QCoreApplication::processEvents();
    QStackedWidget* stack = stepStack(&wizard);
    QVERIFY2(stack, qPrintable("no step stack for " + what));
    QCOMPARE(stack->currentIndex(), 1);
}

// Select every row of the given table in a fresh wizard and press Next.
static void walkTable(const QString& objectName, int tabIndex) {
    QStringList rowLabels;
    {
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        QVERIFY(tabs);
        tabs->setCurrentIndex(tabIndex);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>(objectName);
        QVERIFY2(table, qPrintable("missing table " + objectName));
        for (int r = 0; r < table->rowCount(); ++r) {
            rowLabels << (table->item(r, 0) ? table->item(r, 0)->text() : QString("<null>"));
        }
        qInfo().noquote() << "table" << objectName << "rows:" << rowLabels.size();
    }

    for (const QString& label : rowLabels) {
        qInfo().noquote() << "REPRO" << objectName << "row:" << label;
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(tabIndex);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>(objectName);
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->item(i, 0) && table->item(i, 0)->text() == label) { row = i; break; }
        }
        QVERIFY2(row >= 0, qPrintable("row vanished: " + label));
        table->selectRow(row);
        QCoreApplication::processEvents();
        pressNext(wizard, label);
    }
}

void TestWizardRepro::everyWizardBoardRow() {
    walkTable("boardHardwareTable", 2);
}

void TestWizardRepro::everyWizardDeviceRow() {
    walkTable("mcuHardwareTable", 1);
}

void TestWizardRepro::everyWizardAllRow() {
    walkTable("allHardwareTable", 0);
}

void TestWizardRepro::everyRegistryBoard() {
    const QList<BoardDef> boards = BoardRegistry::instance().boards();
    QVERIFY(!boards.isEmpty());
    for (const BoardDef& board : boards) {
        qInfo().noquote() << "REPRO BoardRegistry board:" << board.id << "mcu:" << board.mcu;
        HardwareWizardDialog wizard;
        // Try the Boards tab first: match by board id/name or by the MCU part.
        QTableWidget* table = wizard.findChild<QTableWidget*>("boardHardwareTable");
        QVERIFY(table);
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(2);
        QCoreApplication::processEvents();

        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i) {
            const QString label = table->item(i, 0) ? table->item(i, 0)->text() : QString();
            const QString mcu = table->item(i, 5) ? table->item(i, 5)->text() : QString();
            if (label.compare(board.displayName, Qt::CaseInsensitive) == 0 ||
                mcu.compare(board.mcu, Qt::CaseInsensitive) == 0) {
                row = i;
                break;
            }
        }
        if (row < 0) {
            qWarning().noquote() << "no wizard board row matches BoardRegistry board:" << board.id;
            continue;
        }
        table->selectRow(row);
        QCoreApplication::processEvents();
        pressNext(wizard, board.id);
    }
}

void TestWizardRepro::everyCapabilityDevice() {
    CapabilityDatabase db;
    db.loadBuiltinDatabase();
    const QStringList names = db.listAllDevices();
    QVERIFY(!names.isEmpty());

    QStringList wizardDevices;
    {
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>("mcuHardwareTable");
        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->item(i, 0)) wizardDevices << table->item(i, 0)->text();
        }
    }

    for (const QString& name : names) {
        qInfo().noquote() << "REPRO capability device:" << name;
        QString matched;
        for (const QString& d : wizardDevices) {
            if (d.compare(name, Qt::CaseInsensitive) == 0 ||
                name.startsWith(d, Qt::CaseInsensitive) ||
                d.startsWith(name, Qt::CaseInsensitive)) {
                matched = d;
                break;
            }
        }
        if (matched.isEmpty()) {
            qWarning().noquote() << "no wizard device row matches capability device:" << name;
            continue;
        }

        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>("mcuHardwareTable");
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->item(i, 0) && table->item(i, 0)->text() == matched) { row = i; break; }
        }
        QVERIFY2(row >= 0, qPrintable("device row missing: " + matched));
        table->selectRow(row);
        QCoreApplication::processEvents();
        pressNext(wizard, name);
    }
}


static bool advanceToReview(HardwareWizardDialog& wizard) {
    QStackedWidget* stack = wizard.findChild<QStackedWidget*>();
    if (!stack) return false;
    for (int i = 0; i < 5; ++i) {
        QPushButton* next = nextButton(&wizard);
        if (!next || !next->isEnabled()) break;
        next->click();
        QCoreApplication::processEvents();
    }
    return stack->currentIndex() == 5;
}

void TestWizardRepro::everyWizardBoardFullFlow() {
    // Every row of the Boards tab must survive Next...Next through the review step.
    HardwareWizardDialog w0;
    QTabWidget* t0 = w0.findChild<QTabWidget*>();
    QVERIFY(t0);
    t0->setCurrentIndex(2);
    QCoreApplication::processEvents();
    QTableWidget* t = w0.findChild<QTableWidget*>("boardHardwareTable");
    QVERIFY(t);
    QStringList labels;
    for (int r = 0; r < t->rowCount(); ++r) labels << t->item(r, 0)->text();

    for (const QString& label : labels) {
        qInfo().noquote() << "FULLFLOW board:" << label;
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(2);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>("boardHardwareTable");
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i)
            if (table->item(i, 0)->text() == label) { row = i; break; }
        QVERIFY2(row >= 0, qPrintable("missing row " + label));
        table->selectRow(row);
        QCoreApplication::processEvents();
        QVERIFY2(advanceToReview(wizard), qPrintable("did not reach review for " + label));
        // Sensible final state: review step is reachable without crashing.
        QCOMPARE(wizard.findChild<QStackedWidget*>()->currentIndex(), 5);
    }
}

void TestWizardRepro::everyWizardDeviceFullFlow() {
    HardwareWizardDialog w0;
    QTabWidget* t0 = w0.findChild<QTabWidget*>();
    QVERIFY(t0);
    t0->setCurrentIndex(1);
    QCoreApplication::processEvents();
    QTableWidget* t = w0.findChild<QTableWidget*>("mcuHardwareTable");
    QVERIFY(t);
    QStringList labels;
    for (int r = 0; r < t->rowCount(); ++r) labels << t->item(r, 0)->text();

    for (const QString& label : labels) {
        qInfo().noquote() << "FULLFLOW device:" << label;
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>("mcuHardwareTable");
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i)
            if (table->item(i, 0)->text() == label) { row = i; break; }
        QVERIFY2(row >= 0, qPrintable("missing row " + label));
        table->selectRow(row);
        QCoreApplication::processEvents();
        QVERIFY2(advanceToReview(wizard), qPrintable("did not reach review for " + label));
    }
}

void TestWizardRepro::everyWizardAllFullFlow() {
    HardwareWizardDialog w0;
    QTabWidget* t0 = w0.findChild<QTabWidget*>();
    QVERIFY(t0);
    t0->setCurrentIndex(0);
    QCoreApplication::processEvents();
    QTableWidget* t = w0.findChild<QTableWidget*>("allHardwareTable");
    QVERIFY(t);
    QStringList labels;
    for (int r = 0; r < t->rowCount(); ++r) labels << t->item(r, 0)->text();

    for (const QString& label : labels) {
        qInfo().noquote() << "FULLFLOW all:" << label;
        HardwareWizardDialog wizard;
        QTabWidget* tabs = wizard.findChild<QTabWidget*>();
        tabs->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QTableWidget* table = wizard.findChild<QTableWidget*>("allHardwareTable");
        int row = -1;
        for (int i = 0; i < table->rowCount(); ++i)
            if (table->item(i, 0)->text() == label) { row = i; break; }
        QVERIFY2(row >= 0, qPrintable("missing row " + label));
        table->selectRow(row);
        QCoreApplication::processEvents();
        // All tab's selection handler may not set type; force current table selection trigger.
        QVERIFY2(advanceToReview(wizard), qPrintable("did not reach review for " + label));
    }
}

QTEST_MAIN(TestWizardRepro)
#include "test_wizard_repro.moc"
