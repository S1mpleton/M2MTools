#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "validation/compositevalidator.h"
#include "validation/editvalidator.h"
#include "validation/listnamesvalidator.h"

#include "infrastructure/logger.h"

#include "ui/presenters/errorpresenterfactory.h"
#include "ui/delegates/mealydelegate.h"
#include "ui/delegates/mooredelegate.h"
#include "ui/variantslist.h"
#include "ui/dialogs/resultmessenger.h"

#include "core/namelistparser.h"

#include <QDebug>
#include <QPushButton>
#include <QRegularExpression>
#include <QMessageBox>
#include <QAction>
#include <QActionGroup>

MainWindow::MainWindow(VariantService* service, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_variantService(service)
    , m_translator(new TranslationManager(this))
{
    ui->setupUi(this);
    // setWindowTitle("M2MTools");

    m_translator->loadSystemLocale();

    m_logWindow = new LogWindow(this);

    CompositeValidator *composite = new CompositeValidator(this);
    composite->addValidator(new EditValidator());
    composite->addValidator(new ListNamesValidator());

    ui->stateNamesLineEdit->setValidator(composite);
    ui->InputSignalNamesLineEdit->setValidator(composite);
    ui->OutputSignalNamesLineEdit->setValidator(composite);

    ui->transitionTableWidget->setItemDelegate(new MealyDelegate(&m_data, this));

    setupLanguageGroup();
    setupTypeСheckGroup();
    setupPresenters();
    setupConnections();
    refreshTable();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupPresenters() {
    m_stateNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->stateNamesLineEdit, ui->statusbar, this);

    m_inputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->InputSignalNamesLineEdit, ui->statusbar, this);

    m_outputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->OutputSignalNamesLineEdit, ui->statusbar, this);

    m_cellPresenter = m_cellPresenter = new TableCellErrorPresenter(
        ui->transitionTableWidget, 0, 0, ui->statusbar, this);
}

void MainWindow::setupConnections() {
    // header signals
    connect(ui->actionShowLogger, &QAction::triggered,
            this, &MainWindow::onShowLogClicked);
    connect(ui->actionLoadVariant, &QAction::triggered,
            this, &MainWindow::onLoadVariantPushButton);
    connect(ui->actionSaveVariant, &QAction::triggered,
            this, &MainWindow::onSavePushButtonClicked);

    connect(ui->actionExit, &QAction::triggered,
            this, &MainWindow::onExitAction);

    // Body signals
    connect(ui->variantTypeComboBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onVariantTypeChanged);
    connect(ui->numberVariantSpinBox, &QSpinBox::valueChanged,
            this, &MainWindow::onVariantNumberChanged);

    connect(ui->stateNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onStateNamesChanged);
    connect(ui->InputSignalNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onInputNamesChanged);
    connect(ui->OutputSignalNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onOutputNamesChanged);
    connect(ui->transitionTableWidget, &QTableWidget::cellChanged,
            this, &MainWindow::onTableCellChanged);

    connect(ui->savePushButton, &QPushButton::clicked,
            this, &MainWindow::onSavePushButtonClicked);
    connect(ui->loadVariantPushButton, &QPushButton::clicked,
            this, &MainWindow::onLoadVariantPushButton);

    // connect(m_translator, &TranslationManager::languageChanged,
    //         m_logWindow, &LogWindow::onLanguageUpdate);

}


void MainWindow::setupLanguageGroup() {
    auto* group = new QActionGroup(this);
    group->setObjectName("languageGroup");
    group->setExclusive(true);

    group->addAction(ui->actionSetLangEn);
    group->addAction(ui->actionSetLangRu);

    connect(group, &QActionGroup::triggered,
            this, &MainWindow::onLanguageSelected);

    const QString current = m_translator->currentLocale();
    ui->actionSetLangEn->setChecked(current == "en");
    ui->actionSetLangRu->setChecked(current != "en");
}

void MainWindow::setupTypeСheckGroup() {
    auto* group = new QActionGroup(this);
    group->setObjectName("typeCheckGroup");
    group->setExclusive(true);

    group->addAction(ui->actionTypeSoft);
    group->addAction(ui->actionTypeStrict);

    connect(group, &QActionGroup::triggered,
            this, &MainWindow::onTypeCheckSelected);

    const ValidationLevel level = m_data.getValidationLevel();
    ui->actionTypeSoft->setChecked(level == ValidationLevel::Soft);
    ui->actionTypeStrict->setChecked(level == ValidationLevel::Strict);
}

void MainWindow::onLanguageSelected(QAction* action) {
    QString locale;
    if (action == ui->actionSetLangEn) locale = QStringLiteral("en");
    else if (action == ui->actionSetLangRu) locale = QStringLiteral("ru_RU");

    Result r = m_translator->setLocale(locale);
    if (!r.ok()) {
        ResultMessenger::show(this, r);
    }

    refreshTable();
}

void MainWindow::onTypeCheckSelected(QAction* action) {
    Result r = Result::success();

    if (action == ui->actionTypeSoft) r = m_data.setStateUsageChecker(ValidationLevel::Soft);
    else if (action == ui->actionTypeStrict) r = m_data.setStateUsageChecker(ValidationLevel::Strict);

    if (!r.ok()) {
        ResultMessenger::show(this, r);
    }

}

// Logger
void MainWindow::onShowLogClicked() {
    m_logWindow->show();
    m_logWindow->raise();
    m_logWindow->activateWindow();
}


// Database
void MainWindow::onLoadVariantPushButton() {
    VariantsList dialog(m_variantService, this);
    if (dialog.exec() == QDialog::Accepted && dialog.hasSelection()) {
        const int number = dialog.selectedVariantNumber();

        Result r = m_variantService->loadVariant(number);
        Logger::log(r);

        if (!r.ok()) {
            ResultMessenger::show(this, r);
            return;
        }
        if (!r.hasPayload()) {
            ResultMessenger::showWarning(this,  tr("Variant №%1 not found").arg(number));
            return;
        }

        m_data = r.payloadAs<AutomatonData>();

        refreshTable();
        refreshData();

        ui->statusbar->showMessage(tr("Variant №%1 loaded").arg(number), 3000);
    }

}

void MainWindow::onSavePushButtonClicked(){
    Result validData = m_data.validate();
    Logger::log(validData);
    if (!validData.ok()) {
        ResultMessenger::show(this, validData);
        return;
    }

    Result existResult = m_variantService->isExist(m_data.getVariantNumber());
    Logger::log(existResult);
    if (!existResult.ok()) {
        ResultMessenger::show(this, existResult);
        return;
    }

    const bool exists = existResult.payloadAs<bool>();
    if (exists) {
        bool isDelete = ResultMessenger::askConfirm(
            this,
            tr("Do you want remove variant №%1?").arg(m_data.getVariantNumber()),
            tr("The variant №%1 is exists.").arg(m_data.getVariantNumber()));


        if (isDelete) {
            Result remuveResult = m_variantService->removeVariant(m_data.getVariantNumber());
            Logger::log(remuveResult);
            if (!remuveResult.ok()) {
                ResultMessenger::show(this, remuveResult);
                return;
            }

        } else {
            return;
        }
    }

    Result resultSaveVariant = m_variantService->saveVariant(m_data);
    Logger::log(resultSaveVariant);
    if (!resultSaveVariant.ok()) {
        ResultMessenger::show(this, resultSaveVariant);
        return;
    }

    ui->statusbar->showMessage(
        tr("Variant №%1 has been saved.").arg(m_data.getVariantNumber()), 3000);
}

// Other
void MainWindow::onExitAction() {
    QApplication::quit();
}


// Variant
void MainWindow::onVariantTypeChanged(int index) {
    if (!index){
        m_data.setVariantType(VariantType::MealyToMoore);
        ui->transitionTableWidget->setItemDelegate(new MealyDelegate(&m_data, this));
    } else {
        m_data.setVariantType(VariantType::MooreToMealy);
        ui->transitionTableWidget->setItemDelegate(new MooreDelegate(&m_data, this));
    }

    refreshTable();
}

void MainWindow::onVariantNumberChanged(int number) {
    m_data.setVariantNumber(number);
}

// STATE signals
void MainWindow::onStateNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    Result result = m_data.setStateNames(names);

    Logger::log(result);

    if (!result.ok()) {
        m_stateNamesPresenter->show(result);
        return;
    }

    m_stateNamesPresenter->clear(result);

    refreshTable();
}

// INPUT signals
void MainWindow::onInputNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    Result result = m_data.setInputSignalNames(names);

    Logger::log(result);

    if (!result.ok()) {
        m_inputNamesPresenter->show(result);
        return;
    }

    m_inputNamesPresenter->clear(result);
    refreshTable();
}

// OUTPUT signals
void MainWindow::onOutputNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    Result result = m_data.setOutputSignalNames(names);

    Logger::log(result);

    if (!result.ok()) {
        m_outputNamesPresenter->show(result);
        return;
    }

    m_outputNamesPresenter->clear(result);
}

void MainWindow::onTableCellChanged(int row, int col) {
    if (m_updatingUi) return;

    auto* item = ui->transitionTableWidget->item(row, col);
    if (!item) return;

    const QString text = item->text();
    const CellKind kind = m_data.cellKind(row, col);

    const Result result = (kind == CellKind::MooreOutput)
        ? m_data.setMooreOutputCell(col, text)
        : m_data.setTransitionCell(row, col, text);

    m_cellPresenter->setCoordinates(row, col);

    Logger::log(result);

    if (result.ok()) {
        m_cellPresenter->clear(result);
    } else {
        m_cellPresenter->show(result);
    }
}

// Ui
void MainWindow::refreshTable() {
    m_updatingTable = true;
    m_updatingUi = true;

    QTableWidget* t = ui->transitionTableWidget;
    t->clear();

    // --- 1. Size ---
    const int stateCount = m_data.getStateNames().size();
    const int inputCount = m_data.getInputSignalNames().size();
    const int rowOffset = (m_data.getType() == VariantType::MooreToMealy) ? 2 : 1;

    t->setColumnCount(stateCount + 1);
    t->setRowCount(inputCount + rowOffset);

    t->setHorizontalHeaderLabels({});
    t->setVerticalHeaderLabels({});

    // --- 2. Column headers (states / outputs) ---
    for (int j = 0; j < stateCount; ++j) {
        const QString state = m_data.getStateNames()[j];

        if (m_data.getType() == VariantType::MooreToMealy) {
            QString out = m_data.getMooreOutputs().value(state).join(", ");
            if (out.isEmpty()) out = "—";

            auto* outItem = new QTableWidgetItem(out);
            outItem->setFlags(outItem->flags() | Qt::ItemIsEditable);
            t->setItem(0, j + 1, outItem);

            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(1, j + 1, stateItem);
        } else {
            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(0, j + 1, stateItem);
        }
    }

    // --- 3. Corner cells ---
    if (m_data.getType() == VariantType::MooreToMealy) {
        auto* corner1 = new QTableWidgetItem(tr("Output signal"));
        corner1->setFlags(corner1->flags() & ~Qt::ItemIsEditable);
        t->setItem(0, 0, corner1);

        auto* corner2 = new QTableWidgetItem(tr("State"));
        corner2->setFlags(corner2->flags() & ~Qt::ItemIsEditable);
        t->setItem(1, 0, corner2);
    } else {
        auto* corner = new QTableWidgetItem(tr("State"));
        corner->setFlags(corner->flags() & ~Qt::ItemIsEditable);
        t->setItem(0, 0, corner);
    }

    // --- 4. Row headers (input signals) ---
    for (int i = 0; i < inputCount; ++i) {
        auto* item = new QTableWidgetItem(m_data.getInputSignalNames()[i]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        t->setItem(i + rowOffset, 0, item);
    }

    // --- 5. Transition cells ---
    const auto& table = m_data.getTransitionTable();
    for (int i = 0; i < inputCount; ++i) {
        for (int j = 0; j < stateCount; ++j) {
            const CellData& cell = table[i][j];
            QString text;

            if (m_data.getType() == VariantType::MooreToMealy) {
                text = cell.nextState.isEmpty() ? "—" : cell.nextState;
            } else {
                QString state = cell.nextState.isEmpty() ? "—" : cell.nextState;
                QString out = cell.outputSignals.isEmpty()
                                  ? "—"
                                  : cell.outputSignals.join(", ");
                text = state + " / " + out;
            }

            auto* item = new QTableWidgetItem(text);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            t->setItem(i + rowOffset, j + 1, item);
        }
    }

    m_updatingTable = false;
    m_updatingUi = false;
}

void MainWindow::refreshData() {
    if (m_data.getType() == VariantType::MealyToMoore){
        ui->variantTypeComboBox->setCurrentIndex(0);
    } else {
        ui->variantTypeComboBox->setCurrentIndex(1);
    }

    ui->numberVariantSpinBox->setValue(m_data.getVariantNumber());
}



