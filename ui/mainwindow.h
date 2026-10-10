#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/automatondata.h"
#include "data/variantservice.h"
#include "infrastructure/translationmanager.h"
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
    // header tool
    void onSetLangRuTriggered(bool isActive);
    void onSetLangEnTriggered(bool isActive);

    void onVariantTypeChanged(int index);
    void onVariantNumberChanged(int number);

    void onStateNamesChanged(const QString& text);
    // void onStateNamesEditingFinished();

    void onInputNamesChanged(const QString& text);
    // void onInputNamesEditingFinished();

    void onOutputNamesChanged(const QString& text);
    // void onOutputNamesEditingFinished();

    void onTableCellChanged(int row, int col);

    void onLoadVariantPushButton();
    void onSavePushButtonClicked();

private:
    Ui::MainWindow *ui;
    AutomatonData m_data;

    bool m_updatingTable = false;
    bool m_updatingUi = false;

    VariantService* m_variantService = nullptr;
    IErrorPresenter* m_stateNamesPresenter = nullptr;
    IErrorPresenter* m_inputNamesPresenter = nullptr;
    IErrorPresenter* m_outputNamesPresenter = nullptr;

    TableCellErrorPresenter* m_cellPresenter = nullptr;

    TranslationManager* m_translator = nullptr;

    void setupPresenters();
    void setupConnections();
    void refreshTable();

protected:
    void changeEvent(QEvent *event) override;

};
#endif // MAINWINDOW_H
