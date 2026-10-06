#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/automatondata.h"
#include "data/variantservice.h"
#include "ui/presenters/tablecellerrorpresenter.h"

#include <QLineEdit>
#include <QMainWindow>

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

    void setupPresenters();
    void setupConnections();
    void refreshTable();

};
#endif // MAINWINDOW_H
