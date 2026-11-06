#include "data/database.h"
#include "data/schemamanager.h"
#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/mainwindow.h"
#include "ui/appicons.h"


#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("M2MTools");
    a.setWindowIcon(AppIcons::applicationIcon());
    a.setAttribute(Qt::AA_DontShowIconsInMenus, false);
    TranslationManager translationManager(&a);
    translationManager.loadSystemLocale();

    Database* db = Database::open("variants-lite.db", &a);
    if (!db) {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "Database error"),
            QCoreApplication::translate("main", "Failed to open the database."));
        return 1;
    }

    SchemaManager schemaManager(db);
    QString schemaError;
    if (!schemaManager.createSchema(&schemaError)) {
        Logger::error(QStringLiteral("Failed to initialize database schema: %1")
                          .arg(schemaError),
                      ResultCategory::Database);
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "Schema error"),
            QCoreApplication::translate(
                "main", "Failed to initialize the database schema.") + "\n" + schemaError);
        return 1;
    }

    VariantRepository* repo = new VariantRepository(db, &a);
    VariantService* service = new VariantService(db, repo, &a);

    MainWindow w(service, &translationManager, nullptr);
    w.show();
    return QApplication::exec();
}
