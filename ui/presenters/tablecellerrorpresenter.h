#ifndef TABLECELLERRORPRESENTER_H
#define TABLECELLERRORPRESENTER_H

#include "ierrorpresenter.h"

#include <QTableWidget>
#include <QPointer>
#include <QStatusBar>

class TableCellErrorPresenter : public IErrorPresenter
{
    Q_OBJECT
public:
    explicit TableCellErrorPresenter(
        QTableWidget* table,
        int row,
        int col,
        QStatusBar* statusBar = nullptr,
        QObject* parent = nullptr
    );

    void setCoordinates(int row, int col);

    void show(const Result& result) override;
    void clear(const Result& result) override;

private:
    QTableWidgetItem* item() const;

    QPointer<QTableWidget> m_table;
    QPointer<QStatusBar> m_statusBar;

    int m_row = -1;
    int m_col = -1;
};

#endif // TABLECELLERRORPRESENTER_H
