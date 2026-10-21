#include "core/automatondata.h"
#include "core/reactionsynchronizer.h"
#include "data/database.h"
#include "data/schemamanager.h"
#include "data/variantservice.h"
#include "ui/mainwindow.h"
#include "ui/variantslist.h"
#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QRect>
#include <QScrollArea>
#include <QShortcut>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}

static void requireOk(const Result& result) {
    if (!result.ok())
        throw std::runtime_error((result.message() + ": " + result.details()).toStdString());
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir temporary("/workspace/.cloud-setup/smoke-data-XXXXXX");
        require(temporary.isValid(), "temporary database directory");
        std::unique_ptr<Database> db(Database::open(temporary.filePath("variants.db")));
        require(db && db->isOpen(), "SQLite plugin and database open");
        SchemaManager schema(db.get());
        QString error;
        require(schema.createSchema(&error), error.toUtf8().constData());
        require(schema.createSchema(&error), "schema must be repeatable");
        VariantRepository repository(db.get());
        VariantService service(db.get(), &repository);
        std::cout << "PASS SQLite schema creation and repeatability\n";

        {
            QTemporaryDir legacyDirectory("/workspace/.cloud-setup/legacy-data-XXXXXX");
            require(legacyDirectory.isValid(), "legacy database directory");
            std::unique_ptr<Database> legacyDb(
                Database::open(legacyDirectory.filePath("variants.db")));
            require(legacyDb && legacyDb->isOpen(), "legacy database open");
            require(legacyDb->exec(
                "CREATE TABLE Variants ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "number INTEGER NOT NULL UNIQUE, "
                "conversion TEXT NOT NULL);"),
                "legacy Variants table");
            SchemaManager legacySchema(legacyDb.get());
            require(legacySchema.createSchema(&error), error.toUtf8().constData());
            QSqlQuery columns = legacyDb->query("PRAGMA table_info(Variants);");
            bool hasImageData = false;
            bool hasImageName = false;
            while (columns.next()) {
                hasImageData |= columns.value(1).toString() == "image_data";
                hasImageName |= columns.value(1).toString() == "image_name";
            }
            require(hasImageData && hasImageName, "legacy schema image migration");
        }
        std::cout << "PASS backward-compatible image schema migration\n";

        AutomatonData reactionBase;
        requireOk(reactionBase.setVariantNumber(77));
        requireOk(reactionBase.setVariantType(VariantType::MealyToMoore));
        const QVector<ReactionStep> reaction = {
            {"a0", "x1", "a1", "y1"},
            {"a1", "x2", "a0", "y2"},
        };
        const ReactionSyncReport reactionReport =
            ReactionSynchronizer::buildAutomaton(reaction, reactionBase);
        require(!reactionReport.hasErrors(), "valid reaction must build an automaton");
        require(reactionReport.data.getStateNames() == QStringList({"a0", "a1"}) &&
                reactionReport.data.getInputSignalNames() == QStringList({"x1", "x2"}) &&
                reactionReport.data.getOutputSignalNames() == QStringList({"y1", "y2"}),
                "reaction infers A, X and Y names");
        require(reactionReport.data.getTransitionTable()[0][0].nextState == "a1" &&
                reactionReport.data.getTransitionTable()[0][0].outputSignals == QStringList({"y1"}),
                "reaction creates transition table cell");
        const auto rebuiltReaction =
            ReactionSynchronizer::buildReaction(reactionReport.data);
        require(rebuiltReaction.size() == 4, "transition table creates canonical reaction");
        const ReactionSyncReport conflictReport =
            ReactionSynchronizer::buildAutomaton({
                {"a0", "x1", "a1", "y1"},
                {"a0", "x1", "a2", "y2"},
            }, reactionBase);
        require(conflictReport.hasErrors(), "conflicting reaction is rejected");
        std::cout << "PASS reaction/table bidirectional model and conflict detection\n";

        AutomatonData closedAutomaton;
        requireOk(closedAutomaton.setStateNames({"a0", "a1"}));
        require(closedAutomaton.getInitialState() == "a0",
                "first state becomes the default initial state");
        requireOk(closedAutomaton.setInitialState("a1"));
        requireOk(closedAutomaton.setStateNames({"a0", "a1", "a2"}));
        require(closedAutomaton.getInitialState() == "a1",
                "explicit initial state survives list changes");
        requireOk(closedAutomaton.setStateNames({"a0", "a1"}));
        requireOk(closedAutomaton.setInputSignalNames({"x0", "x1"}));
        requireOk(closedAutomaton.setOutputSignalNames({"y0"}));
        requireOk(closedAutomaton.setTransitionCellByName(
            "x0", "a0", "a0 / y0"));
        requireOk(closedAutomaton.setTransitionCellByName(
            "x1", "a1", "a1 / y0"));
        requireOk(closedAutomaton.validate());
        const QVector<ReactionStep> initialFirst =
            ReactionSynchronizer::buildReaction(closedAutomaton);
        require(!initialFirst.isEmpty() && initialFirst.first().currentState == "a1",
                "reaction starts with the selected initial state");

        AutomatonData missingIncoming = closedAutomaton;
        requireOk(missingIncoming.setTransitionCellByName("x1", "a1", ""));
        require(missingIncoming.validate().code()
                    == ErrorCode::ADStateWithoutIncomingTransition,
                "state without an incoming edge is rejected");

        AutomatonData missingOutgoing;
        requireOk(missingOutgoing.setStateNames({"a0", "a1", "a2"}));
        requireOk(missingOutgoing.setInputSignalNames({"x0", "x1"}));
        requireOk(missingOutgoing.setOutputSignalNames({"y0"}));
        requireOk(missingOutgoing.setTransitionCellByName(
            "x0", "a0", "a1 / y0"));
        requireOk(missingOutgoing.setTransitionCellByName(
            "x1", "a0", "a2 / y0"));
        requireOk(missingOutgoing.setTransitionCellByName(
            "x0", "a1", "a1 / y0"));
        requireOk(missingOutgoing.setTransitionCellByName(
            "x1", "a1", "a0 / y0"));
        require(missingOutgoing.validate().code()
                    == ErrorCode::ADStateWithoutOutgoingTransition,
                "state without an outgoing edge is rejected");
        std::cout << "PASS closed-automaton validation and initial-state ordering\n";

        for (const auto type : {VariantType::MealyToMoore, VariantType::MooreToMealy}) {
            const int number = type == VariantType::MealyToMoore ? 101 : 102;
            AutomatonData original;
            requireOk(original.setVariantNumber(number));
            requireOk(original.setVariantType(type));
            requireOk(original.setStateNames({"s0"}));
            requireOk(original.setInputSignalNames({"x0"}));
            requireOk(original.setOutputSignalNames({"y0"}));
            original.setDiagramImage("image-payload", "task.png");
            requireOk(original.setTransitionCellByName("x0", "s0",
                type == VariantType::MealyToMoore ? "s0/y0" : "s0"));
            if (type == VariantType::MooreToMealy)
                requireOk(original.setMooreOutputCellByName("s0", "y0"));
            requireOk(original.validate());
            requireOk(service.saveVariant(original));
            const Result loadedResult = service.loadVariant(number);
            requireOk(loadedResult);
            require(loadedResult.hasPayload(), "loaded variant payload");
            const auto loaded = loadedResult.payloadAs<AutomatonData>();
            require(loaded.getType() == type && loaded.getVariantNumber() == number,
                    "loaded variant identity");
            require(loaded.getStateNames() == original.getStateNames() &&
                    loaded.getInputSignalNames() == original.getInputSignalNames() &&
                    loaded.getOutputSignalNames() == original.getOutputSignalNames(),
                    "loaded variant names");
            require(loaded.getTransitionTable().size() == 1 &&
                    loaded.getTransitionTable()[0].size() == 1 &&
                    loaded.getTransitionTable()[0][0].nextState == "s0",
                    "loaded transition");
            require(loaded.getTransitionTable()[0][0].outputSignals ==
                    original.getTransitionTable()[0][0].outputSignals &&
                    loaded.getMooreOutputs() == original.getMooreOutputs(),
                    "loaded output signals");
            require(loaded.getDiagramImage() == original.getDiagramImage() &&
                    loaded.getDiagramFileName() == "task.png",
                    "variant image round trip");
            AutomatonData replacement = original;
            replacement.setDiagramImage("updated-image", "updated.png");
            requireOk(service.replaceVariant(replacement));
            require(service.loadVariant(number).payloadAs<AutomatonData>()
                        .getDiagramFileName() == "updated.png",
                    "atomic variant replacement");
            requireOk(service.removeVariant(number));
            const auto exists = service.isExist(number);
            requireOk(exists);
            require(!exists.payloadAs<bool>(), "variant removal");
            std::cout << "PASS " << variantTypeToString(type).toStdString()
                      << " save/load/delete round trip\n";
        }

        auto makeVariant = [](int number, VariantType type, const char* imageName) {
            AutomatonData data;
            requireOk(data.setVariantNumber(number));
            requireOk(data.setVariantType(type));
            requireOk(data.setStateNames({"s0"}));
            requireOk(data.setInputSignalNames({"x0"}));
            requireOk(data.setOutputSignalNames({"y0"}));
            requireOk(data.setTransitionCellByName(
                "x0", "s0", type == VariantType::MealyToMoore ? "s0/y0" : "s0"));
            if (type == VariantType::MooreToMealy)
                requireOk(data.setMooreOutputCellByName("s0", "y0"));
            data.setDiagramImage("data", imageName);
            return data;
        };
        requireOk(service.saveVariant(makeVariant(
            201, VariantType::MealyToMoore, "first.png")));
        requireOk(service.saveVariant(makeVariant(
            202, VariantType::MooreToMealy, "second.png")));
        const QString archivePath = temporary.filePath("BazaVar.m2m");
        requireOk(service.exportArchive(archivePath, "correct-password"));
        QFile archiveFile(archivePath);
        require(archiveFile.open(QIODevice::ReadOnly), "encrypted archive readable");
        const QByteArray archiveBytes = archiveFile.readAll();
        require(archiveBytes.startsWith("M2MTOOLS1") &&
                !archiveBytes.contains("first.png"),
                "archive has authenticated format and no plaintext metadata");
        require(!service.importArchive(archivePath, "wrong-password").ok() &&
                service.isExist(201).payloadAs<bool>(),
                "wrong archive password does not alter database");
        requireOk(service.swapVariants(201, 202));
        auto swappedFirst = service.loadVariant(201).payloadAs<AutomatonData>();
        auto swappedSecond = service.loadVariant(202).payloadAs<AutomatonData>();
        require(swappedFirst.getType() == VariantType::MooreToMealy &&
                swappedFirst.getDiagramFileName() == "second.png" &&
                swappedSecond.getType() == VariantType::MealyToMoore &&
                swappedSecond.getDiagramFileName() == "first.png",
                "swap keeps complete variant records");
        requireOk(service.swapVariants(202, 299));
        require(!service.isExist(202).payloadAs<bool>() &&
                service.isExist(299).payloadAs<bool>(),
                "swap with empty number moves the variant");
        requireOk(service.compactVariantNumbers());
        require(service.isExist(1).payloadAs<bool>() &&
                service.isExist(2).payloadAs<bool>(),
                "variant gaps are compacted");
        requireOk(service.removeVariant(1));
        requireOk(service.removeVariant(2));
        requireOk(service.importArchive(archivePath, "correct-password"));
        require(service.isExist(201).payloadAs<bool>() &&
                service.isExist(202).payloadAs<bool>() &&
                service.loadVariant(201).payloadAs<AutomatonData>()
                    .getDiagramFileName() == "first.png",
                "encrypted archive restores complete database");
        requireOk(service.removeVariant(201));
        requireOk(service.removeVariant(202));
        std::cout << "PASS variant number swap, empty slot and compaction\n";
        std::cout << "PASS encrypted BazaVar export/import and wrong-password safety\n";

        {
            VariantsList variantsDialog(&service);
            require(variantsDialog.findChild<QComboBox*>("swapFirstComboBox") &&
                    variantsDialog.findChild<QComboBox*>("swapSecondComboBox") &&
                    variantsDialog.findChild<QPushButton*>("swapVariantsButton"),
                    "variant list exposes swap controls");
        }
        std::cout << "PASS variant list swap controls\n";

        MainWindow window(&service);
        window.show();
        QTimer::singleShot(100, &app, [&] {
            try {
                require(window.isVisible(), "main window visibility");
                window.resize(window.minimumSize());
                QApplication::processEvents();
                auto* names = window.findChild<QLineEdit*>("stateNamesLineEdit");
                auto* inputs = window.findChild<QLineEdit*>("InputSignalNamesLineEdit");
                auto* table = window.findChild<QTableWidget*>("transitionTableWidget");
                auto* outputs = window.findChild<QLineEdit*>("OutputSignalNamesLineEdit");
                auto* editMode = window.findChild<QComboBox*>("editModeComboBox");
                auto* initialState =
                    window.findChild<QComboBox*>("initialStateComboBox");
                auto* reactionTable = window.findChild<QTableWidget*>("reactionTableWidget");
                auto* diagram = window.findChild<QLabel*>("diagramDropZone");
                auto* exportArchive =
                    window.findChild<QPushButton*>("exportArchiveButton");
                auto* importArchive =
                    window.findChild<QPushButton*>("importArchiveButton");
                require(names && inputs && outputs && table && editMode && initialState &&
                        reactionTable && diagram && exportArchive && importArchive,
                        "modern main window controls");
                require(editMode->currentIndex() == 1,
                        "reaction editor is selected on startup");
                auto* russianAction = window.findChild<QAction*>("actionSetLangRu");
                auto* englishAction = window.findChild<QAction*>("actionSetLangEn");
                auto* addTactButton =
                    window.findChild<QPushButton*>("addReactionStepButton");
                require(russianAction && englishAction && addTactButton,
                        "language actions and dynamic reaction controls");
                const QString englishTitle = window.windowTitle();
                const QString englishAddTact = addTactButton->text();
                russianAction->trigger();
                QApplication::processEvents();
                require(window.windowTitle() != englishTitle &&
                        addTactButton->text() != englishAddTact,
                        "Russian catalog translates dynamic controls");
                englishAction->trigger();
                QApplication::processEvents();
                require(window.windowTitle() == englishTitle &&
                        addTactButton->text() == englishAddTact,
                        "English remains the untranslated base language");
                require(!window.findChild<QAction*>("actionTypeSoft") &&
                        !window.findChild<QAction*>("actionTypeStrict"),
                        "legacy soft and strict checks are removed");
                require(!window.findChild<QScrollArea*>(),
                        "main workflow does not require a scroll area");
                require(window.styleSheet().isEmpty() &&
                        diagram->styleSheet().isEmpty(),
                        "normal controls use the platform palette");
                require(window.minimumHeight() <= 620,
                        "main window supports a compact desktop size");
                require(reactionTable->item(0, 0),
                        "empty reaction exposes its first state cell");
                reactionTable->item(0, 0)->setText("q0");
                require(initialState->currentText() == "q0",
                        "first state entered in a reaction becomes initial");
                names->setText("s0, s1");
                inputs->setText("x0");
                outputs->setText("y0");
                require(table->rowCount() == 2 && table->columnCount() == 3,
                        "UI signals rebuild the transition table");
                editMode->setCurrentIndex(1);
                require(reactionTable->columnCount() == 2,
                        "reaction has one column per state/input pair");
                require(initialState->currentText() == "s0" &&
                        reactionTable->item(0, 0)->text() == "s0",
                        "first entered state initializes the reaction");
                require(reactionTable->item(2, 0) && reactionTable->item(3, 0),
                        "reaction cells exist");
                reactionTable->item(2, 0)->setText("s1");
                require(outputs->text() == "y0",
                        "manual signal list is preserved while filling reaction");
                reactionTable->item(3, 0)->setText("y0");
                require(table->item(1, 1)->text() == "s1 / y0",
                        "reaction edit updates transition table");
                table->item(1, 1)->setText("s0 / y0");
                require(reactionTable->item(2, 0)->text() == "s0",
                        "transition edit updates reaction");
                initialState->setCurrentText("s1");
                require(reactionTable->item(0, 0)->text() == "s1",
                        "changing initial state reorders the reaction");
                reactionTable->item(0, 0)->setText("s0");
                require(reactionTable->item(0, 0)->text() == "s1",
                        "reaction cannot start outside the selected initial state");
                auto* enterShortcut = window.findChild<QShortcut*>(
                    "insertReactionStepReturnShortcut");
                require(enterShortcut, "reaction Enter shortcut exists");
                reactionTable->setCurrentCell(1, 0);
                const int tactCount = reactionTable->columnCount();
                const QString precedingNextState = reactionTable->item(2, 0)->text();
                require(QMetaObject::invokeMethod(enterShortcut, "activated"),
                        "reaction Enter shortcut activates");
                QApplication::processEvents();
                require(reactionTable->columnCount() == tactCount + 1 &&
                        reactionTable->currentColumn() == 1,
                        "Enter inserts a tact after the selected column");
                require(reactionTable->item(0, 1) &&
                        reactionTable->item(0, 1)->text() == precedingNextState,
                        "inserted tact continues from the preceding next state");
                for (QPushButton* button : {exportArchive, importArchive}) {
                    const QRect buttonRect(
                        button->mapTo(window.centralWidget(), QPoint(0, 0)),
                        button->size());
                    require(window.centralWidget()->rect().contains(buttonRect),
                            "all primary actions fit inside the window");
                }
                const QString screenshotPath =
                    qEnvironmentVariable("M2MTOOLS_SMOKE_SCREENSHOT");
                if (!screenshotPath.isEmpty())
                    require(window.grab().save(screenshotPath),
                            "UI screenshot can be captured");
                std::cout << "PASS modern offscreen UI and live bidirectional editing\n";
                window.close();
                app.exit(0);
            } catch (const std::exception& failure) {
                std::cerr << "FAIL " << failure.what() << '\n';
                app.exit(1);
            }
        });
        return app.exec();
    } catch (const std::exception& failure) {
        std::cerr << "FAIL " << failure.what() << '\n';
        return 1;
    }
}
