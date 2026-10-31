#include "ui/widgets/reactiontable.h"
#include "ui/delegates/reactiondelegate.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyle>

ReactionTable::ReactionTable(QWidget *parent) : QTableWidget(2, 0, parent) {
    setObjectName(QStringLiteral("reactionTable"));
    auto *delegate = new ReactionDelegate(this);
    setItemDelegate(delegate);
    connect(delegate, &ReactionDelegate::nextStepRequested, this, &ReactionTable::insertStep);
    setSortingEnabled(false);
    horizontalHeader()->setSectionsMovable(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(false);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrap(false);
    setAlternatingRowColors(true);
    connect(horizontalHeader(), &QHeaderView::geometriesChanged, this, &ReactionTable::updateTableSize);
    setSteps({});
    connect(this, &QTableWidget::cellChanged, this, [this] { clearCellFeedback(); });
    retranslate();
}
ReactionTable::~ReactionTable() {
    // QTableWidget tears down its headers after the derived object is gone.
    disconnect(horizontalHeader(), &QHeaderView::geometriesChanged, this, &ReactionTable::updateTableSize);
}
void ReactionTable::retranslate() {
    setVerticalHeaderLabels({tr("Input"), tr("Output")});
    updateTableSize();
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
    setColumnCount(qMax(1, int(steps.size())));
    for (int c = 0; c < columnCount(); ++c)
        prepareColumn(c);
    for (int c = 0; c < steps.size(); ++c) {
        item(0, c)->setText(steps[c].input);
        item(1, c)->setText(steps[c].outputs.join(','));
    }
    setCurrentCell(0, 0);
    updateTableSize();
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
void ReactionTable::clearCellFeedback() {
    QSignalBlocker block(this);
    for (int c = 0; c < columnCount(); ++c)
        for (int r = 0; r < 2; ++r) {
            if (!item(r, c))
                continue;
            item(r, c)->setData(ReactionDelegate::ErrorRole, QVariant());
            item(r, c)->setData(Qt::DecorationRole, QVariant());
            item(r, c)->setToolTip(QString());
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
    if (columnCount() == 0) {
        setColumnCount(1);
        prepareColumn(0);
    }
    setCurrentCell(0, qMax(0, currentColumn()));
    clearCellFeedback();
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
    item(r, c)->setData(ReactionDelegate::ErrorRole, true);
    setCurrentCell(r, c);
    scrollToItem(item(r, c));
    setFocus();
}

void ReactionTable::markRowError(int row, const QString &message) {
    QSignalBlocker block(this);
    for (int c = 0; c < columnCount(); ++c) {
        item(row, c)->setData(ReactionDelegate::ErrorRole, true);
        item(row, c)->setToolTip(message);
    }
}

void ReactionTable::keyPressEvent(QKeyEvent *event) {
    if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) &&
        state() != QAbstractItemView::EditingState) {
        if (!event->isAutoRepeat()) removeStep();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!event->isAutoRepeat())
            insertStep();
        event->accept();
        return;
    }
    QTableWidget::keyPressEvent(event);
}
QSize ReactionTable::minimumSizeHint() const {
    const int rowHeight = qMax(verticalHeader()->minimumSectionSize(), fontMetrics().height() + 12);
    const int scrollbarHeight = qMax(horizontalScrollBar()->sizeHint().height(),
                                    style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this));
    const int scrollbarSpacing = qMax(0, style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, nullptr, this));
    return QSize(200, rowCount() * rowHeight + horizontalHeader()->sizeHint().height() +
                          2 * frameWidth() + scrollbarHeight + scrollbarSpacing);
}
QSize ReactionTable::sizeHint() const {
    return minimumSizeHint();
}
void ReactionTable::updateTableSize() {
    const int columnWidth = fontMetrics().horizontalAdvance(QStringLiteral("00000000")) + 12;
    if (horizontalHeader()->minimumSectionSize() != columnWidth) {
        horizontalHeader()->setMinimumSectionSize(columnWidth);
        horizontalHeader()->setDefaultSectionSize(columnWidth);
    }
    const int rowHeight = qMax(verticalHeader()->minimumSectionSize(), fontMetrics().height() + 12);
    if (verticalHeader()->defaultSectionSize() != rowHeight)
        verticalHeader()->setDefaultSectionSize(rowHeight);
    const int height = minimumSizeHint().height();
    if (minimumHeight() != height || maximumHeight() != height) {
        setFixedHeight(height);
        updateGeometry();
    }
}
void ReactionTable::changeEvent(QEvent *event) {
    QTableWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
        updateTableSize();
    }
}
