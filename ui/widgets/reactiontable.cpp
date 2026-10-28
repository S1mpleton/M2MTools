#include "ui/widgets/reactiontable.h"
#include "ui/delegates/reactiondelegate.h"
#include <QHeaderView>
#include <QSignalBlocker>

ReactionTable::ReactionTable(QWidget *parent) : QTableWidget(2, 0, parent) {
    setObjectName(QStringLiteral("reactionTable"));
    setItemDelegate(new ReactionDelegate(this));
    setSortingEnabled(false);
    horizontalHeader()->setSectionsMovable(false);
    horizontalHeader()->setDefaultSectionSize(90);
    horizontalHeader()->setMinimumSectionSize(65);
    verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    setAlternatingRowColors(true);
    setFixedHeight(145);
    setSteps({});
    connect(this, &QTableWidget::cellChanged, this, [this] { ensureTrailingSpace(); });
    retranslate();
}
void ReactionTable::retranslate() {
    setVerticalHeaderLabels({tr("Input"), tr("Output")});
}
void ReactionTable::prepareColumn(int column) {
    QSignalBlocker block(this);
    for (int row = 0; row < 2; ++row) {
        auto *cell = new QTableWidgetItem;
        cell->setTextAlignment(Qt::AlignCenter);
        setItem(row, column, cell);
    }
}
void ReactionTable::setSteps(const QVector<ReactionStep> &steps) {
    QSignalBlocker block(this);
    setColumnCount(0);
    setColumnCount(qMax(12, int(steps.size()) + 1));
    for (int c = 0; c < columnCount(); ++c)
        prepareColumn(c);
    for (int c = 0; c < steps.size(); ++c) {
        item(0, c)->setText(steps[c].input);
        item(1, c)->setText(steps[c].outputs.join(','));
    }
}
QVector<ReactionStep> ReactionTable::steps() const {
    // Ignore only unused space at the right, never gaps inside the sequence.
    int last = columnCount() - 1;
    while (last >= 0 && item(0, last)->text().trimmed().isEmpty() &&
           item(1, last)->text().trimmed().isEmpty())
        --last;
    QVector<ReactionStep> result;
    for (int c = 0; c <= last; ++c) {
        const QString output = item(1, c)->text().trimmed();
        QStringList outputs;
        if (!output.isEmpty() && output != "-" && output != QStringLiteral("—")) {
            outputs = output.split(',');
            for (auto &value : outputs)
                value = value.trimmed();
        }
        result.append({item(0, c)->text().trimmed(), outputs});
    }
    return result;
}
void ReactionTable::ensureTrailingSpace() {
    QSignalBlocker block(this);
    for (int c = 0; c < columnCount(); ++c)
        for (int r = 0; r < 2; ++r) {
            item(r, c)->setData(Qt::DecorationRole, QVariant());
            item(r, c)->setToolTip(QString());
        }
    const int c = columnCount() - 1;
    if (c < 0 || !item(0, c)->text().isEmpty() || !item(1, c)->text().isEmpty()) {
        insertColumn(columnCount());
        prepareColumn(columnCount() - 1);
    }
}
void ReactionTable::insertStep() {
    const int c = currentColumn() < 0 ? steps().size() : currentColumn() + 1;
    insertColumn(c);
    prepareColumn(c);
    setCurrentCell(0, c);
    editItem(item(0, c));
}
void ReactionTable::removeStep() {
    if (currentColumn() < 0)
        return;
    removeColumn(currentColumn());
    ensureTrailingSpace();
}
void ReactionTable::markError(const QString &offender, const QString &message) {
    const auto parts = offender.split(':');
    if (parts.size() != 2)
        return;
    const int r = parts[0].toInt(), c = parts[1].toInt();
    if (r < 0 || r >= 2 || c < 0 || c >= columnCount())
        return;
    QSignalBlocker block(this);
    item(r, c)->setToolTip(message);
    setCurrentCell(r, c);
    scrollToItem(item(r, c));
    setFocus();
}
