#include "variantslist.h"
#include "ui_variantslist.h"

#include "infrastructure/logger.h"
#include "core/reactionvariant.h"
#include "ui/dialogs/resultmessenger.h"

#include "ui/appicons.h"
#include <QMessageBox>
#include <QComboBox>
#include <QIntValidator>
#include <QPushButton>

#include <algorithm>

namespace {
enum class VariantSort {
    NumberAscending,
    NumberDescending,
};

enum class VariantTypeFilter {
    All,
    MealyToMoore,
    MooreToMealy,
};

QList<VariantRow> sortedVariants(QList<VariantRow> variants,
                                 VariantSort sort) {
    std::stable_sort(
        variants.begin(), variants.end(),
        [sort](const VariantRow& left, const VariantRow& right) {
            return sort == VariantSort::NumberAscending
                ? left.number < right.number
                : left.number > right.number;
        });
    return variants;
}

QList<VariantRow> filteredVariants(
    const QList<VariantRow>& variants, VariantTypeFilter filter) {
    if (filter == VariantTypeFilter::All) return variants;

    const VariantType expected = filter == VariantTypeFilter::MealyToMoore
        ? VariantType::MealyToMoore : VariantType::MooreToMealy;
    QList<VariantRow> filtered;
    for (const VariantRow& variant : variants) {
        if (variantTypeStringToType(variant.conversion) == expected)
            filtered.append(variant);
    }
    return filtered;
}
}

VariantsList::VariantsList(VariantService *service, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VariantsList)
    , m_service(service)
{
    ui->setupUi(this);



    ui->loadPushButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    ui->deletePushButton->setIcon(AppIcons::icon(AppIcons::Kind::Remove));
    configureTable();
    configureSwapControls();
    populateTable();

    connect(ui->loadPushButton,   &QPushButton::clicked,
            this, &VariantsList::onLoadClicked);
    connect(ui->deletePushButton, &QPushButton::clicked,
            this, &VariantsList::onDeleteClicked);
    connect(ui->variantsTableWidget, &QTableWidget::itemSelectionChanged,
            this, &VariantsList::onSelectionChanged);
    connect(ui->variantSortComboBox, &QComboBox::currentIndexChanged,
            this, [this] { populateTable(); });
    connect(ui->variantTypeFilterComboBox, &QComboBox::currentIndexChanged,
            this, [this] { populateTable(); });
    connect(m_swapButton, &QPushButton::clicked,
            this, &VariantsList::onSwapClicked);

    ui->loadPushButton->setEnabled(false);
    ui->deletePushButton->setEnabled(false);
}

VariantsList::~VariantsList()
{
    delete ui;

}

void VariantsList::configureTable() {
    auto* table = ui->variantsTableWidget;
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    for (int i = 0; i < ui->variantSortComboBox->count(); ++i)
        ui->variantSortComboBox->setItemData(i, i);
    for (int i = 0; i < ui->variantTypeFilterComboBox->count(); ++i)
        ui->variantTypeFilterComboBox->setItemData(i, i);
}

void VariantsList::configureSwapControls() {
    m_swapFirst = ui->swapFirstComboBox;
    m_swapSecond = ui->swapSecondComboBox;
    m_swapButton = ui->swapVariantsButton;
    for (QComboBox *combo : {m_swapFirst, m_swapSecond}) {
        combo->setValidator(new QIntValidator(1, 999999, combo));
        combo->setCurrentIndex(-1);
    }
    m_swapButton->setIcon(AppIcons::icon(AppIcons::Kind::Swap));
}

void VariantsList::populateTable() {
    auto* table = ui->variantsTableWidget;

    // table->clear();
    table->setRowCount(0);

    Result r = m_service->findAllVariants();
    Logger::log(r);

    if (!r.ok()) {
        ResultMessenger::show(this, r);
        return;
    }

    const QList<VariantRow> allVariants =
        r.payloadAs<QList<VariantRow>>();
    const auto sort = static_cast<VariantSort>(
        ui->variantSortComboBox->currentData().toInt());
    const auto filter = static_cast<VariantTypeFilter>(
        ui->variantTypeFilterComboBox->currentData().toInt());
    const auto variants = sortedVariants(
        filteredVariants(allVariants, filter), sort);
    table->setRowCount(variants.size());

    for (int i = 0; i < variants.size(); ++i) {
        const VariantRow& v = variants[i];

        auto* numberItem = new QTableWidgetItem(QString::number(v.number));
        numberItem->setData(Qt::UserRole, v.number);
        numberItem->setData(Qt::UserRole + 1, static_cast<int>(variantTypeStringToType(v.conversion)));
        table->setItem(i, 0, numberItem);

        auto* typeItem = new QTableWidgetItem(
            variantTypeDisplayName(variantTypeStringToType(v.conversion)));
        table->setItem(i, 1, typeItem);

        table->setItem(i,2,new QTableWidgetItem(QString::number(v.stepCount)));
    }
    populateSwapControls(allVariants);
}

void VariantsList::populateSwapControls(const QList<VariantRow>& variants) {
    const QString firstText = m_swapFirst->currentText();
    const QString secondText = m_swapSecond->currentText();
    const QList<VariantRow> variantsByNumber = sortedVariants(
        variants, VariantSort::NumberAscending);
    for (QComboBox* combo : {m_swapFirst, m_swapSecond}) {
        combo->clear();
        for (const VariantRow& variant : variantsByNumber)
            combo->addItem(QString::number(variant.number));
        combo->setCurrentIndex(-1);
    }
    m_swapFirst->setEditText(firstText);
    m_swapSecond->setEditText(secondText);
}

void VariantsList::onSelectionChanged() {
    const bool hasSelection = ui->variantsTableWidget->currentRow() >= 0;

    ui->loadPushButton->setEnabled(hasSelection);
    ui->deletePushButton->setEnabled(hasSelection);
}

std::optional<VariantsList::VariantRef> VariantsList::variantRefAt(int row) const {
    if (row < 0 || row >= ui->variantsTableWidget->rowCount())
        return std::nullopt;

    auto* item = ui->variantsTableWidget->item(row, 0);
    if (!item) return std::nullopt;

    const int number = item->data(Qt::UserRole).toInt();
    const int typeInt = item->data(Qt::UserRole + 1).toInt();

    return VariantRef{ number, static_cast<VariantType>(typeInt) };
}


void VariantsList::onLoadClicked() {
    const int row = ui->variantsTableWidget->currentRow();
    auto ref = variantRefAt(row);
    if (!ref) return;

    m_selectedNumber = ref->number;
    m_selectedType = ref->type;
    m_hasSelection = true;

    accept();
}

void VariantsList::onDeleteClicked() {
    const int row = ui->variantsTableWidget->currentRow();
    auto ref = variantRefAt(row);
    if (!ref) return;

    bool isOk = ResultMessenger::askConfirm(this, tr("Do you want to delete variant #%1?")
                                        .arg(ref->number));

    if (!isOk) return;

    Result r = m_service->removeVariant(ref->number);

    Logger::log(r);

    if (!r.ok()) {
        ResultMessenger::show(this, r);
        return;
    }

    emit variantDeleted(ref->number);
    populateTable();
}

void VariantsList::onSwapClicked() {
    bool firstOk = false;
    bool secondOk = false;
    const int first = m_swapFirst->currentText().trimmed().toInt(&firstOk);
    const int second = m_swapSecond->currentText().trimmed().toInt(&secondOk);
    if (!firstOk || !secondOk) {
        ResultMessenger::showWarning(
            this, tr("The variant number field is empty."));
        if (!firstOk) m_swapFirst->setFocus();
        else m_swapSecond->setFocus();
        return;
    }
    if (first == second) {
        ResultMessenger::showWarning(
            this, tr("Specify two different variant numbers."));
        return;
    }

    Result result = m_service->swapVariants(first, second);
    Logger::log(result);
    if (!result.ok()) {
        ResultMessenger::show(this, result);
        return;
    }
    emit variantsSwapped(first, second);
    populateTable();
    QMessageBox::information(
        this, tr("Variants"),
        tr("Numbers #%1 and #%2 were swapped successfully.").arg(first).arg(second));
}


