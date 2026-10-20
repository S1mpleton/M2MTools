#include "variantslist.h"
#include "ui_variantslist.h"

#include "infrastructure/logger.h"
#include "core/automatondata.h"
#include "ui/dialogs/resultmessenger.h"

#include <QMessageBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QIntValidator>
#include <QPushButton>
#include <QSet>

VariantsList::VariantsList(VariantService *service, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VariantsList)
    , m_service(service)
{
    ui->setupUi(this);
    setWindowTitle(tr("Select variant"));


    configureTable();
    configureSwapControls();
    populateTable();

    connect(ui->loadPushButton,   &QPushButton::clicked,
            this, &VariantsList::onLoadClicked);
    connect(ui->deletePushButton, &QPushButton::clicked,
            this, &VariantsList::onDeleteClicked);
    connect(ui->variantsTableWidget, &QTableWidget::itemSelectionChanged,
            this, &VariantsList::onSelectionChanged);
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
    ui->variantsTableWidget->setColumnCount(3);
    table->setHorizontalHeaderLabels({
        tr("Number"), tr("Type"), QStringLiteral("Изображение")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
}

void VariantsList::configureSwapControls() {
    auto* group = new QGroupBox(QStringLiteral("Поменять варианты местами"), this);
    auto* layout = new QFormLayout(group);
    m_swapFirst = new QComboBox(group);
    m_swapSecond = new QComboBox(group);
    for (QComboBox* combo : {m_swapFirst, m_swapSecond}) {
        combo->setEditable(true);
        combo->setInsertPolicy(QComboBox::NoInsert);
        combo->setValidator(new QIntValidator(1, 999999, combo));
        combo->setCurrentIndex(-1);
        combo->setPlaceholderText(QStringLiteral("Введите номер"));
    }
    m_swapFirst->setObjectName(QStringLiteral("swapFirstComboBox"));
    m_swapSecond->setObjectName(QStringLiteral("swapSecondComboBox"));
    m_swapButton = new QPushButton(QStringLiteral("Поменять"), group);
    m_swapButton->setObjectName(QStringLiteral("swapVariantsButton"));
    layout->addRow(QStringLiteral("Первый номер"), m_swapFirst);
    layout->addRow(QStringLiteral("Второй номер"), m_swapSecond);
    layout->addRow(m_swapButton);
    ui->gridLayout->addWidget(group, 1, 1);
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

        auto* imageItem = new QTableWidgetItem(
            v.imageName.isEmpty() ? QStringLiteral("Нет") : QStringLiteral("Да"));
        imageItem->setToolTip(v.imageName);
        table->setItem(i, 2, imageItem);
    }
    populateSwapControls(variants);
}

void VariantsList::populateSwapControls(const QList<VariantRow>& variants) {
    const QString firstText = m_swapFirst->currentText();
    const QString secondText = m_swapSecond->currentText();
    for (QComboBox* combo : {m_swapFirst, m_swapSecond}) {
        combo->clear();
        for (const VariantRow& variant : variants)
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

    bool isOk = ResultMessenger::askConfirm(this, tr("Do you want delete varinat №%1?")
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

    Result allResult = m_service->findAllVariants();
    if (!allResult.ok()) return;
    const auto variants = allResult.payloadAs<QList<VariantRow>>();
    QSet<int> numbers;
    int maximum = 0;
    for (const VariantRow& variant : variants) {
        numbers.insert(variant.number);
        maximum = qMax(maximum, variant.number);
    }
    int gap = 0;
    for (int number = 1; number < maximum; ++number) {
        if (!numbers.contains(number)) {
            gap = number;
            break;
        }
    }
    if (gap > 0 && ResultMessenger::askConfirm(
            this,
            QStringLiteral("Перенумеровать варианты?"),
            QStringLiteral("Вариант №%1 пустой. Сдвинуть следующие номера?").arg(gap))) {
        Result compactResult = m_service->compactVariantNumbers();
        Logger::log(compactResult);
        if (!compactResult.ok())
            ResultMessenger::show(this, compactResult);
        else {
            emit variantsCompacted(gap);
            populateTable();
        }
    }
}

void VariantsList::onSwapClicked() {
    bool firstOk = false;
    bool secondOk = false;
    const int first = m_swapFirst->currentText().trimmed().toInt(&firstOk);
    const int second = m_swapSecond->currentText().trimmed().toInt(&secondOk);
    if (!firstOk || !secondOk) {
        ResultMessenger::showWarning(
            this, QStringLiteral("Поле № варианта не заполнено."));
        if (!firstOk) m_swapFirst->setFocus();
        else m_swapSecond->setFocus();
        return;
    }
    if (first == second) {
        ResultMessenger::showWarning(
            this, QStringLiteral("Укажите два разных номера вариантов."));
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
        this, QStringLiteral("Варианты"),
        QStringLiteral("Номера №%1 и №%2 успешно поменяны.").arg(first).arg(second));
}


