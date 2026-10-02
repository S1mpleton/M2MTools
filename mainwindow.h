#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "automatondata.h"
#include "tablecellerrorpresenter.h"

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onVariantTypeChanged(int index);
    void onVariantNumberChanged(int number);

    void onStateNamesChanged(const QString& text);
    void onInputNamesChanged(const QString& text);
    void onOutputNamesChanged(const QString& text);
    void onTableCellChanged(int row, int col);

private:
    Ui::MainWindow *ui;
    AutomatonData m_data;

    bool m_updatingTable = false;
    bool m_updatingUi = false;

    IErrorPresenter* m_stateNamesPresenter = nullptr;
    IErrorPresenter* m_inputNamesPresenter = nullptr;
    IErrorPresenter* m_outputNamesPresenter = nullptr;

    TableCellErrorPresenter* m_cellPresenter = nullptr;

    void setupPresenters();
    void setupConnections();
    void refreshTable();

};
#endif // MAINWINDOW_H
