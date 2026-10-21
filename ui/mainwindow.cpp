#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "core/namelistparser.h"
#include "infrastructure/logger.h"
#include "ui/delegates/mealydelegate.h"
#include "ui/delegates/mooredelegate.h"
#include "ui/delegates/reactiondelegate.h"
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
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kMaximumImageSize = 10 * 1024 * 1024;
constexpr int kMinimumTransientFeedbackMs = 5000;
constexpr int kAutomaticFieldPulseMs = 900;
QString displayCell(const QString& value) {
    return value.trimmed().isEmpty() ? QStringLiteral("—") : value;
}
}

MainWindow::MainWindow(VariantService* service, QWidget *parent)
    : MainWindow(service, nullptr, parent)
{
}

MainWindow::MainWindow(VariantService* service,
                       TranslationManager* translator,
                       QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_variantService(service)
    , m_translator(translator ? translator : new TranslationManager(this))
{
    ui->setupUi(this);
    if (m_translator->currentLocale().isEmpty())
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
    setupPresenters();
    setupConnections();

    m_data.setVariantNumber(m_variantService->firstAvailableVariantNumber());
    refreshData();
    refreshTable();
    syncReactionFromData();
    Logger::info(QStringLiteral("Main window initialized; reaction editor is active"),
                 ResultCategory::Ui);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupModernUi() {
    resize(1100, 720);
    setMinimumSize(900, 620);
    ui->statusbar->clearMessage();
    ui->statusbar->hide();

    // Use the platform palette and widget style. A partial custom theme can
    // easily combine a dark system foreground with a forced light background.
    setStyleSheet(QString());
    ui->verticalLayout->setContentsMargins(8, 6, 8, 6);
    ui->verticalLayout->setSpacing(6);

    ui->groupBox->setMinimumHeight(58);
    ui->groupBox->setMaximumHeight(72);
    ui->numberVariantSpinBox->setMaximum(999999);

    m_newVariantButton = new QPushButton(ui->groupBox);
    m_newVariantButton->setObjectName(QStringLiteral("newVariantButton"));
    ui->gridLayout_2->removeWidget(ui->label_2);
    ui->gridLayout_2->removeWidget(ui->variantTypeComboBox);
    ui->gridLayout_2->addWidget(ui->label_2, 0, 2);
    ui->gridLayout_2->addWidget(ui->variantTypeComboBox, 0, 3);
    ui->gridLayout_2->addWidget(m_newVariantButton, 0, 4);
    ui->gridLayout_2->setColumnStretch(3, 1);

    m_initialStateLabel = new QLabel(ui->groupBox_2);
    m_initialStateComboBox = new QComboBox(ui->groupBox_2);
    m_initialStateComboBox->setObjectName(QStringLiteral("initialStateComboBox"));
    ui->gridLayout_3->addWidget(m_initialStateLabel, 3, 0);
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
    m_editModeLabel = new QLabel(ui->groupBox_3);
    m_editModeComboBox = new QComboBox(ui->groupBox_3);
    m_editModeComboBox->setObjectName(QStringLiteral("editModeComboBox"));
    m_editModeComboBox->addItems({QString(), QString()});
    m_editModeHintLabel = new QLabel(ui->groupBox_3);
    modeLayout->addWidget(m_editModeLabel);
    modeLayout->addWidget(m_editModeComboBox);
    modeLayout->addStretch();
    modeLayout->addWidget(m_editModeHintLabel);
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
    m_reactionHintLabel = new QLabel(reactionPage);
    m_reactionHintLabel->setWordWrap(true);
    reactionLayout->addWidget(m_reactionHintLabel);
    m_reactionTable = new QTableWidget(4, 1, reactionPage);
    m_reactionTable->setObjectName(QStringLiteral("reactionTableWidget"));
    m_reactionTable->setHorizontalHeaderLabels({QStringLiteral("t1")});
    m_reactionTable->horizontalHeader()->setDefaultSectionSize(150);
    m_reactionTable->horizontalHeader()->setMinimumSectionSize(110);
    m_reactionTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reactionTable->setAlternatingRowColors(true);
    m_reactionTable->setMinimumHeight(145);
    m_reactionTable->setItemDelegate(new ReactionDelegate(m_reactionTable));
    reactionLayout->addWidget(m_reactionTable);
    auto* reactionButtons = new QHBoxLayout;
    m_addReactionStepButton = new QPushButton(reactionPage);
    m_addReactionStepButton->setObjectName(QStringLiteral("addReactionStepButton"));
    m_removeReactionStepButton = new QPushButton(reactionPage);
    m_removeReactionStepButton->setObjectName(QStringLiteral("removeReactionStepButton"));
    reactionButtons->addWidget(m_addReactionStepButton);
    reactionButtons->addWidget(m_removeReactionStepButton);
    reactionButtons->addStretch();
    reactionLayout->addLayout(reactionButtons);
    m_editorStack->addWidget(reactionPage);
    ui->gridLayout_4->addWidget(m_editorStack, 1, 0);
    ui->groupBox_3->setMinimumHeight(250);
    ui->verticalLayout->setStretch(2, 1);

    m_validationLabel = new QLabel(centralWidget());
    m_validationLabel->setObjectName(QStringLiteral("validationMessage"));
    m_validationLabel->setWordWrap(true);
    m_validationLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_validationLabel->setMinimumHeight(40);
    m_validationLabel->setMaximumHeight(72);
    m_validationLabel->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_validationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_validationLabel->hide();
    ui->verticalLayout->insertWidget(3, m_validationLabel);

    ui->savePushButton->setObjectName(QStringLiteral("primaryButton"));
    ui->savePushButton->setDefault(true);
    m_importArchiveButton = new QPushButton(this);
    m_importArchiveButton->setObjectName(
        QStringLiteral("importArchiveButton"));
    m_exportArchiveButton = new QPushButton(this);
    m_exportArchiveButton->setObjectName(
        QStringLiteral("exportArchiveButton"));
    ui->horizontalLayout->addWidget(m_importArchiveButton);
    ui->horizontalLayout->addWidget(m_exportArchiveButton);

    m_editModeComboBox->setCurrentIndex(1);
    m_editorStack->setCurrentIndex(1);
    retranslateDynamicUi();
}

void MainWindow::retranslateDynamicUi() {
    setWindowTitle(tr("M2MTools — Equivalent Automata Editor"));
    ui->groupBox->setTitle(tr("Variant"));
    ui->groupBox_2->setTitle(tr("Automaton Symbols"));
    ui->groupBox_3->setTitle(tr("Automaton Definition"));
    ui->label->setText(tr("Variant number"));
    ui->label_2->setText(tr("Conversion"));
    ui->label_3->setText(tr("States A"));
    ui->label_4->setText(tr("Input signals X"));
    ui->label_5->setText(tr("Output signals Y"));
    ui->variantTypeComboBox->setItemText(0, tr("Mealy → Moore"));
    ui->variantTypeComboBox->setItemText(1, tr("Moore → Mealy"));
    ui->stateNamesLineEdit->setPlaceholderText(tr("For example: a0, a1, a2"));
    ui->InputSignalNamesLineEdit->setPlaceholderText(tr("For example: x1, x2"));
    ui->OutputSignalNamesLineEdit->setPlaceholderText(tr("For example: y1, y2"));
    ui->menuMain->setTitle(tr("File"));
    ui->menuSettings->setTitle(tr("Settings"));
    ui->actionSaveVariant->setText(tr("Save variant"));
    ui->actionLoadVariant->setText(tr("Open variants"));
    ui->actionShowLogger->setText(tr("Event log"));
    ui->actionExit->setText(tr("Exit"));
    m_newVariantButton->setText(tr("New variant"));
    m_initialStateLabel->setText(tr("Initial state"));
    m_editModeLabel->setText(tr("Edit using:"));
    m_editModeComboBox->setItemText(0, tr("Transition table"));
    m_editModeComboBox->setItemText(1, tr("Automaton reaction"));
    m_editModeHintLabel->setText(tr("Both forms update automatically"));
    m_reactionHintLabel->setText(tr(
        "One column is one tact: A + X → A′ / Y. Names are added automatically. "
        "Press Enter to insert a tact after the selected column."));
    m_reactionTable->setVerticalHeaderLabels({
        tr("State"), tr("Input X"), tr("Next state"), tr("Output Y")});
    m_addReactionStepButton->setText(tr("+ Add tact"));
    m_removeReactionStepButton->setText(tr("Remove tact"));
    ui->savePushButton->setText(tr("Save variant"));
    ui->loadVariantPushButton->setText(tr("Open variants"));
    m_importArchiveButton->setText(tr("Import database"));
    m_exportArchiveButton->setText(tr("Export database"));
}

void MainWindow::setupPresenters() {
    m_stateNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->stateNamesLineEdit, nullptr, this);
    m_inputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->InputSignalNamesLineEdit, nullptr, this);
    m_outputNamesPresenter = ErrorPresenterFactory::forLineEdit(
        ui->OutputSignalNamesLineEdit, nullptr, this);
    m_cellPresenter = new TableCellErrorPresenter(
        ui->transitionTableWidget, 0, 0, nullptr, this);
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

    const auto addInsertShortcut = [this](const QKeySequence& key,
                                          const QString& objectName) {
        auto* shortcut = new QShortcut(key, m_reactionTable);
        shortcut->setObjectName(objectName);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated, this, [this] {
            const int selectedColumn = m_reactionTable->currentColumn();
            m_reactionTable->setFocus(Qt::ShortcutFocusReason);
            insertReactionStepAfter(selectedColumn);
        });
    };
    addInsertShortcut(QKeySequence(Qt::Key_Return),
                      QStringLiteral("insertReactionStepReturnShortcut"));
    addInsertShortcut(QKeySequence(Qt::Key_Enter),
                      QStringLiteral("insertReactionStepEnterShortcut"));

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

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        retranslateDynamicUi();
        refreshTable();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::onLanguageSelected(QAction* action) {
    const QString locale = action == ui->actionSetLangEn
        ? QStringLiteral("en") : QStringLiteral("ru_RU");
    Result result = m_translator->setLocale(locale);
    Logger::log(result);
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
    Logger::info(QStringLiteral("Automaton conversion changed to %1")
                     .arg(variantTypeToString(m_data.getType())),
                 ResultCategory::Ui);
}

void MainWindow::onVariantNumberChanged(int number) {
    if (!m_updatingUi) m_data.setVariantNumber(number);
}

void MainWindow::onInitialStateChanged(const QString& state) {
    if (m_updatingUi || state.isEmpty()) return;
    cancelFieldPulse(m_initialStateComboBox);
    m_initialStateComboBox->setToolTip(QString());
    const Result result = m_data.setInitialState(state);
    Logger::log(result);
    if (!result.ok()) {
        showInlineMessage(translateResult(result));
        return;
    }
    m_initialStateWasEdited = true;
    syncReactionFromData();
    clearInlineMessage();
}

void MainWindow::onStateNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    cancelFieldPulse(ui->stateNamesLineEdit);
    const QString previousInitialState = m_data.getInitialState();
    const Result result = m_data.setStateNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_stateNamesPresenter->show(result);
        showInlineMessage(translateResult(result));
        return;
    }
    m_stateNamesPresenter->clear(result);
    clearInlineMessage();
    refreshInitialStates();
    refreshTable();
    syncReactionFromData();
    updateSummary();
    if (previousInitialState != m_data.getInitialState())
        pulseField(m_initialStateComboBox,
                   m_data.getInitialState().isEmpty()
                       ? UiFeedbackLevel::Warning
                       : UiFeedbackLevel::Success);
}

void MainWindow::onInputNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    cancelFieldPulse(ui->InputSignalNamesLineEdit);
    const Result result = m_data.setInputSignalNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_inputNamesPresenter->show(result);
        showInlineMessage(translateResult(result));
        return;
    }
    m_inputNamesPresenter->clear(result);
    clearInlineMessage();
    refreshTable();
    syncReactionFromData();
    updateSummary();
}

void MainWindow::onOutputNamesChanged(const QString& text) {
    if (m_updatingUi) return;
    cancelFieldPulse(ui->OutputSignalNamesLineEdit);
    const Result result = m_data.setOutputSignalNames(NameListParser::parse(text));
    Logger::log(result);
    if (!result.ok()) {
        m_outputNamesPresenter->show(result);
        showInlineMessage(translateResult(result));
        return;
    }
    m_outputNamesPresenter->clear(result);
    clearInlineMessage();
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
        showInlineMessage(translateResult(result));
    }
}

void MainWindow::onEditModeChanged(int index) {
    m_editorStack->setCurrentIndex(index);
    if (index == 1) syncReactionFromData();
    Logger::info(index == 1
                     ? QStringLiteral("Editor mode changed to automaton reaction")
                     : QStringLiteral("Editor mode changed to transition table"),
                 ResultCategory::Ui);
}

void MainWindow::onReactionCellChanged(int row, int column) {
    if (m_updatingReaction || m_updatingUi) return;
    if (row == 0 && column == 0 && m_initialStateWasEdited) {
        QTableWidgetItem* firstState = m_reactionTable->item(0, 0);
        if (firstState && firstState->text().trimmed()
                != m_data.getInitialState()) {
            const QSignalBlocker blocker(m_reactionTable);
            firstState->setText(m_data.getInitialState());
            showInlineMessage(tr(
                "The first tact must start with the selected initial state '%1'.")
                .arg(m_data.getInitialState()));
            Logger::warning(QStringLiteral(
                "Rejected a reaction edit that changed the first tact away "
                "from initial state '%1'").arg(m_data.getInitialState()),
                ResultCategory::Validation);
            return;
        }
    }
    if (row == 2 && column + 1 < m_reactionTable->columnCount()) {
        QTableWidgetItem* nextCurrent = m_reactionTable->item(0, column + 1);
        const QTableWidgetItem* nextState = m_reactionTable->item(2, column);
        if (nextCurrent && nextCurrent->text().trimmed().isEmpty() && nextState) {
            const QSignalBlocker blocker(m_reactionTable);
            nextCurrent->setText(nextState->text().trimmed());
        }
    }
    const AutomatonData previousData = m_data;
    ReactionSyncReport report = syncDataFromReaction();
    if (report.hasErrors()) return;

    if (row == 0 && column == 0 && !m_initialStateWasEdited) {
        const QTableWidgetItem* firstState = m_reactionTable->item(0, 0);
        const QString state = firstState ? firstState->text().trimmed() : QString();
        if (!state.isEmpty() && report.data.getStateNames().contains(state))
            report.data.setInitialState(state);
    }

    m_data = report.data;
    refreshData();
    refreshTable();
    updateSummary();
    highlightAutomaticDataChanges(previousData);
}

void MainWindow::onAddReactionStep() {
    insertReactionStepAfter(m_reactionTable->currentColumn());
}

void MainWindow::insertReactionStepAfter(int column) {
    const int existingCount = m_reactionTable->columnCount();
    const int boundedColumn = qBound(-1, column, existingCount - 1);
    const int insertionColumn = boundedColumn + 1;
    m_updatingReaction = true;
    m_reactionTable->insertColumn(insertionColumn);
    for (int row = 0; row < 4; ++row)
        m_reactionTable->setItem(row, insertionColumn, new QTableWidgetItem);
    if (insertionColumn > 0) {
        const QTableWidgetItem* previousNext =
            m_reactionTable->item(2, insertionColumn - 1);
        if (previousNext)
            m_reactionTable->item(0, insertionColumn)->setText(previousNext->text());
    }
    renumberReactionSteps();
    m_reactionTable->setCurrentCell(0, insertionColumn);
    m_updatingReaction = false;
    Logger::info(QStringLiteral("Inserted reaction tact t%1 after column %2")
                     .arg(insertionColumn + 1)
                     .arg(boundedColumn + 1),
                 ResultCategory::Ui);
    syncDataFromReaction();
}

void MainWindow::renumberReactionSteps() {
    for (int i = 0; i < m_reactionTable->columnCount(); ++i) {
        m_reactionTable->setHorizontalHeaderItem(
            i, new QTableWidgetItem(QStringLiteral("t%1").arg(i + 1)));
    }
}

void MainWindow::onRemoveReactionStep() {
    int column = m_reactionTable->currentColumn();
    if (column < 0) column = m_reactionTable->columnCount() - 1;
    m_updatingReaction = true;
    if (m_reactionTable->columnCount() > 1) {
        m_reactionTable->removeColumn(column);
        renumberReactionSteps();
    } else {
        for (int row = 0; row < 4; ++row) {
            if (!m_reactionTable->item(row, 0))
                m_reactionTable->setItem(row, 0, new QTableWidgetItem);
            m_reactionTable->item(row, 0)->setText(QString());
        }
    }
    m_updatingReaction = false;
    Logger::info(QStringLiteral("Removed reaction tact at column %1")
                     .arg(column + 1),
                 ResultCategory::Ui);
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
        m_initialStateWasEdited = false;
        refreshData();
        refreshTable();
        syncReactionFromData();
        showInlineMessage(
            tr("The open variant was deleted. The fields have been cleared."),
            UiFeedbackLevel::Warning, 3500);
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
        showInlineMessage(translateResult(result));
        return;
    }
    m_data = result.payloadAs<AutomatonData>();
    m_initialStateWasEdited = true;
    refreshData();
    refreshTable();
    syncReactionFromData();
    showInlineMessage(tr("Variant #%1 loaded").arg(number),
                      UiFeedbackLevel::Success, 3500);
}

void MainWindow::onSavePushButtonClicked() {
    if (!validateBeforeSave()) return;

    Result existsResult = m_variantService->isExist(m_data.getVariantNumber());
    if (!existsResult.ok()) {
        showInlineMessage(translateResult(existsResult));
        return;
    }
    const bool exists = existsResult.payloadAs<bool>();

    Result result = exists
        ? m_variantService->replaceVariant(m_data)
        : m_variantService->saveVariant(m_data);
    Logger::log(result);
    if (!result.ok()) {
        showInlineMessage(translateResult(result));
        return;
    }
    showInlineMessage(tr("Variant #%1 saved").arg(m_data.getVariantNumber()),
                      UiFeedbackLevel::Success, 3500);
}

void MainWindow::onNewVariantClicked() {
    const int number = m_variantService->firstAvailableVariantNumber();
    m_data = AutomatonData();
    m_data.setVariantNumber(number);
    m_initialStateWasEdited = false;
    ui->numberVariantSpinBox->setStyleSheet(QString());
    refreshData();
    refreshTable();
    syncReactionFromData();
    showInlineMessage(tr("New variant #%1 is ready.").arg(number),
                      UiFeedbackLevel::Success, 2500);
    ui->stateNamesLineEdit->setFocus();
    Logger::info(QStringLiteral("Created a new in-memory variant #%1")
                     .arg(number), ResultCategory::Ui);
}

void MainWindow::onRemoveImageClicked() {
    m_data.clearDiagramImage();
    refreshImagePreview();
    showInlineMessage(
        tr("The image was removed. Load a new one before saving."),
        UiFeedbackLevel::Warning, 3500);
    Logger::info(QStringLiteral("Removed the current variant image"),
                 ResultCategory::Ui);
}

void MainWindow::onExportArchiveClicked() {
    Result allResult = m_variantService->findAllVariants();
    if (!allResult.ok()) {
        showInlineMessage(translateResult(allResult));
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
            this,
            tr("Variant #%1 is empty. Shift subsequent numbers before export?")
                .arg(gap),
            tr("Renumber variants?"))) {
        int newCurrentNumber = m_data.getVariantNumber();
        for (int i = 0; i < rows.size(); ++i) {
            if (rows[i].number == m_data.getVariantNumber()) {
                newCurrentNumber = i + 1;
                break;
            }
        }
        Result compactResult = m_variantService->compactVariantNumbers();
        if (!compactResult.ok()) {
            showInlineMessage(translateResult(compactResult));
            return;
        }
        m_data.setVariantNumber(newCurrentNumber);
        refreshData();
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Save BazaVar"),
        QStringLiteral("BazaVar.m2m"),
        tr("M2MTools archive (*.m2m);;All files (*)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".m2m");

    Result result = m_variantService->exportArchive(path);
    Logger::log(result);
    if (!result.ok()) {
        showInlineMessage(translateResult(result));
        return;
    }
    showInlineMessage(tr("The BazaVar file was saved."),
                      UiFeedbackLevel::Success, 4000);
}

void MainWindow::onImportArchiveClicked() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open BazaVar"), QString(),
        tr("M2MTools archive (*.m2m);;All files (*)"));
    if (path.isEmpty()) return;

    if (!ResultMessenger::askConfirm(
            this,
            tr("The current variant database will be replaced by the archive contents."),
            tr("Open BazaVar?"))) {
        return;
    }

    Result result = m_variantService->importArchive(path);
    Logger::log(result);
    if (!result.ok()) {
        showInlineMessage(translateResult(result));
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
    refreshData();
    refreshTable();
    syncReactionFromData();
    showInlineMessage(tr("The BazaVar file was opened."),
                      UiFeedbackLevel::Success, 4000);
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
            new QTableWidgetItem(tr("Output Y"));
        outputCorner->setFlags(
            outputCorner->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(0, 0, outputCorner);
        auto* stateCorner =
            new QTableWidgetItem(tr("State"));
        stateCorner->setFlags(
            stateCorner->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(1, 0, stateCorner);
    } else {
        auto* corner = new QTableWidgetItem(tr("State"));
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
    cancelFieldPulse(ui->stateNamesLineEdit);
    cancelFieldPulse(ui->InputSignalNamesLineEdit);
    cancelFieldPulse(ui->OutputSignalNamesLineEdit);
    cancelFieldPulse(m_initialStateComboBox);
    const Result refreshed = Result::success(
        QStringLiteral("Automaton fields refreshed from the current data"));
    m_stateNamesPresenter->clear(refreshed);
    m_inputNamesPresenter->clear(refreshed);
    m_outputNamesPresenter->clear(refreshed);
    m_initialStateComboBox->setStyleSheet(QString());
    m_initialStateComboBox->setToolTip(QString());

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
                        ? UiFeedbackStyle::background(UiFeedbackLevel::Error)
                        : UiFeedbackStyle::background(UiFeedbackLevel::Warning));
                item->setForeground(
                    issue.severity == ReactionIssueSeverity::Error
                        ? UiFeedbackStyle::foreground(UiFeedbackLevel::Error)
                        : UiFeedbackStyle::foreground(UiFeedbackLevel::Warning));
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
        showInlineMessage(firstIncomplete, UiFeedbackLevel::Warning);
    else
        clearInlineMessage();
}

bool MainWindow::validateBeforeSave() {
    const auto validateSymbols = [this](QLineEdit* field,
                                        IErrorPresenter* presenter,
                                        const Result& result) {
        Logger::log(result);
        if (result.ok()) return true;
        cancelFieldPulse(field);
        presenter->show(result);
        showInlineMessage(translateResult(result));
        return false;
    };
    if (!validateSymbols(
            ui->stateNamesLineEdit, m_stateNamesPresenter,
            m_data.setStateNames(
                NameListParser::parse(ui->stateNamesLineEdit->text())))) {
        return false;
    }
    if (!validateSymbols(
            ui->InputSignalNamesLineEdit, m_inputNamesPresenter,
            m_data.setInputSignalNames(
                NameListParser::parse(ui->InputSignalNamesLineEdit->text())))) {
        return false;
    }
    if (!validateSymbols(
            ui->OutputSignalNamesLineEdit, m_outputNamesPresenter,
            m_data.setOutputSignalNames(
                NameListParser::parse(ui->OutputSignalNamesLineEdit->text())))) {
        return false;
    }

    if (m_editModeComboBox->currentIndex() == 1) {
        const ReactionSyncReport report = syncDataFromReaction();
        if (report.hasErrors()) return false;
        m_data = report.data;
        refreshData();
        refreshTable();
    }
    const Result result = m_data.validate();
    Logger::log(result);
    if (!result.ok()) {
        presentAutomatonValidation(result);
        return false;
    }
    if (!m_data.hasDiagramImage()) {
        m_imagePanel->setInvalid(true);
        showInlineMessage(
            tr("Add the task image for this variant."));
        Logger::warning(QStringLiteral("Variant validation failed: task image is missing"),
                        ResultCategory::Validation);
        return false;
    }
    return true;
}

void MainWindow::presentAutomatonValidation(const Result& result) {
    const QString message = translateResult(result);
    showInlineMessage(message);
    const QSignalBlocker transitionTableBlocker(
        ui->transitionTableWidget);
    const QSignalBlocker reactionTableBlocker(m_reactionTable);

    const auto showFieldError = [this, &result](QWidget* field,
                                                IErrorPresenter* presenter) {
        cancelFieldPulse(field);
        presenter->show(result);
    };
    const auto markItem = [&message](QTableWidgetItem* item) {
        if (!item) return;
        item->setBackground(
            UiFeedbackStyle::background(UiFeedbackLevel::Error));
        item->setForeground(
            UiFeedbackStyle::foreground(UiFeedbackLevel::Error));
        item->setToolTip(message);
    };
    const auto markTransitionRow = [this, &markItem](int row) {
        if (row < 0) return;
        for (int column = 0;
             column < ui->transitionTableWidget->columnCount(); ++column) {
            markItem(ui->transitionTableWidget->item(row, column));
        }
    };
    const auto markTransitionColumn = [this, &markItem](int column) {
        if (column < 0) return;
        for (int row = 0;
             row < ui->transitionTableWidget->rowCount(); ++row) {
            markItem(ui->transitionTableWidget->item(row, column));
        }
    };
    const auto markReactionTacts = [this, &markItem](int row,
                                                      const QString& offender) {
        for (int column = 0;
             column < m_reactionTable->columnCount(); ++column) {
            const QTableWidgetItem* match = m_reactionTable->item(row, column);
            if (!match || match->text().trimmed() != offender) continue;
            for (int reactionRow = 0;
                 reactionRow < m_reactionTable->rowCount(); ++reactionRow) {
                markItem(m_reactionTable->item(reactionRow, column));
            }
        }
    };

    switch (result.code()) {
    case ErrorCode::ADEmptyStateList:
        showFieldError(ui->stateNamesLineEdit, m_stateNamesPresenter);
        return;
    case ErrorCode::ADEmptyInputList:
        showFieldError(ui->InputSignalNamesLineEdit, m_inputNamesPresenter);
        return;
    case ErrorCode::ADEmptyOutputList:
        showFieldError(ui->OutputSignalNamesLineEdit, m_outputNamesPresenter);
        return;
    case ErrorCode::ADInitialStateNotSet:
    case ErrorCode::ADInitialStateNotFound:
        cancelFieldPulse(m_initialStateComboBox);
        m_initialStateComboBox->setStyleSheet(
            UiFeedbackStyle::fieldStyle(UiFeedbackLevel::Error));
        m_initialStateComboBox->setToolTip(message);
        return;
    case ErrorCode::ADUnusedInput: {
        showFieldError(ui->InputSignalNamesLineEdit, m_inputNamesPresenter);
        const int inputIndex =
            m_data.getInputSignalNames().indexOf(result.offender());
        const int rowOffset =
            m_data.getType() == VariantType::MooreToMealy ? 2 : 1;
        markTransitionRow(inputIndex < 0 ? -1 : inputIndex + rowOffset);
        markReactionTacts(1, result.offender());
        return;
    }
    case ErrorCode::ADUnusedOutput:
        showFieldError(ui->OutputSignalNamesLineEdit, m_outputNamesPresenter);
        markReactionTacts(3, result.offender());
        return;
    case ErrorCode::ADStateWithoutIncomingTransition:
    case ErrorCode::ADStateWithoutOutgoingTransition:
        showFieldError(ui->stateNamesLineEdit, m_stateNamesPresenter);
        break;
    default:
        return;
    }

    const int stateIndex = m_data.getStateNames().indexOf(result.offender());
    markTransitionColumn(stateIndex < 0 ? -1 : stateIndex + 1);
    markReactionTacts(0, result.offender());
}

bool MainWindow::loadImageFromPath(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        showInlineMessage(tr("Could not open the image file."));
        Logger::error(QStringLiteral("Failed to open image '%1': %2")
                          .arg(path, file.errorString()),
                      ResultCategory::Ui);
        return false;
    }
    if (file.size() <= 0 || file.size() > kMaximumImageSize) {
        showInlineMessage(tr("The image size must not exceed 10 MB."));
        Logger::warning(QStringLiteral("Rejected image '%1' with size %2 bytes")
                            .arg(path).arg(file.size()),
                        ResultCategory::Validation);
        return false;
    }
    const QByteArray data = file.readAll();
    QImage image;
    if (!image.loadFromData(data)) {
        showInlineMessage(tr("The selected file is not a supported image."));
        Logger::warning(QStringLiteral("Rejected unsupported image file '%1'").arg(path),
                        ResultCategory::Validation);
        return false;
    }
    m_data.setDiagramImage(data, QFileInfo(path).fileName());
    refreshImagePreview();
    showInlineMessage(tr("The task image was loaded."),
                      UiFeedbackLevel::Success, 2500);
    Logger::info(QStringLiteral("Loaded image '%1' (%2 bytes)")
                     .arg(QFileInfo(path).fileName()).arg(data.size()),
                 ResultCategory::Ui);
    return true;
}

void MainWindow::showInlineMessage(const QString& text,
                                   UiFeedbackLevel level,
                                   int timeoutMs) {
    const quint64 revision = ++m_messageRevision;
    const int effectiveTimeout = timeoutMs > 0
        ? qMax(timeoutMs, kMinimumTransientFeedbackMs)
        : 0;
    m_validationLabel->setText(text);
    m_validationLabel->setStyleSheet(UiFeedbackStyle::bannerStyle(level));
    m_validationLabel->show();
    m_validationLabel->updateGeometry();
    ui->verticalLayout->invalidate();
    ui->verticalLayout->activate();
    m_validationLabel->raise();
    if (effectiveTimeout > 0) {
        QTimer::singleShot(effectiveTimeout, this, [this, revision] {
            if (revision == m_messageRevision)
                clearInlineMessage();
        });
    }
}

void MainWindow::clearInlineMessage() {
    ++m_messageRevision;
    m_validationLabel->clear();
    m_validationLabel->hide();
}

void MainWindow::cancelFieldPulse(QWidget* field) {
    if (!field) return;
    auto* animation = qobject_cast<QVariantAnimation*>(
        field->property("automaticFieldFeedbackAnimation")
            .value<QObject*>());
    if (!animation) return;

    animation->stop();
    field->setProperty("automaticFieldFeedbackAnimation", QVariant());
    field->setStyleSheet(QString());
    animation->deleteLater();
}

void MainWindow::pulseField(QWidget* field, UiFeedbackLevel level) {
    if (!field) return;
    cancelFieldPulse(field);

    auto* animation = new QVariantAnimation(field);
    animation->setObjectName(QStringLiteral("automaticFieldFeedback"));
    animation->setDuration(kAutomaticFieldPulseMs);
    animation->setStartValue(190);
    animation->setKeyValueAt(0.12, 255);
    animation->setKeyValueAt(0.85, 255);
    animation->setEndValue(190);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    field->setProperty("automaticFieldFeedbackAnimation",
                       QVariant::fromValue<QObject*>(animation));
    connect(animation, &QVariantAnimation::valueChanged, field,
            [field, level](const QVariant& value) {
                field->setStyleSheet(
                    UiFeedbackStyle::fieldStyle(level, value.toInt()));
            });
    connect(animation, &QVariantAnimation::finished, field,
            [field, animation] {
                if (field->property("automaticFieldFeedbackAnimation")
                        .value<QObject*>() == animation) {
                    field->setStyleSheet(QString());
                    field->setProperty("automaticFieldFeedbackAnimation", QVariant());
                }
                animation->deleteLater();
            });
    animation->start();
}

void MainWindow::highlightAutomaticDataChanges(
    const AutomatonData& previousData) {
    const auto pulseListChange = [this](QWidget* field,
                                        IErrorPresenter* presenter,
                                        const QStringList& before,
                                        const QStringList& after) {
        if (before == after) return;
        presenter->clear(Result::success(
            QStringLiteral("Automatically synchronized symbol list")));
        pulseField(field, after.size() >= before.size()
                              ? UiFeedbackLevel::Success
                              : UiFeedbackLevel::Warning);
    };

    pulseListChange(ui->stateNamesLineEdit,
                    m_stateNamesPresenter,
                    previousData.getStateNames(), m_data.getStateNames());
    pulseListChange(ui->InputSignalNamesLineEdit,
                    m_inputNamesPresenter,
                    previousData.getInputSignalNames(),
                    m_data.getInputSignalNames());
    pulseListChange(ui->OutputSignalNamesLineEdit,
                    m_outputNamesPresenter,
                    previousData.getOutputSignalNames(),
                    m_data.getOutputSignalNames());
    if (previousData.getInitialState() != m_data.getInitialState()) {
        m_initialStateComboBox->setToolTip(QString());
        pulseField(m_initialStateComboBox,
                   m_data.getInitialState().isEmpty()
                       ? UiFeedbackLevel::Warning
                       : UiFeedbackLevel::Success);
    }
}
