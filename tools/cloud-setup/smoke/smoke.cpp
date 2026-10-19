#include "core/automatondata.h"
#include "data/database.h"
#include "data/schemamanager.h"
#include "data/variantservice.h"
#include "ui/mainwindow.h"
#include <QApplication>
#include <QLineEdit>
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

        for (const auto type : {VariantType::MealyToMoore, VariantType::MooreToMealy}) {
            const int number = type == VariantType::MealyToMoore ? 101 : 102;
            AutomatonData original;
            requireOk(original.setVariantNumber(number));
            requireOk(original.setVariantType(type));
            requireOk(original.setStateNames({"s0"}));
            requireOk(original.setInputSignalNames({"x0"}));
            requireOk(original.setOutputSignalNames({"y0"}));
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
            requireOk(service.removeVariant(number));
            const auto exists = service.isExist(number);
            requireOk(exists);
            require(!exists.payloadAs<bool>(), "variant removal");
            std::cout << "PASS " << variantTypeToString(type).toStdString()
                      << " save/load/delete round trip\n";
        }

        MainWindow window(&service);
        window.show();
        QTimer::singleShot(100, &app, [&] {
            try {
                require(window.isVisible(), "main window visibility");
                auto* names = window.findChild<QLineEdit*>("stateNamesLineEdit");
                auto* inputs = window.findChild<QLineEdit*>("InputSignalNamesLineEdit");
                auto* table = window.findChild<QTableWidget*>("transitionTableWidget");
                require(names && inputs && table, "main window controls");
                names->setText("s0, s1");
                inputs->setText("x0");
                require(table->rowCount() == 2 && table->columnCount() == 3,
                        "UI signals rebuild the transition table");
                std::cout << "PASS offscreen window and interactive table update\n";
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
