#include "core/reactionvariant.h"
#include "core/variantarchive.h"
#include "data/database.h"
#include "data/schemamanager.h"
#include "data/variantservice.h"
#include "infrastructure/translationmanager.h"
#include "ui/mainwindow.h"
#include "ui/variantslist.h"
#include "ui/widgets/reactiontable.h"
#include "validation/reactionvalidator.h"
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <cstdlib>
#include <iostream>

void require(bool ok, const char *text) {
    if (!ok) {
        std::cerr << "FAIL: " << text << '\n';
        std::exit(1);
    }
}
ReactionVariant sample() {
    ReactionVariant d;
    d.number = 4;
    d.mealy = {2, 3, 4};
    d.moore = {5, 3, 4};
    d.reaction = {{"x2", {"y3"}}, {"x1", {"y0", "y2"}}, {"x3", {}}, {"x2", {"y2", "y0"}}};
    return d;
}
bool equal(const ReactionVariant &a, const ReactionVariant &b) {
    return a.toJson() == b.toJson();
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    TranslationManager translator;
    require(translator.setLocale("ru_RU").ok(), "Russian translation available");
    QTemporaryDir dir;
    require(dir.isValid(), "temporary directory");
    auto data = sample();
    require(data.validate().ok(), "valid ordered reaction");
    auto invalid = data;
    invalid.reaction[1].input = "x10";
    require(!invalid.validate().ok(), "reject x10");
    invalid = data;
    invalid.reaction[1].input = "x1,x2";
    require(!invalid.validate().ok(), "reject multiple inputs");
    invalid = data;
    invalid.reaction[1].outputs = {"y1", "y1"};
    require(!invalid.validate().ok(), "reject duplicate outputs");
    invalid = data;
    invalid.reaction[1].outputs = {"y10"};
    require(!invalid.validate().ok(), "reject y10");
    invalid = data;
    invalid.mealy.inputs = 2;
    require(!invalid.validate().ok(), "declared counts checked");
    invalid = data;
    invalid.moore.outputs = 2;
    require(!invalid.validate().ok(), "both count blocks checked");
    auto boundary = data;
    boundary.mealy.inputs = boundary.moore.inputs = 10;
    boundary.reaction.clear();
    for (int i = 0; i < 10; ++i)
        boundary.reaction.append({QString("x%1").arg(i), {}});
    require(boundary.validate().ok(), "all ten inputs accepted");
    ReactionValidator input(false), output(true);
    int pos = 0;
    for (auto text : QStringList{"x0", "x9"})
        require(input.validate(text, pos) == QValidator::Acceptable, "input validator bounds");
    for (auto text : QStringList{"X1", "x10", "x1,x2", "a1"})
        require(input.validate(text, pos) == QValidator::Invalid, "input validator rejects");
    for (auto text : QStringList{"y9,y0,y2", "y1, y2", "", "-"})
        require(output.validate(text, pos) == QValidator::Acceptable,
                "output validator accepts lists");
    for (auto text : QStringList{"y10", "y1,y1", "y1; y2", "x1"})
        require(output.validate(text, pos) == QValidator::Invalid, "output validator rejects");
    QString partial = "y1,y";
    require(output.validate(partial, pos) == QValidator::Intermediate, "partial output editable");

    const auto dbPath = dir.filePath("variants-lite.db");
    {
        std::unique_ptr<Database> db(Database::open(dbPath));
        require(bool(db), "open database");
        SchemaManager schema(db.get());
        QString error;
        require(schema.createSchema(&error), qPrintable(error));
        VariantRepository repo(db.get());
        VariantService service(db.get(), &repo);
        require(service.saveVariant(data).ok(), "save reaction");
        require(!service.replaceVariant(invalid).ok(), "invalid replacement refused");
        require(equal(data, service.loadVariant(4).payloadAs<ReactionVariant>()),
                "replacement did not modify reaction");
        // Force SQLite to return unordered selects backwards; ordered reading must still work.
        require(db->exec("PRAGMA reverse_unordered_selects=ON"), "reverse unordered selects");
        require(equal(data, service.loadVariant(4).payloadAs<ReactionVariant>()),
                "explicit step order and output order");
        require(service.exportArchive(dir.filePath("test.m2m")).ok(), "encrypted export");
        require(service.swapVariants(4, 8).ok(), "move variant number");
        require(service.importArchive(dir.filePath("test.m2m")).ok(), "encrypted import");
        require(equal(data, service.loadVariant(4).payloadAs<ReactionVariant>()),
                "archive preserves order and counts");
        QFile archive(dir.filePath("test.m2m"));
        require(archive.open(QIODevice::ReadWrite), "open archive for corruption");
        auto bytes = archive.readAll();
        require(bytes.startsWith("M2MLITE1") && !bytes.contains("reaction"),
                "new encrypted format");
        bytes[bytes.size() - 1] = char(bytes.back() ^ 1);
        archive.resize(0);
        archive.write(bytes);
        archive.close();
        require(!service.importArchive(archive.fileName()).ok(), "tampered archive refused");
        require(equal(data, service.loadVariant(4).payloadAs<ReactionVariant>()),
                "failed import leaves database intact");
        // A valid archive that fails during insertion must roll back the old database too.
        require(service.exportArchive(dir.filePath("rollback.m2m")).ok(), "archive for rollback");
        require(db->exec("CREATE TRIGGER fail_import BEFORE INSERT ON ReactionSteps BEGIN SELECT "
                         "RAISE(ABORT,'test failure'); END"),
                "install failure trigger");
        require(!service.importArchive(dir.filePath("rollback.m2m")).ok(),
                "insertion failure detected");
        require(equal(data, service.loadVariant(4).payloadAs<ReactionVariant>()),
                "transaction rolls back failed import");
        require(db->exec("DROP TRIGGER fail_import"), "remove test trigger");
        MainWindow window(&service, &translator, nullptr);
        window.show();
        app.processEvents();
        require(window.findChildren<ReactionTable *>().size() == 1, "only reaction editor");
        require(window.findChildren<QSpinBox *>().size() == 7, "number and six counts, no names");
        for (auto *action : window.menuBar()->actions())
            require(window.menuBar()->actionGeometry(action).width() >=
                        window.menuBar()->fontMetrics().horizontalAdvance(action->text()) + 20,
                    "top menu reserves space for icon and full title");
        QSet<QByteArray> icons;
        for (auto *menu : window.findChildren<QMenu *>())
            for (auto *action : menu->actions()) {
                if (action->isSeparator())
                    continue;
                require(!action->icon().isNull(), "each menu action has icon");
                QImage image = action->icon().pixmap(24, 24).toImage();
                QByteArray pixels(reinterpret_cast<const char *>(image.constBits()),
                                  image.sizeInBytes());
                require(!icons.contains(pixels), "menu icons are visually unique");
                icons.insert(pixels);
            }
        auto *table = window.findChild<ReactionTable *>();
        require(table->columnCount() == 1, "startup has one reaction column");
        require(window.size() == window.minimumSize(), "window opens at minimum permissible size");
        if (argc > 1)
            require(window.grab().save(QString::fromLocal8Bit(argv[1]) + ".startup.png"),
                    "startup screenshot");
        {
            ReactionTable keyboardTable;
            keyboardTable.resize(400, 160);
            keyboardTable.show();
            app.processEvents();
            keyboardTable.item(0, 0)->setText("x2");
            require(keyboardTable.columnCount() == 1, "typing alone does not append a step");
            keyboardTable.setCurrentCell(0, 0);
            QTest::keyClick(&keyboardTable, Qt::Key_Return);
            require(keyboardTable.columnCount() == 2 && keyboardTable.currentColumn() == 1,
                    "Enter after selection inserts exactly one step");
            auto activeEditor = [&keyboardTable]() -> QLineEdit * {
                for (auto *line : keyboardTable.findChildren<QLineEdit *>("reactionCellEditor"))
                    if (line->isVisible())
                        return line;
                return nullptr;
            };
            auto *line = activeEditor();
            require(line != nullptr, "new step is ready to edit");
            QTest::keyClicks(line, "x");
            QTest::keyClick(line, Qt::Key_Return);
            require(keyboardTable.columnCount() == 2, "incomplete signal does not advance");
            QTest::keyClicks(line, "9");
            QTest::keyClick(line, Qt::Key_Enter);
            require(keyboardTable.columnCount() == 3 && keyboardTable.item(0, 1)->text() == "x9",
                    "keypad Enter commits and inserts one step");
            if (auto *current = activeEditor())
                QTest::keyClick(current, Qt::Key_Escape);
            keyboardTable.setSteps(data.reaction);
            keyboardTable.setCurrentCell(1, 1);
            QTest::keyClick(&keyboardTable, Qt::Key_Return);
            require(keyboardTable.columnCount() == 5 && keyboardTable.item(0, 3)->text() == "x3",
                    "Enter in the middle preserves later steps and their order");
            if (auto *current = activeEditor())
                QTest::keyClick(current, Qt::Key_Escape);
            keyboardTable.setSteps({});
            keyboardTable.removeStep();
            require(keyboardTable.columnCount() == 1, "last column cannot leave an empty table");
        }
        {
            VariantsList variants(&service);
            if (argc > 1) {
                variants.show();
                app.processEvents();
                require(variants.grab().save(QString::fromLocal8Bit(argv[1]) + ".variants.png"),
                        "variant list screenshot");
            }
            require(variants.findChild<QGroupBox *>("swapGroup") &&
                        variants.findChild<QComboBox *>("swapFirstComboBox"),
                    "variant browser swap controls are present from Designer form");
        }
        window.activateWindow();
        window.setFocus();
        table->setSteps(data.reaction);
        require(table->rowCount() == 2 && table->steps()[3].outputs == QStringList({"y2", "y0"}),
                "two-row UI retains sequence");
        table->item(0, 1)->setText("");
        invalid = data;
        invalid.reaction = table->steps();
        require(!invalid.validate().ok(), "internal gap is not silently removed");
        table->setSteps(data.reaction);
        table->setCurrentCell(0, 1);
        table->removeStep();
        require(table->steps().size() == 3 && table->steps()[1].input == "x3",
                "delete preserves remaining order");
        table->setSteps(data.reaction);
        table->setCurrentCell(0, 0);
        table->insertStep();
        auto *insertEditor = table->findChild<QLineEdit *>("reactionCellEditor");
        require(insertEditor != nullptr, "inserted column is editable");
        QTest::keyClick(insertEditor, Qt::Key_Escape);
        require(table->steps().size() == 5 && table->steps()[1].input.isEmpty() &&
                    table->steps()[2].input == "x1",
                "insertion retains step positions");
        table->markError("0:1", "Missing input");
        require(table->item(0, 1)->toolTip() == "Missing input",
                "validation tooltip stays visible");
        table->setSteps(data.reaction);
        for (auto *spin : window.findChildren<QSpinBox *>()) {
            const auto name = spin->objectName();
            if (name == "variantNumber")
                spin->setValue(7);
            else if (name.endsWith("InputsCount"))
                spin->setValue(3);
            else if (name.endsWith("OutputsCount"))
                spin->setValue(4);
            else
                spin->setValue(name.startsWith("mealy") ? 2 : 5);
        }
        // Exercise the real delegate, including committing an active cell on save.
        table->setCurrentCell(1, 3);
        table->editItem(table->item(1, 3));
        app.processEvents();
        auto *editor = [table]() -> QLineEdit * {
            for (auto *cellEditor : table->findChildren<QLineEdit *>("reactionCellEditor"))
                if (cellEditor->isVisible())
                    return cellEditor;
            return nullptr;
        }();
        require(editor != nullptr, "delegate opens editor");
        editor->selectAll();
        QTest::keyClicks(editor, "y2,y0,y3");
        QTest::mouseClick(window.findChild<QPushButton *>("saveVariantButton"), Qt::LeftButton);
        data.reaction[3].outputs = {"y2", "y0", "y3"};
        auto saved = data;
        saved.number = 7;
        require(equal(saved, service.loadVariant(7).payloadAs<ReactionVariant>()),
                "UI save writes counts and ordered reaction");
        app.processEvents();
        const auto *hint = window.findChild<QLabel *>("reactionHint");
        require(table->geometry().bottom() < hint->geometry().top(),
                "reaction hint does not overlap the table scrollbar");
        // Regression: wrapped hints must stay below the table at narrow widths
        // and with the larger fonts used by Windows display scaling.
        const QFont originalFont = window.font();
        QFont larger = originalFont;
        larger.setPointSizeF(qMax(10.0, originalFont.pointSizeF()) * 1.5);
        window.setFont(larger);
        window.resize(window.minimumSizeHint());
        app.processEvents();
        require(table->geometry().bottom() < hint->geometry().top() &&
                    hint->height() >= hint->heightForWidth(hint->width()),
                "large-font wrapped text stays fully outside the table");
        window.setFont(originalFont);
        app.processEvents();
        window.resize(window.minimumSize());
        app.processEvents();
        if (argc > 1) {
            require(window.grab().save(QString::fromLocal8Bit(argv[1])), "screenshot");
            QPalette dark = app.palette();
            dark.setColor(QPalette::Window, QColor(38, 38, 38));
            dark.setColor(QPalette::WindowText, Qt::white);
            dark.setColor(QPalette::Base, QColor(30, 30, 30));
            dark.setColor(QPalette::AlternateBase, QColor(48, 48, 48));
            dark.setColor(QPalette::Text, Qt::white);
            dark.setColor(QPalette::Button, QColor(50, 50, 50));
            dark.setColor(QPalette::ButtonText, Qt::white);
            dark.setColor(QPalette::HighlightedText, Qt::white);
            app.setPalette(dark);
            QTest::qWait(20);
            require(table->item(1, 3)->text() == QStringLiteral("y2,y0,y3"),
                    "theme change preserves edited cell");
            require(window.grab().save(QString::fromLocal8Bit(argv[1]) + ".dark.png"),
                    "dark screenshot");
        }
        QTest::mouseClick(window.findChild<QPushButton *>("newVariantButton"), Qt::LeftButton);
        require(table->columnCount() == 1 && table->steps().isEmpty(),
                "New variant resets the reaction to one empty column");
    }
    data = sample();
    {
        std::unique_ptr<Database> db(Database::open(dbPath));
        VariantRepository repo(db.get());
        require(equal(data, repo.loadVariant(4).payloadAs<ReactionVariant>()),
                "order survives reopening database");
        require(db->exec("UPDATE ReactionSteps SET position=99 WHERE position=1 AND "
                         "variant_id=(SELECT id FROM Variants WHERE number=4)"),
                "corrupt step order");
        require(!repo.loadVariant(4).ok(), "noncontiguous stored positions rejected");
    }
    {
        std::unique_ptr<Database> db(Database::open(dir.filePath("old.db")));
        db->exec("CREATE TABLE States(name TEXT)");
        SchemaManager schema(db.get());
        require(!schema.createSchema(), "full database refused without migration");
    }
    std::cout
        << "PASS Lite validation, ordered SQLite/archive round trips, rollback, GUI and icons\n";
}
