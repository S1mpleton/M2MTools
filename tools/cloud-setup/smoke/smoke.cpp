#include "core/automatondata.h"
#include "core/reactionsynchronizer.h"
#include "data/database.h"
#include "data/schemamanager.h"
#include "data/variantservice.h"
#include "ui/mainwindow.h"
#include "ui/feedbackstyle.h"
#include "ui/variantslist.h"
#include "validation/compositevalidator.h"
#include "validation/editvalidator.h"
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
#include <QSpinBox>
#include <QSqlQuery>
#include <QStatusBar>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariantAnimation>
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
        require(rebuiltReaction.size() == 2,
                "reaction contains only explicitly defined transitions");
        const ReactionSyncReport conflictReport =
            ReactionSynchronizer::buildAutomaton({
                {"a0", "x1", "a1", "y1"},
                {"a0", "x1", "a2", "y2"},
            }, reactionBase);
        require(conflictReport.hasErrors(), "conflicting reaction is rejected");
        AutomatonData sparseDraft;
        requireOk(sparseDraft.setStateNames({"a0", "a1"}));
        requireOk(sparseDraft.setInputSignalNames({"x1", "x2"}));
        requireOk(sparseDraft.setOutputSignalNames({"y1"}));
        requireOk(sparseDraft.setTransitionCellByName(
            "x2", "a0", "a0 / y1"));
        const QVector<ReactionStep> sparseReaction =
            ReactionSynchronizer::buildReaction(sparseDraft);
        require(sparseReaction.size() == 1 &&
                sparseReaction.first().currentState == "a0" &&
                sparseReaction.first().inputSignal == "x2",
                "states do not create tacts for every available input");
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
        auto makeSparseVariant = [] {
            AutomatonData data;
            requireOk(data.setVariantNumber(203));
            requireOk(data.setVariantType(VariantType::MealyToMoore));
            requireOk(data.setStateNames({"s0", "s1"}));
            requireOk(data.setInputSignalNames({"x0", "x1"}));
            requireOk(data.setOutputSignalNames({"y0"}));
            requireOk(data.setTransitionCellByName("x0", "s0", "s1/y0"));
            requireOk(data.setTransitionCellByName("x1", "s1", "s0/y0"));
            data.setDiagramImage("sparse-data", "sparse.png");
            requireOk(data.validate());
            return data;
        };
        requireOk(service.saveVariant(makeVariant(
            201, VariantType::MealyToMoore, "first.png")));
        requireOk(service.saveVariant(makeVariant(
            202, VariantType::MooreToMealy, "second.png")));
        requireOk(service.saveVariant(makeSparseVariant()));
        const QString archivePath = temporary.filePath("BazaVar.m2m");
        requireOk(service.exportArchive(archivePath));
        QFile archiveFile(archivePath);
        require(archiveFile.open(QIODevice::ReadOnly), "protected archive readable");
        const QByteArray archiveBytes = archiveFile.readAll();
        require(archiveBytes.startsWith("M2MTOOLS2") &&
                !archiveBytes.contains("first.png"),
                "archive has authenticated format and no plaintext metadata");
        QByteArray modifiedBytes = archiveBytes;
        modifiedBytes[modifiedBytes.size() - 1] =
            modifiedBytes.at(modifiedBytes.size() - 1) ^ char(1);
        const QString modifiedArchivePath =
            temporary.filePath("ModifiedBazaVar.m2m");
        QFile modifiedArchive(modifiedArchivePath);
        require(modifiedArchive.open(QIODevice::WriteOnly) &&
                modifiedArchive.write(modifiedBytes) == modifiedBytes.size(),
                "modified archive fixture can be written");
        modifiedArchive.close();
        require(!service.importArchive(modifiedArchivePath).ok() &&
                service.isExist(201).payloadAs<bool>(),
                "modified archive does not alter database");
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
        requireOk(service.importArchive(archivePath));
        require(service.isExist(201).payloadAs<bool>() &&
                service.isExist(202).payloadAs<bool>() &&
                service.loadVariant(201).payloadAs<AutomatonData>()
                    .getDiagramFileName() == "first.png",
                "protected archive restores complete database");
        const AutomatonData sparseArchiveVariant =
            service.loadVariant(203).payloadAs<AutomatonData>();
        require(sparseArchiveVariant.getTransitionTable()[0][1].isEmpty() &&
                sparseArchiveVariant.getTransitionTable()[1][0].isEmpty(),
                "BazaVar round trip preserves valid sparse transitions");
        requireOk(service.removeVariant(201));
        requireOk(service.removeVariant(202));
        requireOk(service.removeVariant(203));
        std::cout << "PASS variant number swap, empty slot and compaction\n";
        std::cout << "PASS protected BazaVar export/import and modification safety\n";

        requireOk(service.saveVariant(makeVariant(
            1, VariantType::MealyToMoore, "one.png")));
        requireOk(service.saveVariant(makeVariant(
            3, VariantType::MooreToMealy, "three.png")));
        require(service.firstAvailableVariantNumber() == 2,
                "smallest free variant number fills the first gap");

        {
            VariantsList variantsDialog(&service);
            auto* sort = variantsDialog.findChild<QComboBox*>(
                "variantSortComboBox");
            auto* variantsTable = variantsDialog.findChild<QTableWidget*>(
                "variantsTableWidget");
            require(sort && variantsTable &&
                    variantsDialog.findChild<QComboBox*>("swapFirstComboBox") &&
                    variantsDialog.findChild<QComboBox*>("swapSecondComboBox") &&
                    variantsDialog.findChild<QPushButton*>("swapVariantsButton"),
                    "variant list exposes sorting and swap controls");
            require(variantsTable->item(0, 0)->text() == "1" &&
                    variantsTable->item(1, 0)->text() == "3",
                    "variants are sorted by number ascending by default");
            sort->setCurrentIndex(1);
            require(variantsTable->item(0, 0)->text() == "3" &&
                    variantsTable->item(1, 0)->text() == "1",
                    "variants can be sorted by number descending");
            sort->setCurrentIndex(2);
            require(variantsTable->item(0, 0)->text() == "1",
                    "variants can be sorted by task type");
            sort->setCurrentIndex(3);
            require(variantsTable->item(0, 0)->text() == "3",
                    "task type sorting can be reversed");
        }
        std::cout << "PASS variant list sorting and swap controls\n";

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
                auto* validationMessage =
                    window.findChild<QLabel*>("validationMessage");
                auto* newVariant =
                    window.findChild<QPushButton*>("newVariantButton");
                auto* variantNumber =
                    window.findChild<QSpinBox*>("numberVariantSpinBox");
                auto* conversion =
                    window.findChild<QComboBox*>("variantTypeComboBox");
                require(names && inputs && outputs && table && editMode && initialState &&
                        reactionTable && diagram && exportArchive && importArchive &&
                        validationMessage && newVariant && variantNumber && conversion,
                        "modern main window controls");
                require(importArchive->text() == "Import database" &&
                        exportArchive->text() == "Export database",
                        "database actions use concise English labels");
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
                        addTactButton->text() != englishAddTact &&
                        importArchive->text() == QString::fromUtf8("Импорт БД") &&
                        exportArchive->text() == QString::fromUtf8("Экспорт БД"),
                        "Russian catalog translates dynamic controls");
                require(QCoreApplication::translate(
                            "ResultMessenger", "Error") == QString::fromUtf8("Ошибка") &&
                        QCoreApplication::translate(
                            "main", "Database error") ==
                            QString::fromUtf8("Ошибка базы данных"),
                        "Russian catalog translates popup windows");
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
                QWidget* singleNameEditor = reactionTable->itemDelegate()->createEditor(
                    reactionTable, QStyleOptionViewItem(),
                    reactionTable->model()->index(0, 0));
                auto* singleNameLineEdit = qobject_cast<QLineEdit*>(singleNameEditor);
                require(singleNameLineEdit &&
                        qobject_cast<const EditValidator*>(
                            singleNameLineEdit->validator()),
                        "reaction state cells use the shared single-name validator");
                QString invalidSingleName = QStringLiteral("q@ 1");
                int singleNamePosition = invalidSingleName.size();
                singleNameLineEdit->validator()->validate(
                    invalidSingleName, singleNamePosition);
                require(invalidSingleName == QStringLiteral("q1"),
                        "reaction single-name validator removes separators and symbols");
                delete singleNameEditor;

                QWidget* outputEditor = reactionTable->itemDelegate()->createEditor(
                    reactionTable, QStyleOptionViewItem(),
                    reactionTable->model()->index(3, 0));
                auto* outputLineEdit = qobject_cast<QLineEdit*>(outputEditor);
                require(outputLineEdit &&
                        qobject_cast<const CompositeValidator*>(
                            outputLineEdit->validator()),
                        "reaction output cells use the shared name-list validator");
                QString outputList = QStringLiteral("y1 ");
                int outputPosition = outputList.size();
                outputLineEdit->validator()->validate(outputList, outputPosition);
                require(outputList == QStringLiteral("y1, "),
                        "reaction output validator normalizes list separators");
                delete outputEditor;
                require(QMetaObject::invokeMethod(
                            &window, "onSavePushButtonClicked"),
                        "save validation can be invoked");
                require(!QApplication::activeModalWidget() &&
                        validationMessage->styleSheet() ==
                            UiFeedbackStyle::bannerStyle(UiFeedbackLevel::Error) &&
                        validationMessage->text().contains("no states") &&
                        validationMessage->isVisible() &&
                        validationMessage->height() >= 40,
                        "save validation explains the automaton error inline without a popup");
                variantNumber->setValue(99);
                require(QMetaObject::invokeMethod(variantNumber, "editingFinished"),
                        "manual variant number edit can be simulated");
                conversion->setCurrentIndex(1);
                names->setText("temporaryState");
                inputs->setText("temporaryInput");
                outputs->setText("temporaryOutput");
                newVariant->click();
                require(variantNumber->value() == 2 &&
                        conversion->currentIndex() == 0 &&
                        names->text().isEmpty() && inputs->text().isEmpty() &&
                        outputs->text().isEmpty() && initialState->count() == 0 &&
                        reactionTable->item(0, 0)->text().isEmpty(),
                        "new variant clears all fields and uses the smallest free number");
                require(validationMessage->styleSheet() ==
                            UiFeedbackStyle::bannerStyle(UiFeedbackLevel::Success),
                        "successful user actions use the green inline message");
                reactionTable->item(0, 0)->setText("1bad");
                require(reactionTable->item(0, 0)->background().color() ==
                            UiFeedbackStyle::background(UiFeedbackLevel::Error) &&
                        validationMessage->text().contains("Invalid name"),
                        "invalid reaction names are highlighted immediately");
                reactionTable->item(0, 0)->setText("q0");
                require(initialState->currentText() == "q0",
                        "first state entered in a reaction becomes initial");
                require(names->findChild<QVariantAnimation*>(
                            "automaticFieldFeedback", Qt::FindDirectChildrenOnly) &&
                        initialState->findChild<QVariantAnimation*>(
                            "automaticFieldFeedback", Qt::FindDirectChildrenOnly),
                        "automatic state and initial-state changes are animated");
                require(names->findChild<QVariantAnimation*>(
                            "automaticFieldFeedback", Qt::FindDirectChildrenOnly)
                            ->duration() <= 1500,
                        "automatic field feedback is one short pulse");
                reactionTable->item(1, 0)->setText("xAuto");
                require(inputs->findChild<QVariantAnimation*>(
                            "automaticFieldFeedback", Qt::FindDirectChildrenOnly),
                        "automatic input addition is animated");
                reactionTable->item(2, 0)->setText("q0");
                reactionTable->item(3, 0)->setText("yAuto");
                require(outputs->findChild<QVariantAnimation*>(
                            "automaticFieldFeedback", Qt::FindDirectChildrenOnly),
                        "automatic output addition is animated");
                names->setText("1bad");
                require(names->styleSheet() ==
                            UiFeedbackStyle::fieldStyle(UiFeedbackLevel::Error),
                        "invalid symbol lists use the shared error color");
                require(names->styleSheet().contains("color: #8a1c13") &&
                        QMetaObject::invokeMethod(
                            &window, "onSavePushButtonClicked") &&
                        validationMessage->text().contains("Invalid name") &&
                        window.statusBar()->currentMessage().isEmpty(),
                        "save errors are readable in the inline message, not the status bar");
                names->setText("s0, s1");
                inputs->setText("x0");
                outputs->setText("y0");
                require(table->rowCount() == 2 && table->columnCount() == 3,
                        "UI signals rebuild the transition table");
                editMode->setCurrentIndex(1);
                require(reactionTable->columnCount() == 1,
                        "unused state/input pairs do not create reaction tacts");
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
                require(QMetaObject::invokeMethod(
                            &window, "onSavePushButtonClicked"),
                        "closed-state save validation can be invoked");
                QApplication::processEvents();
                require(
                        validationMessage->isVisible() &&
                        validationMessage->height() >= 40 &&
                        validationMessage->text().contains("no incoming transition"),
                        "closed-state save error is visible in the feedback panel");
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
                names->setText("s0");
                inputs->setText("x0, x1");
                outputs->setText("y0");
                table->item(1, 1)->setText("s0 / y0");
                require(QMetaObject::invokeMethod(
                            &window, "onSavePushButtonClicked"),
                        "unused-input save validation can be invoked");
                QApplication::processEvents();
                require(
                        validationMessage->isVisible() &&
                        validationMessage->height() >= 40 &&
                        validationMessage->text().contains(
                            "Input signal 'x1' is not used"),
                        "unused-input save error is visible in the feedback panel");
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
                requireOk(service.removeVariant(1));
                requireOk(service.removeVariant(3));
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
