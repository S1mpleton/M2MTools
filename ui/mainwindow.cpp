#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "core/namelistparser.h"
#include "infrastructure/logger.h"
#include "ui/delegates/mealydelegate.h"
#include "ui/delegates/mooredelegate.h"
#include "ui/dialogs/resultmessenger.h"
#include "ui/presenters/errorpresenterfactory.h"
#include "ui/translateresult.h"
#include "ui/variantslist.h"
#include "ui/widgets/variantimagepanel.h"
#include "validation/compositevalidator.h"
#include "validation/editvalidator.h"
#include "validation/listnamesvalidator.h"

#include <QActionGroup>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kMaximumImageSize = 10 * 1024 * 1024;
const QColor kErrorBackground(255, 224, 224);
const QColor kIncompleteBackground(255, 244, 204);
const QColor kErrorForeground(127, 29, 29);
const QColor kIncompleteForeground(120, 53, 15);

QString displayCell(const QString& value) {
    return value.trimmed().isEmpty() ? QStringLiteral("—") : value;
}
}

MainWindow::MainWindow(VariantService* service, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_variantService(service)
    , m_translator(new TranslationManager(this))
{
    ui->setupUi(this);
    m_translator->loadSystemLocale();
    m_logWindow = new LogWindow(this);

    auto* composite = new CompositeValidator(this);
    composite->addValidator(new EditValidator());
    composite->addValidator(new ListNamesValidator());
    ui->stateNamesLineEdit->setValidator(composite);
    ui->InputSignalNamesLineEdit->setValidator(composite);
    ui->OutputSignalNamesLineEdit->setValidator(composite);

    setupModernUi();
    ui->transitionTableWidget->setItemDelegate(new MealyDelegate(&m_data, this));
    setupLanguageGroup();
    setupTypeCheckGroup();
    setupPresenters();
    setupConnections();

    m_data.setVariantNumber(m_variantService->firstAvailableVariantNumber());
    refreshData();
    refreshTable();
    syncReactionFromData();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupModernUi() {
    setWindowTitle(QStringLiteral("M2MTools — редактор эквивалентных автоматов"));
    resize(1100, 720);
    setMinimumSize(900, 620);

    // Use the platform palette and widget style. A partial custom theme can
    // easily combine a dark system foreground with a forced light background.
    setStyleSheet(QString());
    ui->verticalLayout->setContentsMargins(8, 6, 8, 6);
    ui->verticalLayout->setSpacing(6);

    ui->groupBox->setTitle(QStringLiteral("Вариант"));
    ui->groupBox->setMinimumHeight(58);
    ui->groupBox->setMaximumHeight(72);
    ui->groupBox_2->setTitle(QStringLiteral("Обозначения автомата"));
    ui->groupBox_3->setTitle(QStringLiteral("Описание автомата"));
    ui->label->setText(QStringLiteral("Номер варианта"));
    ui->label_2->setText(QStringLiteral("Преобразование"));
    ui->label_3->setText(QStringLiteral("Состояния A"));
    ui->label_4->setText(QStringLiteral("Входные сигналы X"));
    ui->label_5->setText(QStringLiteral("Выходные сигналы Y"));
    ui->variantTypeComboBox->setItemText(0, QStringLiteral("Мили → Мура"));
    ui->variantTypeComboBox->setItemText(1, QStringLiteral("Мура → Мили"));
    ui->numberVariantSpinBox->setMaximum(999999);
    ui->stateNamesLineEdit->setPlaceholderText(QStringLiteral("Например: a0, a1, a2"));
    ui->InputSignalNamesLineEdit->setPlaceholderText(QStringLiteral("Например: x1, x2"));
    ui->OutputSignalNamesLineEdit->setPlaceholderText(QStringLiteral("Например: y1, y2"));
    ui->menuMain->setTitle(QStringLiteral("Файл"));
    ui->menuSettings->setTitle(QStringLiteral("Настройки"));
    ui->actionSaveVariant->setText(QStringLiteral("Сохранить вариант"));
    ui->actionLoadVariant->setText(QStringLiteral("Открыть варианты"));
    ui->actionShowLogger->setText(QStringLiteral("Журнал событий"));
    ui->actionExit->setText(QStringLiteral("Выход"));

    m_newVariantButton = new QPushButton(QStringLiteral("Новый вариант"), ui->groupBox);
    m_newVariantButton->setObjectName(QStringLiteral("newVariantButton"));
    ui->gridLayout_2->removeWidget(ui->label_2);
    ui->gridLayout_2->removeWidget(ui->variantTypeComboBox);
    ui->gridLayout_2->addWidget(ui->label_2, 0, 2);
    ui->gridLayout_2->addWidget(ui->variantTypeComboBox, 0, 3);
    ui->gridLayout_2->addWidget(m_newVariantButton, 0, 4);
    ui->gridLayout_2->setColumnStretch(3, 1);

    auto* initialStateLabel = new QLabel(QStringLiteral("Начальное состояние"), ui->groupBox_2);
    m_initialStateComboBox = new QComboBox(ui->groupBox_2);
    m_initialStateComboBox->setObjectName(QStringLiteral("initialStateComboBox"));
    ui->gridLayout_3->addWidget(initialStateLabel, 3, 0);
    ui->gridLayout_3->addWidget(m_initialStateComboBox, 3, 1);

    m_imagePanel = new VariantImagePanel(centralWidget());
    ui->verticalLayout->removeWidget(ui->groupBox_2);
    auto* detailsRow = new QWidget(centralWidget());
    detailsRow->setMinimumHeight(130);
    detailsRow->setMaximumHeight(160);
    auto* detailsLayout = new QHBoxLayout(detailsRow);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(6);
    detailsLayout->addWidget(ui->groupBox_2, 3);
    detailsLayout->addWidget(m_imagePanel, 4);
    ui->verticalLayout->insertWidget(1, detailsRow);

    ui->label_6->hide();
    ui->gridLayout_4->removeWidget(ui->transitionTableWidget);
    auto* modeLayout = new QHBoxLayout;
    auto* modeLabel = new QLabel(QStringLiteral("Редактировать по:"), ui->groupBox_3);
    m_editModeComboBox = new QComboBox(ui->groupBox_3);
    m_editModeComboBox->setObjectName(QStringLiteral("editModeComboBox"));
    m_editModeComboBox->addItems({
        QStringLiteral("Таблице переходов"),
        QStringLiteral("Реакции автомата")
    });
    auto* modeHint = new QLabel(
        QStringLiteral("Обе формы обновляются автоматически"), ui->groupBox_3);
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_editModeComboBox);
    modeLayout->addStretch();
    modeLayout->addWidget(modeHint);
    ui->gridLayout_4->addLayout(modeLayout, 0, 0);

    m_editorStack = new QStackedWidget(ui->groupBox_3);
    m_editorStack->setObjectName(QStringLiteral("editorStack"));
    auto* transitionPage = new QWidget(m_editorStack);
    auto* transitionLayout = new QVBoxLayout(transitionPage);
    transitionLayout->setContentsMargins(0, 0, 0, 0);
    ui->transitionTableWidget->setParent(transitionPage);
    ui->transitionTableWidget->setAlternatingRowColors(true);
    ui->transitionTableWidget->setMinimumHeight(155);
    transitionLayout->addWidget(ui->transitionTableWidget);
    m_editorStack->addWidget(transitionPage);

    auto* reactionPage = new QWidget(m_editorStack);
    auto* reactionLayout = new QVBoxLayout(reactionPage);
    reactionLayout->setContentsMargins(0, 0, 0, 0);
    auto* reactionHint = new QLabel(
        QStringLiteral(
            "Один столбец — один такт: A + X → A′ / Y. "
            "Имена заполняются автоматически."),
        reactionPage);
    reactionHint->setWordWrap(true);
    reactionLayout->addWidget(reactionHint);
    m_reactionTable = new QTableWidget(4, 1, reactionPage);
    m_reactionTable->setObjectName(QStringLiteral("reactionTableWidget"));
    m_reactionTable->setVerticalHeaderLabels({
        QStringLiteral("Состояние"), QStringLiteral("Вход X"),
        QStringLiteral("След. состояние"), QStringLiteral("Выход Y")
    });
    m_reactionTable->setHorizontalHeaderLabels({QStringLiteral("t1")});
    m_reactionTable->horizontalHeader()->setDefaultSectionSize(150);
    m_reactionTable->horizontalHeader()->setMinimumSectionSize(110);
    m_reactionTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reactionTable->setAlternatingRowColors(true);
    m_reactionTable->setMinimumHeight(145);
    reactionLayout->addWidget(m_reactionTable);
    auto* reactionButtons = new QHBoxLayout;
    m_addReactionStepButton = new QPushButton(QStringLiteral("+ Добавить такт"), reactionPage);
    m_addReactionStepButton->setObjectName(QStringLiteral("addReactionStepButton"));
    m_removeReactionStepButton = new QPushButton(QStringLiteral("Удалить такт"), reactionPage);
    m_removeReactionStepButton->setObjectName(QStringLiteral("removeReactionStepButton"));
    reactionButtons->addWidget(m_addReactionStepButton);
    reactionButtons->addWidget(m_removeReactionStepButton);
    reactionButtons->addStretch();
    reactionLayout->addLayout(reactionButtons);
    m_editorStack->addWidget(reactionPage);
    ui->gridLayout_4->addWidget(m_editorStack, 1, 0);
    ui->groupBox_3->setMinimumHeight(250);
    ui->verticalLayout->setStretch(2, 1);

    m_validationLabel = new QLabel(this);
    m_validationLabel->setObjectName(QStringLiteral("validationMessage"));
    m_validationLabel->setWordWrap(true);
    m_validationLabel->hide();
    ui->verticalLayout->insertWidget(
        ui->verticalLayout->count() - 1, m_validationLabel);

    ui->savePushButton->setText(QStringLiteral("Сохранить вариант"));
    ui->savePushButton->setObjectName(QStringLiteral("primaryButton"));
    ui->savePushButton->setDefault(true);
    ui->loadVariantPushButton->setText(QStringLiteral("Открыть варианты"));
    m_importArchiveButton =
        new QPushButton(QStringLiteral("Открыть BazaVar…"), this);
    m_importArchiveButton->setObjectName(
        QStringLiteral("importArchiveButton"));
    m_exportArchiveButton =
        new QPushButton(QStringLiteral("Экспорт BazaVar…"), this);
    m_exportArchiveButton->setObjectName(
        QStringLiteral("exportArchiveButton"));
    ui->horizontalLayout->addWidget(m_importArchiveButton);
    ui->horizontalLayout->addWidget(m_exportArchiveButton);
}

void MainWindow::setupPresenters() {
    m_stateNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->stateNamesLineEdit, ui->statusbar, this);
    m_inputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->InputSignalNamesLineEdit, ui->statusbar, this);
    m_outputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->OutputSignalNamesLineEdit, ui->statusbar, this);
    m_cellPresenter = new TableCellErrorPresenter(
        ui->transitionTableWidget, 0, 0, ui->statusbar, this);
}

void MainWindow::setupConnections() {
    connect(ui->actionShowLogger, &QAction::triggered,
            this, &MainWindow::onShowLogClicked);
    connect(ui->actionLoadVariant, &QAction::triggered,
            this, &MainWindow::onLoadVariantPushButton);
    connect(ui->actionSaveVariant, &QAction::triggered,
            this, &MainWindow::onSavePushButtonClicked);
    connect(ui->actionExit, &QAction::triggered,
            this, &MainWindow::onExitAction);

    connect(ui->variantTypeComboBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onVariantTypeChanged);
    connect(ui->numberVariantSpinBox, &QSpinBox::valueChanged,
            this, &MainWindow::onVariantNumberChanged);
    connect(ui->numberVariantSpinBox, &QSpinBox::editingFinished, this, [this] {
        if (!m_updatingUi) m_numberWasEdited = true;
    });
    connect(m_initialStateComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onInitialStateChanged);
    connect(ui->stateNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onStateNamesChanged);
    connect(ui->InputSignalNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onInputNamesChanged);
    connect(ui->OutputSignalNamesLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onOutputNamesChanged);
    connect(ui->transitionTableWidget, &QTableWidget::cellChanged,
            this, &MainWindow::onTableCellChanged);
    connect(m_reactionTable, &QTableWidget::cellChanged,
            this, &MainWindow::onReactionCellChanged);
    connect(m_editModeComboBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onEditModeChanged);
    connect(m_addReactionStepButton, &QPushButton::clicked,
            this, &MainWindow::onAddReactionStep);
    connect(m_removeReactionStepButton, &QPushButton::clicked,
            this, &MainWindow::onRemoveReactionStep);

    connect(ui->savePushButton, &QPushButton::clicked,
            this, &MainWindow::onSavePushButtonClicked);
    connect(ui->loadVariantPushButton, &QPushButton::clicked,
            this, &MainWindow::onLoadVariantPushButton);
    connect(m_newVariantButton, &QPushButton::clicked,
            this, &MainWindow::onNewVariantClicked);
    connect(m_imagePanel, &VariantImagePanel::imagePathSelected,
            this, [this](const QString& path) { loadImageFromPath(path); });
    connect(m_imagePanel, &VariantImagePanel::removeRequested,
            this, &MainWindow::onRemoveImageClicked);
    connect(m_exportArchiveButton, &QPushButton::clicked,
            this, &MainWindow::onExportArchiveClicked);
    connect(m_importArchiveButton, &QPushButton::clicked,
            this, &MainWindow::onImportArchiveClicked);
}

void MainWindow::setupLanguageGroup() {
    auto* group = new QActionGroup(this);
    group->setObjectName(QStringLiteral("languageGroup"));
    group->setExclusive(true);
    group->addAction(ui->actionSetLangEn);
    group->addAction(ui->actionSetLangRu);
    connect(group, &QActionGroup::triggered,
            this, &MainWindow::onLanguageSelected);
    const QString current = m_translator->currentLocale();
    ui->actionSetLangEn->setChecked(current == QStringLiteral("en"));
    ui->actionSetLangRu->setChecked(current != QStringLiteral("en"));
}

void MainWindow::setupTypeCheckGroup() {
    auto* group = new QActionGroup(this);
    group->setObjectName(QStringLiteral("typeCheckGroup"));
    group->setExclusive(true);
    group->addAction(ui->actionTypeSoft);
    group->addAction(ui->actionTypeStrict);
    connect(group, &QActionGroup::triggered,
            this, &MainWindow::onTypeCheckSelected);
    ui->actionTypeSoft->setChecked(
        m_data.getValidationLevel() == ValidationLevel::Soft);
    ui->actionTypeStrict->setChecked(
        m_data.getValidationLevel() == ValidationLevel::Strict);
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QMainWindow::changeEvent(event);
}

void MainWindow::onLanguageSelected(QAction* action) {
    const QString locale = action == ui->actionSetLangEn
        ? QStringLiteral("en") : QStringLiteral("ru_RU");
    Result result = m_translator->setLocale(locale);
    if (!result.ok()) ResultMessenger::show(this, result);
    refreshTable();
}

void MainWindow::onTypeCheckSelected(QAction* action) {
    Result result = m_data.setStateUsageChecker(
        action == ui->actionTypeStrict
            ? ValidationLevel::Strict : ValidationLevel::Soft);
    if (!result.ok()) ResultMessenger::show(this, result);
}

void MainWindow::onShowLogClicked() {
    m_logWindow->show();
    m_logWindow->raise();
    m_logWindow->activateWindow();
}

void MainWindow::onExitAction() {
    QApplication::quit();
}

void MainWindow::onVariantTypeChanged(int index) {
    if (m_updatingUi) return;
    m_data.setVariantType(index == 0
        ? VariantType::MealyToMoore : VariantType::MooreToMealy);
    ui->transitionTableWidget->setItemDelegate(index == 0
        ? static_cast<QAbstractItemDelegate*>(new MealyDelegate(&m_data, this))
        : static_cast<QAbstractItemDelegate*>(new MooreDelegate(&m_data, this)));
    refreshTable();
    syncReactionFromData();
    updateSummary();
}

void MainWindow::onVariantNumberChanged(int number) {
    if (!m_updatingUi) m_data.setVariantNumber(number);
}

void MainWindow::onInitialStateChanged(const QString& state) {
    if (m_updatingUi || state.isEmpty()) return;
    const Result result = m_data.setInitialState(state);
    if (!result.ok()) showInlineMessage(translateResult(result));
}

void MainWindow::onStateNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    const Result result = m_data.setStateNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_stateNamesPresenter->show(result);
        return;
    }
    m_stateNamesPresenter->clear(result);
    refreshInitialStates();
    refreshTable();
    syncReactionFromData();
    updateSummary();
}

void MainWindow::onInputNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    const Result result = m_data.setInputSignalNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_inputNamesPresenter->show(result);
        return;
    }
    m_inputNamesPresenter->clear(result);
    refreshTable();
    syncReactionFromData();
    updateSummary();
}

void MainWindow::onOutputNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    const Result result = m_data.setOutputSignalNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_outputNamesPresenter->show(result);
        return;
    }
    m_outputNamesPresenter->clear(result);
    syncReactionFromData();
    updateSummary();
}

void MainWindow::onTableCellChanged(int row, int col) {
    if (m_updatingTable || m_updatingUi) return;
    auto* item = ui->transitionTableWidget->item(row, col);
    if (!item) return;
    const CellKind kind = m_data.cellKind(row, col);
    if (kind != CellKind::Transition && kind != CellKind::MooreOutput) return;

    const Result result = kind == CellKind::MooreOutput
        ? m_data.setMooreOutputCell(col, item->text())
        : m_data.setTransitionCell(row, col, item->text());
    m_cellPresenter->setCoordinates(row, col);
    Logger::log(result);
    if (result.ok()) {
        m_cellPresenter->clear(result);
        clearInlineMessage();
        syncReactionFromData();
    } else {
        m_cellPresenter->show(result);
        showInlineMessage(translateResult(result) + (result.details().isEmpty()
            ? QString() : QStringLiteral(" — ") + result.details()));
    }
}

void MainWindow::onEditModeChanged(int index) {
    m_editorStack->setCurrentIndex(index);
    if (index == 1) syncReactionFromData();
}

void MainWindow::onReactionCellChanged(int row, int column) {
    if (m_updatingReaction || m_updatingUi) return;
    if (row == 2 && column + 1 < m_reactionTable->columnCount()) {
        QTableWidgetItem* nextCurrent = m_reactionTable->item(0, column + 1);
        const QTableWidgetItem* nextState = m_reactionTable->item(2, column);
        if (nextCurrent && nextCurrent->text().trimmed().isEmpty() && nextState) {
            const QSignalBlocker blocker(m_reactionTable);
            nextCurrent->setText(nextState->text().trimmed());
        }
    }
    const ReactionSyncReport report = syncDataFromReaction();
    if (report.hasErrors()) return;

    m_data = report.data;
    refreshData();
    refreshTable();
    updateSummary();
}

void MainWindow::onAddReactionStep() {
    m_updatingReaction = true;
    const int column = m_reactionTable->columnCount();
    m_reactionTable->insertColumn(column);
    m_reactionTable->setHorizontalHeaderItem(
        column, new QTableWidgetItem(QStringLiteral("t%1").arg(column + 1)));
    for (int row = 0; row < 4; ++row)
        m_reactionTable->setItem(row, column, new QTableWidgetItem);
    if (column > 0) {
        const QTableWidgetItem* previousNext =
            m_reactionTable->item(2, column - 1);
        if (previousNext)
            m_reactionTable->item(0, column)->setText(previousNext->text());
    }
    m_reactionTable->setCurrentCell(0, column);
    m_updatingReaction = false;
}

void MainWindow::onRemoveReactionStep() {
    int column = m_reactionTable->currentColumn();
    if (column < 0) column = m_reactionTable->columnCount() - 1;
    m_updatingReaction = true;
    if (m_reactionTable->columnCount() > 1) {
        m_reactionTable->removeColumn(column);
        for (int i = 0; i < m_reactionTable->columnCount(); ++i)
            m_reactionTable->setHorizontalHeaderItem(
                i, new QTableWidgetItem(QStringLiteral("t%1").arg(i + 1)));
    } else {
        for (int row = 0; row < 4; ++row) {
            if (!m_reactionTable->item(row, 0))
                m_reactionTable->setItem(row, 0, new QTableWidgetItem);
            m_reactionTable->item(row, 0)->setText(QString());
        }
    }
    m_updatingReaction = false;
    onReactionCellChanged(0, 0);
}

void MainWindow::onLoadVariantPushButton() {
    VariantsList dialog(m_variantService, this);
    connect(&dialog, &VariantsList::variantDeleted, this, [this](int number) {
        if (m_data.getVariantNumber() != number) return;
        const VariantType type = m_data.getType();
        m_data = AutomatonData();
        m_data.setVariantType(type);
        m_data.setVariantNumber(m_variantService->firstAvailableVariantNumber());
        refreshData();
        refreshTable();
        syncReactionFromData();
        showInlineMessage(
            QStringLiteral("Открытый вариант удалён. Поля очищены."), false);
    });
    connect(&dialog, &VariantsList::variantsSwapped, this,
            [this](int first, int second) {
        if (m_data.getVariantNumber() == first)
            m_data.setVariantNumber(second);
        else if (m_data.getVariantNumber() == second)
            m_data.setVariantNumber(first);
        else
            return;
        refreshData();
    });
    connect(&dialog, &VariantsList::variantsCompacted, this, [this](int gap) {
        if (m_data.getVariantNumber() > gap) {
            m_data.setVariantNumber(m_data.getVariantNumber() - 1);
            refreshData();
        }
    });
    if (dialog.exec() != QDialog::Accepted || !dialog.hasSelection()) return;

    const int number = dialog.selectedVariantNumber();
    Result result = m_variantService->loadVariant(number);
    Logger::log(result);
    if (!result.ok() || !result.hasPayload()) {
        ResultMessenger::show(this, result);
        return;
    }
    m_data = result.payloadAs<AutomatonData>();
    m_numberWasEdited = false;
    refreshData();
    refreshTable();
    syncReactionFromData();
    clearInlineMessage();
    ui->statusbar->showMessage(
        QStringLiteral("Вариант №%1 загружен").arg(number), 3000);
}

void MainWindow::onSavePushButtonClicked() {
    if (!validateBeforeSave()) return;

    Result existsResult = m_variantService->isExist(m_data.getVariantNumber());
    if (!existsResult.ok()) {
        ResultMessenger::show(this, existsResult);
        return;
    }
    const bool exists = existsResult.payloadAs<bool>();
    if (exists) {
        const bool replace = ResultMessenger::askConfirm(
            this,
            QStringLiteral("Заменить вариант №%1?").arg(m_data.getVariantNumber()),
            QStringLiteral(
                "Вариант с таким номером уже существует. Его данные будут заменены."));
        if (!replace) return;
    }

    Result result = exists
        ? m_variantService->replaceVariant(m_data)
        : m_variantService->saveVariant(m_data);
    Logger::log(result);
    if (!result.ok()) {
        ResultMessenger::show(this, result);
        return;
    }
    clearInlineMessage();
    ui->statusbar->showMessage(
        QStringLiteral("Вариант №%1 сохранён").arg(m_data.getVariantNumber()),
        3500);
}

void MainWindow::onNewVariantClicked() {
    int number = m_variantService->firstAvailableVariantNumber();
    if (m_numberWasEdited) {
        const int requested = ui->numberVariantSpinBox->value();
        Result exists = m_variantService->isExist(requested);
        if (!exists.ok()) {
            ResultMessenger::show(this, exists);
            return;
        }
        if (exists.payloadAs<bool>()) {
            ui->numberVariantSpinBox->setStyleSheet(
                QStringLiteral("border: 2px solid #dc2626;"));
            showInlineMessage(
                QStringLiteral("Вариант №%1 уже существует. Выберите другой номер.")
                    .arg(requested));
            return;
        }
        number = requested;
    }

    const VariantType type = ui->variantTypeComboBox->currentIndex() == 0
        ? VariantType::MealyToMoore : VariantType::MooreToMealy;
    m_data = AutomatonData();
    m_data.setVariantType(type);
    m_data.setVariantNumber(number);
    m_numberWasEdited = false;
    ui->numberVariantSpinBox->setStyleSheet(QString());
    refreshData();
    refreshTable();
    syncReactionFromData();
    clearInlineMessage();
    ui->stateNamesLineEdit->setFocus();
}

void MainWindow::onRemoveImageClicked() {
    m_data.clearDiagramImage();
    refreshImagePreview();
    showInlineMessage(
        QStringLiteral("Изображение удалено. Загрузите новое перед сохранением."),
        false);
}

void MainWindow::onExportArchiveClicked() {
    Result allResult = m_variantService->findAllVariants();
    if (!allResult.ok()) {
        ResultMessenger::show(this, allResult);
        return;
    }
    QList<VariantRow> rows = allResult.payloadAs<QList<VariantRow>>();
    std::sort(rows.begin(), rows.end(),
              [](const VariantRow& left, const VariantRow& right) {
                  return left.number < right.number;
              });
    int gap = 0;
    for (int i = 0; i < rows.size(); ++i) {
        if (rows[i].number != i + 1) {
            gap = i + 1;
            break;
        }
    }
    if (gap > 0 && ResultMessenger::askConfirm(
            this, QStringLiteral("Перенумеровать варианты?"),
            QStringLiteral(
                "Вариант №%1 пустой. Сдвинуть следующие номера перед экспортом?")
                .arg(gap))) {
        int newCurrentNumber = m_data.getVariantNumber();
        for (int i = 0; i < rows.size(); ++i) {
            if (rows[i].number == m_data.getVariantNumber()) {
                newCurrentNumber = i + 1;
                break;
            }
        }
        Result compactResult = m_variantService->compactVariantNumbers();
        if (!compactResult.ok()) {
            ResultMessenger::show(this, compactResult);
            return;
        }
        m_data.setVariantNumber(newCurrentNumber);
        refreshData();
    }

    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить зашифрованную базу"),
        QStringLiteral("BazaVar.m2m"),
        QStringLiteral("Архив M2MTools (*.m2m);;Все файлы (*)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".m2m");

    bool accepted = false;
    const QString password = QInputDialog::getText(
        this, QStringLiteral("Пароль BazaVar"),
        QStringLiteral("Введите пароль (не менее 8 символов):"),
        QLineEdit::Password, QString(), &accepted);
    if (!accepted) return;
    const QString confirmation = QInputDialog::getText(
        this, QStringLiteral("Подтверждение пароля"),
        QStringLiteral("Повторите пароль:"),
        QLineEdit::Password, QString(), &accepted);
    if (!accepted) return;
    if (password != confirmation) {
        showInlineMessage(QStringLiteral("Пароли не совпадают."));
        return;
    }

    Result result = m_variantService->exportArchive(path, password);
    Logger::log(result);
    if (!result.ok()) {
        ResultMessenger::show(this, result);
        showInlineMessage(result.message());
        return;
    }
    clearInlineMessage();
    ui->statusbar->showMessage(
        QStringLiteral("Зашифрованный архив BazaVar сохранён."), 4000);
}

void MainWindow::onImportArchiveClicked() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Открыть зашифрованную базу"), QString(),
        QStringLiteral("Архив M2MTools (*.m2m);;Все файлы (*)"));
    if (path.isEmpty()) return;

    bool accepted = false;
    const QString password = QInputDialog::getText(
        this, QStringLiteral("Пароль BazaVar"),
        QStringLiteral("Введите пароль архива:"),
        QLineEdit::Password, QString(), &accepted);
    if (!accepted) return;
    if (!ResultMessenger::askConfirm(
            this, QStringLiteral("Открыть BazaVar?"),
            QStringLiteral(
                "Текущая база вариантов будет заменена содержимым архива."))) {
        return;
    }

    Result result = m_variantService->importArchive(path, password);
    Logger::log(result);
    if (!result.ok()) {
        ResultMessenger::show(this, result);
        showInlineMessage(result.message());
        return;
    }

    Result allResult = m_variantService->findAllVariants();
    if (allResult.ok() && allResult.hasPayload() &&
        !allResult.payloadAs<QList<VariantRow>>().isEmpty()) {
        const int firstNumber =
            allResult.payloadAs<QList<VariantRow>>().first().number;
        Result loaded = m_variantService->loadVariant(firstNumber);
        if (loaded.ok() && loaded.hasPayload())
            m_data = loaded.payloadAs<AutomatonData>();
    } else {
        m_data = AutomatonData();
        m_data.setVariantNumber(
            m_variantService->firstAvailableVariantNumber());
    }
    m_numberWasEdited = false;
    refreshData();
    refreshTable();
    syncReactionFromData();
    clearInlineMessage();
    ui->statusbar->showMessage(
        QStringLiteral("Архив BazaVar открыт."), 4000);
}

void MainWindow::refreshTable() {
    m_updatingTable = true;
    QTableWidget* tableWidget = ui->transitionTableWidget;
    tableWidget->clear();

    const int stateCount = m_data.getStateNames().size();
    const int inputCount = m_data.getInputSignalNames().size();
    const int rowOffset =
        m_data.getType() == VariantType::MooreToMealy ? 2 : 1;
    tableWidget->setColumnCount(stateCount + 1);
    tableWidget->setRowCount(inputCount + rowOffset);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->verticalHeader()->setVisible(false);

    for (int stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
        const QString state = m_data.getStateNames()[stateIndex];
        if (m_data.getType() == VariantType::MooreToMealy) {
            auto* outputItem = new QTableWidgetItem(displayCell(
                m_data.getMooreOutputs()
                    .value(state).join(QStringLiteral(", "))));
            tableWidget->setItem(0, stateIndex + 1, outputItem);
            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(1, stateIndex + 1, stateItem);
        } else {
            auto* stateItem = new QTableWidgetItem(state);
            stateItem->setFlags(stateItem->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(0, stateIndex + 1, stateItem);
        }
    }

    if (m_data.getType() == VariantType::MooreToMealy) {
        auto* outputCorner =
            new QTableWidgetItem(QStringLiteral("Выход Y"));
        outputCorner->setFlags(
            outputCorner->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(0, 0, outputCorner);
        auto* stateCorner =
            new QTableWidgetItem(QStringLiteral("Состояние"));
        stateCorner->setFlags(
            stateCorner->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(1, 0, stateCorner);
    } else {
        auto* corner = new QTableWidgetItem(QStringLiteral("Состояние"));
        corner->setFlags(corner->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(0, 0, corner);
    }

    for (int inputIndex = 0; inputIndex < inputCount; ++inputIndex) {
        auto* inputItem =
            new QTableWidgetItem(m_data.getInputSignalNames()[inputIndex]);
        inputItem->setFlags(inputItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(inputIndex + rowOffset, 0, inputItem);
    }

    const auto& transitions = m_data.getTransitionTable();
    for (int inputIndex = 0; inputIndex < inputCount; ++inputIndex) {
        for (int stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
            const CellData& cell = transitions[inputIndex][stateIndex];
            QString text;
            if (m_data.getType() == VariantType::MooreToMealy) {
                text = displayCell(cell.nextState);
            } else {
                text = QStringLiteral("%1 / %2").arg(
                    displayCell(cell.nextState),
                    displayCell(
                        cell.outputSignals.join(QStringLiteral(", "))));
            }
            tableWidget->setItem(
                inputIndex + rowOffset, stateIndex + 1,
                new QTableWidgetItem(text));
        }
    }
    m_updatingTable = false;
}

void MainWindow::refreshData() {
    m_updatingUi = true;
    const int typeIndex =
        m_data.getType() == VariantType::MealyToMoore ? 0 : 1;
    ui->variantTypeComboBox->setCurrentIndex(typeIndex);
    ui->numberVariantSpinBox->setValue(m_data.getVariantNumber());
    ui->stateNamesLineEdit->setText(
        m_data.getStateNames().join(QStringLiteral(", ")));
    ui->InputSignalNamesLineEdit->setText(
        m_data.getInputSignalNames().join(QStringLiteral(", ")));
    ui->OutputSignalNamesLineEdit->setText(
        m_data.getOutputSignalNames().join(QStringLiteral(", ")));
    ui->transitionTableWidget->setItemDelegate(typeIndex == 0
        ? static_cast<QAbstractItemDelegate*>(
              new MealyDelegate(&m_data, this))
        : static_cast<QAbstractItemDelegate*>(
              new MooreDelegate(&m_data, this)));
    refreshInitialStates();
    m_updatingUi = false;
    refreshImagePreview();
    updateSummary();
}

void MainWindow::refreshInitialStates() {
    const bool wasUpdating = m_updatingUi;
    m_updatingUi = true;
    const QString selected = m_data.getInitialState();
    m_initialStateComboBox->clear();
    m_initialStateComboBox->addItems(m_data.getStateNames());
    const int index = m_initialStateComboBox->findText(selected);
    if (index >= 0) m_initialStateComboBox->setCurrentIndex(index);
    m_initialStateComboBox->setEnabled(
        !m_data.getStateNames().isEmpty());
    m_updatingUi = wasUpdating;
}

void MainWindow::refreshImagePreview() {
    m_imagePanel->setImage(
        m_data.getDiagramImage(), m_data.getDiagramFileName());
}

void MainWindow::updateSummary() {
    m_imagePanel->setCounts(
        m_data.getStateNames().size(),
        m_data.getInputSignalNames().size(),
        m_data.getOutputSignalNames().size());
}

void MainWindow::syncReactionFromData() {
    const QVector<ReactionStep> steps =
        ReactionSynchronizer::buildReaction(m_data);
    m_updatingReaction = true;
    m_reactionTable->clearContents();
    m_reactionTable->setColumnCount(steps.size());
    for (int column = 0; column < steps.size(); ++column) {
        m_reactionTable->setHorizontalHeaderItem(
            column,
            new QTableWidgetItem(QStringLiteral("t%1").arg(column + 1)));
        const QStringList values = {
            steps[column].currentState,
            steps[column].inputSignal,
            steps[column].nextState,
            steps[column].outputSignals,
        };
        for (int row = 0; row < values.size(); ++row)
            m_reactionTable->setItem(
                row, column, new QTableWidgetItem(values[row]));
    }
    m_updatingReaction = false;
}

QVector<ReactionStep> MainWindow::reactionSteps() const {
    QVector<ReactionStep> steps;
    for (int column = 0;
         column < m_reactionTable->columnCount(); ++column) {
        const auto textAt = [this, column](int row) {
            const QTableWidgetItem* item =
                m_reactionTable->item(row, column);
            return item ? item->text() : QString();
        };
        steps.append({
            textAt(0), textAt(1), textAt(2), textAt(3)
        });
    }
    return steps;
}

ReactionSyncReport MainWindow::syncDataFromReaction() {
    const ReactionSyncReport report =
        ReactionSynchronizer::buildAutomaton(reactionSteps(), m_data);
    applyReactionIssues(report.issues);
    return report;
}

void MainWindow::applyReactionIssues(
    const QVector<ReactionIssue>& issues) {
    const QSignalBlocker blocker(m_reactionTable);
    for (int column = 0;
         column < m_reactionTable->columnCount(); ++column) {
        for (int row = 0;
             row < m_reactionTable->rowCount(); ++row) {
            if (auto* item =
                    m_reactionTable->item(row, column)) {
                item->setBackground(QBrush());
                item->setForeground(QBrush());
                item->setToolTip(QString());
            }
        }
    }
    QString firstError;
    QString firstIncomplete;
    for (const ReactionIssue& issue : issues) {
        const int firstRow = issue.row >= 0 ? issue.row : 0;
        const int lastRow = issue.row >= 0
            ? issue.row : m_reactionTable->rowCount() - 1;
        for (int row = firstRow; row <= lastRow; ++row) {
            if (auto* item =
                    m_reactionTable->item(row, issue.column)) {
                item->setBackground(
                    issue.severity == ReactionIssueSeverity::Error
                        ? kErrorBackground : kIncompleteBackground);
                item->setForeground(
                    issue.severity == ReactionIssueSeverity::Error
                        ? kErrorForeground : kIncompleteForeground);
                item->setToolTip(issue.message);
            }
        }
        if (issue.severity == ReactionIssueSeverity::Error
            && firstError.isEmpty())
            firstError = issue.message;
        if (issue.severity == ReactionIssueSeverity::Incomplete
            && firstIncomplete.isEmpty())
            firstIncomplete = issue.message;
    }
    if (!firstError.isEmpty())
        showInlineMessage(firstError);
    else if (!firstIncomplete.isEmpty())
        showInlineMessage(firstIncomplete, false);
    else
        clearInlineMessage();
}

bool MainWindow::validateBeforeSave() {
    if (m_editModeComboBox->currentIndex() == 1) {
        const ReactionSyncReport report = syncDataFromReaction();
        if (report.hasErrors()) return false;
        m_data = report.data;
        refreshData();
        refreshTable();
    }
    if (!m_data.hasDiagramImage()) {
        m_imagePanel->setInvalid(true);
        showInlineMessage(
            QStringLiteral("Добавьте изображение задания для варианта."));
        return false;
    }
    if (!validateTransitionCompleteness()) return false;

    const Result result = m_data.validate();
    Logger::log(result);
    if (!result.ok()) {
        showInlineMessage(
            translateResult(result) + (result.offender().isEmpty()
                ? QString() : QStringLiteral(": ") + result.offender()));
        ResultMessenger::show(this, result);
        return false;
    }
    return true;
}

bool MainWindow::validateTransitionCompleteness() {
    const int rowOffset =
        m_data.getType() == VariantType::MooreToMealy ? 2 : 1;
    const auto& table = m_data.getTransitionTable();
    for (int input = 0; input < table.size(); ++input) {
        for (int state = 0;
             state < table[input].size(); ++state) {
            if (!table[input][state].nextState.isEmpty()) continue;
            if (auto* item = ui->transitionTableWidget->item(
                    input + rowOffset, state + 1)) {
                item->setBackground(kErrorBackground);
                item->setForeground(kErrorForeground);
                item->setToolTip(
                    QStringLiteral("Укажите следующее состояние"));
            }
            m_editorStack->setCurrentIndex(0);
            m_editModeComboBox->setCurrentIndex(0);
            showInlineMessage(QStringLiteral(
                "Таблица переходов заполнена не полностью: "
                "укажите следующее состояние для каждой пары A × X."));
            return false;
        }
    }
    return true;
}

bool MainWindow::loadImageFromPath(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        showInlineMessage(
            QStringLiteral("Не удалось открыть файл изображения."));
        return false;
    }
    if (file.size() <= 0 || file.size() > kMaximumImageSize) {
        showInlineMessage(
            QStringLiteral(
                "Размер изображения должен быть не больше 10 МБ."));
        return false;
    }
    const QByteArray data = file.readAll();
    QImage image;
    if (!image.loadFromData(data)) {
        showInlineMessage(QStringLiteral(
            "Выбранный файл не является поддерживаемым изображением."));
        return false;
    }
    m_data.setDiagramImage(data, QFileInfo(path).fileName());
    refreshImagePreview();
    clearInlineMessage();
    return true;
}

void MainWindow::showInlineMessage(
    const QString& text, bool error) {
    m_validationLabel->setText(text);
    m_validationLabel->setStyleSheet(error
        ? QStringLiteral(
              "background: #fff1f2; color: #9f1239; "
              "border: 1px solid #fecdd3;")
        : QStringLiteral(
              "background: #fffbeb; color: #92400e; "
              "border: 1px solid #fde68a;"));
    m_validationLabel->show();
    ui->statusbar->showMessage(text);
}

void MainWindow::clearInlineMessage() {
    m_validationLabel->clear();
    m_validationLabel->hide();
}
