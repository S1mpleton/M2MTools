#include "data/database.h"
#include "data/schemamanager.h"
#include "ui/mainwindow.h"


#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // QTranslator translator;
    // const QStringList uiLanguages = QLocale::system().uiLanguages();
    // for (const QString &locale : uiLanguages) {
    //     const QString baseName = "M2MTools_" + QLocale(locale).name();
    //     if (translator.load(":/i18n/" + baseName)) {
    //         a.installTranslator(&translator);
    //         break;
    //     }
    // }

    Database* db = Database::open("variants.db", &a);
    if (!db) {
        QMessageBox::critical(nullptr, "Error database", "Failed to open the database.");
        return 1;
    }

    SchemaManager schemaManager(db);
    QString schemaError;
    if (!schemaManager.createSchema(&schemaError)) {
        QMessageBox::critical(nullptr, "Ошибка схемы", schemaError);
        return 1;
    }

    VariantRepository* repo = new VariantRepository(db, &a);
    VariantService* service = new VariantService(db, repo, &a);

    MainWindow w(service);
    w.show();
    return QApplication::exec();
}
