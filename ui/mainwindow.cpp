#include "ui/mainwindow.h"
#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/appicons.h"
#include "ui/logwindow.h"
#include "ui/translateresult.h"
#include "ui/variantslist.h"
#include "ui/widgets/reactiontable.h"
#include "ui/widgets/variantimagepanel.h"
#include "ui_mainwindow.h"
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QTimer>

using Icon = AppIcons::Kind;
MainWindow::MainWindow(VariantService *service, QWidget *parent)
    : MainWindow(service, nullptr, parent) {}
MainWindow::MainWindow(VariantService *service, TranslationManager *translator, QWidget *parent)
    : QMainWindow(parent), m_service(service), m_translator(translator) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    m_reaction = ui->reactionTable;
    m_number = ui->variantNumber;
    m_type = ui->conversionType;
    m_mealy = {ui->mealyStatesCount, ui->mealyInputsCount, ui->mealyOutputsCount};
    m_moore = {ui->mooreStatesCount, ui->mooreInputsCount, ui->mooreOutputsCount};
    m_feedback = ui->validationMessage;
    m_imagePanel = new VariantImagePanel(ui->diagramGroup, ui->diagramDropZone, ui->imageFileLabel,
                                         ui->loadImageButton, ui->removeImageButton);
    ui->addReactionStep->setIcon(AppIcons::icon(Icon::Add));
    ui->removeReactionStep->setIcon(AppIcons::icon(Icon::Remove));
    ui->newVariantButton->setIcon(AppIcons::icon(Icon::New));
    ui->openVariantButton->setIcon(AppIcons::icon(Icon::Open));
    ui->saveVariantButton->setIcon(AppIcons::icon(Icon::Save));
    connect(ui->addReactionStep, &QPushButton::clicked, m_reaction, &ReactionTable::insertStep);
    connect(ui->removeReactionStep, &QPushButton::clicked, m_reaction, &ReactionTable::removeStep);
    connect(ui->saveVariantButton, &QPushButton::clicked, this, [this] { save(); });
    connect(ui->newVariantButton, &QPushButton::clicked, this, &MainWindow::newVariant);
    connect(ui->openVariantButton, &QPushButton::clicked, this, &MainWindow::openVariant);
    auto changed = [this] {
        if (!m_loading) {
            updateModified();
            m_lastResult.reset();
            m_feedback->clear();
            ui->validationMessage->refreshHeight();
        }
    };
    connect(m_number, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    for (auto *spin : m_mealy)
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    for (auto *spin : m_moore)
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    connect(m_reaction, &QTableWidget::cellChanged, this, changed);
    // Columns receive their cells after the model emits its structural signals.
    auto columnsChanged = [this, changed] {
        if (!m_loading)
            QTimer::singleShot(0, this, changed);
    };
    connect(m_reaction->model(), &QAbstractItemModel::columnsInserted, this, columnsChanged);
    connect(m_reaction->model(), &QAbstractItemModel::columnsRemoved, this, columnsChanged);
    connect(m_imagePanel, &VariantImagePanel::imageChanged, this, changed);
    connect(m_imagePanel, &VariantImagePanel::loadFailed, this, &MainWindow::showResult);
    setupMenus();
    refreshTexts();
    ReactionVariant initial;
    initial.number = m_service->firstAvailableVariantNumber();
    loadData(initial);
}
MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (m_initialSizeApplied)
        return;
    m_initialSizeApplied = true;
    // Settle height-for-width labels at the layout's minimum width.
    for (int pass = 0; pass < 3; ++pass) {
        ui->centralwidget->layout()->activate();
        layout()->activate();
        resize(minimumSizeHint().expandedTo(minimumSize()));
        ui->reactionHint->refreshHeight();
        ui->validationMessage->refreshHeight();
    }
    layout()->activate();
    resize(minimumSize());
}

void MainWindow::setupMenus() {
    AppIcons::configureMenuBar(menuBar());
    const QList<QPair<QAction *, Icon>> icons = {{ui->menuFile->menuAction(), Icon::File},
                                                 {ui->menuSettings->menuAction(), Icon::Settings},
                                                 {ui->menuLanguage->menuAction(), Icon::Language},
                                                 {ui->actionNewVariant, Icon::New},
                                                 {ui->actionSaveVariant, Icon::Save},
                                                 {ui->actionOpenVariant, Icon::Open},
                                                 {ui->actionImportDatabase, Icon::Import},
                                                 {ui->actionExportDatabase, Icon::Export},
                                                 {ui->actionExit, Icon::Exit},
                                                 {ui->actionEnglish, Icon::English},
                                                 {ui->actionRussian, Icon::Russian},
                                                 {ui->actionShowLogger, Icon::Log}};
    for (const auto &entry : icons) {
        entry.first->setIcon(AppIcons::icon(entry.second));
        entry.first->setIconVisibleInMenu(true);
    }
    connect(ui->actionNewVariant, &QAction::triggered, this, &MainWindow::newVariant);
    connect(ui->actionSaveVariant, &QAction::triggered, this, [this] { save(); });
    connect(ui->actionOpenVariant, &QAction::triggered, this, &MainWindow::openVariant);
    connect(ui->actionImportDatabase, &QAction::triggered, this, &MainWindow::importArchive);
    connect(ui->actionExportDatabase, &QAction::triggered, this, &MainWindow::exportArchive);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    auto *languages = new QActionGroup(this);
    languages->addAction(ui->actionEnglish);
    languages->addAction(ui->actionRussian);
    connect(ui->actionEnglish, &QAction::triggered, this, [this] {
        if (m_translator)
            showResult(m_translator->setLocale("en"));
    });
    connect(ui->actionRussian, &QAction::triggered, this, [this] {
        if (m_translator)
            showResult(m_translator->setLocale("ru_RU"));
    });
    connect(ui->actionShowLogger, &QAction::triggered, this, [this] {
        if (!m_log)
            m_log = new LogWindow(this);
        m_log->show();
        m_log->raise();
    });
}
void MainWindow::refreshTexts() {
    if (!ui)
        return;
    ui->retranslateUi(this);
    m_reaction->retranslate();
    ui->reactionHint->refreshHeight();
    if (m_lastResult) {
        m_feedback->setText(translateResult(*m_lastResult));
        ui->validationMessage->refreshHeight();
    }
    const bool russian = m_translator && m_translator->currentLocale() == "ru_RU";
    ui->actionRussian->setChecked(russian);
    ui->actionEnglish->setChecked(!russian);
}
void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        refreshTexts();
}
ReactionVariant MainWindow::readData() const {
    ReactionVariant data;
    data.number = m_number->value();
    data.type = m_type->currentIndex() == 0 ? VariantType::MealyToMoore : VariantType::MooreToMealy;
    data.mealy = {m_mealy[0]->value(), m_mealy[1]->value(), m_mealy[2]->value()};
    data.moore = {m_moore[0]->value(), m_moore[1]->value(), m_moore[2]->value()};
    data.reaction = m_reaction->steps();
    data.image = m_imagePanel->image();
    return data;
}
void MainWindow::loadData(const ReactionVariant &data) {
    m_loading = true;
    m_number->setValue(data.number);
    m_type->setCurrentIndex(data.type == VariantType::MealyToMoore ? 0 : 1);
    for (int i = 0; i < 3; ++i) {
        m_mealy[i]->setValue(
            std::array<int, 3>{data.mealy.states, data.mealy.inputs, data.mealy.outputs}[i]);
        m_moore[i]->setValue(
            std::array<int, 3>{data.moore.states, data.moore.inputs, data.moore.outputs}[i]);
    }
    m_reaction->setSteps(data.reaction);
    m_imagePanel->setImage(data.image);
    m_baseline = readData();
    m_lastResult.reset();
    setWindowModified(false);
    m_feedback->clear();
    ui->validationMessage->refreshHeight();
    m_loading = false;
}
void MainWindow::showResult(const Result &result) {
    m_lastResult = result;
    Logger::log(result);
    m_feedback->setText(translateResult(result));
    ui->validationMessage->refreshHeight();
}
bool MainWindow::save() {
    m_reaction->setFocus(); // Finish editing before reading the sequence.
    const auto data = readData();
    const auto valid = data.validateForSave();
    if (!valid.ok()) {
        showResult(valid);
        m_reaction->markError(valid.offender(), translateResult(valid));
        if (valid.code() == ErrorCode::VariantMissingImage)
            ui->loadImageButton->setFocus();
        return false;
    }
    const auto exists = m_service->isExist(data.number);
    if (!exists.ok()) {
        showResult(exists);
        return false;
    }
    if (exists.payloadAs<bool>() &&
        QMessageBox::question(this, tr("Replace variant?"),
                              tr("Variant #%1 already exists. Replace it?").arg(data.number)) !=
            QMessageBox::Yes)
        return false;
    const auto saved =
        exists.payloadAs<bool>() ? m_service->replaceVariant(data) : m_service->saveVariant(data);
    showResult(saved);
    if (!saved.ok())
        return false;
    m_baseline = data;
    setWindowModified(false);
    return true;
}
void MainWindow::updateModified() {
    setWindowModified(!(readData() == m_baseline));
}
bool MainWindow::confirmDiscard(QWidget *parent) {
    m_reaction->setFocus(); // Commit an active editor, including menu/shortcut actions.
    const auto current = readData();
    if (current.reaction.isEmpty() || current == m_baseline)
        return true;
    const auto answer = QMessageBox::question(
        parent ? parent : this, tr("Unsaved changes"), tr("Save changes before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    return answer == QMessageBox::Discard || (answer == QMessageBox::Save && save());
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (confirmDiscard())
        event->accept();
    else
        event->ignore();
}
void MainWindow::newVariant() {
    if (!confirmDiscard())
        return;
    ReactionVariant data;
    data.number = m_service->firstAvailableVariantNumber();
    loadData(data);
}
void MainWindow::openVariant() {
    m_reaction->setFocus();
    VariantsList dialog(m_service, this);
    dialog.setBeforeLoad([this, &dialog] { return confirmDiscard(&dialog); });
    connect(&dialog, &VariantsList::variantDeleted, this, [this](int number) {
        if (m_number->value() == number) {
            ReactionVariant data;
            data.number = m_service->firstAvailableVariantNumber();
            if (readData() == m_baseline)
                loadData(data);
            else {
                // Deleting a stored variant must not erase its edited draft.
                m_baseline = data;
                updateModified();
            }
        }
    });
    connect(&dialog, &VariantsList::variantsSwapped, this, [this](int a, int b) {
        m_loading = true;
        if (m_number->value() == a)
            m_number->setValue(b);
        else if (m_number->value() == b)
            m_number->setValue(a);
        if (m_baseline.number == a)
            m_baseline.number = b;
        else if (m_baseline.number == b)
            m_baseline.number = a;
        m_loading = false;
        updateModified();
    });
    if (dialog.exec() != QDialog::Accepted || !dialog.hasSelection())
        return;
    const auto result = m_service->loadVariant(dialog.selectedVariantNumber());
    if (result.ok() && result.hasPayload())
        loadData(result.payloadAs<ReactionVariant>());
    else
        showResult(result);
}
void MainWindow::importArchive() {
    if (!confirmDiscard())
        return;
    const auto path =
        QFileDialog::getOpenFileName(this, tr("Import database"), {}, tr("M2M archives (*.m2m)"));
    if (path.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Import database"),
                              tr("Replace the current database with the archive contents?")) !=
        QMessageBox::Yes)
        return;
    const auto result = m_service->importArchive(path);
    if (result.ok()) {
        ReactionVariant data;
        data.number = m_service->firstAvailableVariantNumber();
        loadData(data);
    }
    showResult(result);
}
void MainWindow::exportArchive() {
    auto path = QFileDialog::getSaveFileName(this, tr("Export database"), "BazaVar.m2m",
                                             tr("M2M archives (*.m2m)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(".m2m", Qt::CaseInsensitive))
        path += ".m2m";
    showResult(m_service->exportArchive(path));
}
