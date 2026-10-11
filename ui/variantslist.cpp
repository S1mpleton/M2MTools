#include "variantslist.h"
#include "ui_variantslist.h"

#include "infrastructure/logger.h"
#include "core/automatondata.h"
#include "ui/dialogs/resultmessenger.h"

#include <QMessageBox>

VariantsList::VariantsList(VariantService *service, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VariantsList)
    , m_service(service)
{
    ui->setupUi(this);
    setWindowTitle(tr("Select variant"));


    configureTable();
    populateTable();

    connect(ui->loadPushButton,   &QPushButton::clicked,
            this, &VariantsList::onLoadClicked);
    connect(ui->deletePushButton, &QPushButton::clicked,
            this, &VariantsList::onDeleteClicked);
    connect(ui->variantsTableWidget, &QTableWidget::itemSelectionChanged,
            this, &VariantsList::onSelectionChanged);

    ui->loadPushButton->setEnabled(false);
    ui->deletePushButton->setEnabled(false);
}

VariantsList::~VariantsList()
{
    delete ui;

}

void VariantsList::configureTable() {
    auto* table = ui->variantsTableWidget;
    ui->variantsTableWidget->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Number"), tr("Type")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
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

    const auto variants = r.payloadAs<QList<VariantRow>>();
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
    }
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

    bool isOk = ResultMessenger::askConfirm(this, tr("Do you want delete varinat №%1?")
                                        .arg(ref->number));

    if (!isOk) return;

    Result r = m_service->removeVariant(ref->number);

    Logger::log(r);

    if (!r.ok()) {
        ResultMessenger::show(this, r);
        return;
    }

    populateTable();
}


