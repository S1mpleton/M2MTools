#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "editvalidator.h"

#include <QDebug>
#include <QRegularExpression>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    EditValidator *validator = new EditValidator(this);

    ui->stateNamesLineEdit->setValidator(validator);
    ui->InputSignalNamesLineEdit->setValidator(validator);
    ui->OutputSignalNamesLineEdit->setValidator(validator);

    setupConnections();
    refreshTable();
}

MainWindow::~MainWindow()
{
    delete ui;
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

}

QStringList MainWindow::parseNames(const QString& text) const {
    QStringList result;
    for (const QString& part : text.split(", ", Qt::SkipEmptyParts)) {
        QString trimmed = part.trimmed();
        if (!trimmed.isEmpty())
            result.append(trimmed);
    }
    qDebug() << "parseNames " << result;
    return result;
}

void MainWindow::onVariantTypeChanged(int index) {
    qDebug() << index;

    if (!index){
        m_data.setVariantType(VariantType::MealyToMoore);
    } else {
        m_data.setVariantType(VariantType::MooreToMealy);
    }

    refreshTable();
}

void MainWindow::onVariantNumberChanged(int number) {
    qDebug() << number;

    m_data.setVariantNumber(number);
}

void MainWindow::onStateNamesChanged(const QString& text) {
    QStringList names = parseNames(text);
    if (names == m_data.getStateNames()) return;

    m_data.setStateNames(names);
    refreshTable();
}


void MainWindow::onInputNamesChanged(const QString& text) {
    QStringList names = parseNames(text);
    if (names == m_data.getInputSignalNames()) return;

    m_data.setInputSignalNames(names);
    refreshTable();
}

void MainWindow::onOutputNamesChanged(const QString& text) {
    QStringList names = parseNames(text);
    if (names == m_data.getOutputSignalNames()) return;
    m_data.setOutputSignalNames(names);
}

void MainWindow::refreshTable() {
    // m_updatingTable = true;  // защита от рекурсии

    QTableWidget* t = ui->transitionTableWidget;

    // --- 1. Размеры ---
    const int stateCount = m_data.getStateNames().size();
    const int inputCount = m_data.getInputSignalNames().size();

    if (m_data.getType() == VariantType::MooreToMealy) {
        // +1 столбец под подписи входов,
        // +2 строки: "Вых." и "Сост."
        t->setColumnCount(stateCount + 1);
        t->setRowCount(inputCount + 2);
    } else {
        // +1 столбец, +1 строка
        t->setColumnCount(stateCount + 1);
        t->setRowCount(inputCount + 1);
    }

    t->setHorizontalHeaderLabels({});  // убираем стандартные номера
    t->setVerticalHeaderLabels({});

    // --- 2. Заголовки столбцов (имена состояний) ---
    for (int j = 0; j < stateCount; ++j) {
        const QString state = m_data.getStateNames()[j];
        if (m_data.getType() == VariantType::MooreToMealy) {
            // Строка "Вых." — выход для состояния
            QString out = m_data.getMooreOutputs().value(state).join(",");
            if (out.isEmpty()) out = "—";
            auto* outItem = new QTableWidgetItem(out);
            outItem->setFlags(outItem->flags() | Qt::ItemIsEditable);
            t->setItem(0, j + 1, outItem);

            // Строка "Сост." — имя состояния (только для чтения)
            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(1, j + 1, stateItem);
        } else {
            // Мили: одна строка заголовков — имена состояний
            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            t->setItem(0, j + 1, stateItem);
        }
    }

    // Уголки (пустые ячейки) — только для чтения
    if (m_data.getType() == VariantType::MooreToMealy) {
        auto* corner1 = new QTableWidgetItem("Вых.");
        corner1->setFlags(corner1->flags() & ~Qt::ItemIsEditable);
        t->setItem(0, 0, corner1);

        auto* corner2 = new QTableWidgetItem("Сост.");
        corner2->setFlags(corner2->flags() & ~Qt::ItemIsEditable);
        t->setItem(1, 0, corner2);
    } else {
        auto* corner = new QTableWidgetItem("Сост.");
        corner->setFlags(corner->flags() & ~Qt::ItemIsEditable);
        t->setItem(0, 0, corner);
    }

    // --- 3. Заголовки строк (имена входов) ---
    const int rowOffset = (m_data.getType() == VariantType::MooreToMealy) ? 2 : 1;
    for (int i = 0; i < inputCount; ++i) {
        auto* item = new QTableWidgetItem(m_data.getInputSignalNames()[i]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        t->setItem(i + rowOffset, 0, item);
    }

    // --- 4. Ячейки переходов ---
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
                                  : cell.outputSignals.join(",");
                text = state + " / " + out;
            }

            auto* item = new QTableWidgetItem(text);
            t->setItem(i + rowOffset, j + 1, item);
        }
    }

    // m_updatingTable = false;
}


