#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "compositevalidator.h"
#include "editvalidator.h"
#include "errorpresenterfactory.h"
#include "listnamesvalidator.h"
#include "mealydelegate.h"
#include "mooredelegate.h"
#include "namelistparser.h"

#include <QDebug>
#include <QRegularExpression>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // m_highlighter = new FieldErrorHighlighter(ui->statusbar, this);

    CompositeValidator *composite = new CompositeValidator(this);
    composite->addValidator(new EditValidator());
    composite->addValidator(new ListNamesValidator());

    ui->stateNamesLineEdit->setValidator(composite);
    ui->InputSignalNamesLineEdit->setValidator(composite);
    ui->OutputSignalNamesLineEdit->setValidator(composite);

    ui->transitionTableWidget->setItemDelegate(new MealyDelegate(&m_data, this));

    setupPresenters();
    setupConnections();
    refreshTable();
}

MainWindow::~MainWindow()
{
    delete ui;
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
}

void MainWindow::onVariantTypeChanged(int index) {
    qDebug() << index;

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
    qDebug() << number;

    m_data.setVariantNumber(number);
}

void MainWindow::onStateNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    ValidationResult result = m_data.setStateNames(names);

    if (!result.ok) {
        m_stateNamesPresenter->show(result);
        return;
    }

    m_stateNamesPresenter->clear(result);
    refreshTable();
}

void MainWindow::onInputNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    ValidationResult result = m_data.setInputSignalNames(names);

    if (!result.ok) {
        m_inputNamesPresenter->show(result);
        return;
    }

    m_inputNamesPresenter->clear(result);
    refreshTable();
}

void MainWindow::onOutputNamesChanged(const QString& text) {
    if (m_updatingUi) return;

    QStringList names = NameListParser::parse(text);
    ValidationResult result = m_data.setOutputSignalNames(names);

    if (!result.ok) {
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

    const ValidationResult r = (kind == CellKind::MooreOutput)
        ? m_data.setMooreOutputCell(col, text)
        : m_data.setTransitionCell(row, col, text);

    m_cellPresenter->setCoordinates(row, col);

    if (r.ok) {
        m_cellPresenter->clear(r);
    } else {
        m_cellPresenter->show(r);
    }
}

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
        auto* corner1 = new QTableWidgetItem("Output signal");
        corner1->setFlags(corner1->flags() & ~Qt::ItemIsEditable);
        t->setItem(0, 0, corner1);

        auto* corner2 = new QTableWidgetItem("State");
        corner2->setFlags(corner2->flags() & ~Qt::ItemIsEditable);
        t->setItem(1, 0, corner2);
    } else {
        auto* corner = new QTableWidgetItem("State");
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



