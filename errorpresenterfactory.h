#ifndef ERRORPRESENTERFACTORY_H
#define ERRORPRESENTERFACTORY_H

#include "IErrorPresenter.h"

#include <QStatusBar>
#include <QLineEdit>
#include <QTableWidget>

namespace ErrorPresenterFactory {

    IErrorPresenter* forLineEdit(
        QLineEdit* field,
        QStatusBar* statusBar,
        QObject* parent = nullptr
    );

    IErrorPresenter* forTableCell(
        QTableWidget* table,
        int row,
        int col,
        QStatusBar* statusBar,
        QObject* parent = nullptr
    );

}

#endif // ERRORPRESENTERFACTORY_H
