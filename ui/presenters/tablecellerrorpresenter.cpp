#include "tablecellerrorpresenter.h"

#include "ui/translateresult.h"

namespace {
    const QColor kErrorBackground(204, 34, 34);
    const QColor kNormalBackground("");
}

TableCellErrorPresenter::TableCellErrorPresenter(
    QTableWidget* table,
    int row,
    int col,
    QStatusBar* statusBar,
    QObject* parent
)
    : IErrorPresenter(parent), m_table(table), m_row(row), m_col(col), m_statusBar(statusBar)
{
}

QTableWidgetItem* TableCellErrorPresenter::item() const {
    if (!m_table) return nullptr;

    return m_table->item(m_row, m_col);
}

void TableCellErrorPresenter::setCoordinates(int row, int col) {
    m_row = row;
    m_col = col;
}

void TableCellErrorPresenter::show(const Result& result) {
    if (auto* it = item()) {
        it->setBackground(QBrush(kErrorBackground));
        it->setToolTip(translateResult(result));
    }
    if (m_statusBar)
        m_statusBar->showMessage(translateResult(result));
}

void TableCellErrorPresenter::clear(const Result& result) {
    if (auto* it = item()) {
        it->setBackground(QBrush());
        it->setToolTip("");
    }
    if (m_statusBar)
        m_statusBar->showMessage(translateResult(result), 2500);
}