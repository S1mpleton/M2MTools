#include "errorpresenterfactory.h"
#include "fielderrorhighlighter.h"
#include "tablecellerrorpresenter.h"


namespace ErrorPresenterFactory {

    IErrorPresenter* forLineEdit(
        QLineEdit* field,
        QStatusBar* statusBar,
        QObject* parent
    )
    {
        return new FieldErrorHighlighter(field, statusBar, parent);
    }

    IErrorPresenter* forTableCell(
        QTableWidget* table,
        int row,
        int col,
        QStatusBar* statusBar,
        QObject* parent
    )
    {
        return new TableCellErrorPresenter(table, row, col, statusBar, parent);
    }

}