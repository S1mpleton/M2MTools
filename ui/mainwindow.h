#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/automatondata.h"
#include "data/variantservice.h"
#include "infrastructure/translationmanager.h"

#include "ui/logwindow.h"
#include "ui/presenters/tablecellerrorpresenter.h"

#include <QLineEdit>
#include <QMainWindow>
#include <QTranslator>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(VariantService* service, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // header slots
    void onLanguageSelected(QAction* action);
    void onTypeCheckSelected(QAction* action);

    void onShowLogClicked();
    void onExitAction();

    // basic slots
    void onVariantTypeChanged(int index);
    void onVariantNumberChanged(int number);

    void onStateNamesChanged(const QString& text);

    void onInputNamesChanged(const QString& text);

    void onOutputNamesChanged(const QString& text);

    void onTableCellChanged(int row, int col);

    void onLoadVariantPushButton();
    void onSavePushButtonClicked();



private:
    Ui::MainWindow *ui;
    AutomatonData m_data;

    bool m_updatingTable = false;
    bool m_updatingUi = false;

    LogWindow* m_logWindow = nullptr;

    VariantService* m_variantService = nullptr;
    IErrorPresenter* m_stateNamesPresenter = nullptr;
    IErrorPresenter* m_inputNamesPresenter = nullptr;
    IErrorPresenter* m_outputNamesPresenter = nullptr;

    TableCellErrorPresenter* m_cellPresenter = nullptr;

    TranslationManager* m_translator = nullptr;

    void setupPresenters();
    void setupConnections();
    void setupLanguageGroup();
    void setupTypeСheckGroup();

    void refreshTable();
    void refreshData();

protected:
    void changeEvent(QEvent *event) override;

};
#endif // MAINWINDOW_H
