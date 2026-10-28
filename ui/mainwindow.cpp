#include "ui/mainwindow.h"
#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/appicons.h"
#include "ui/logwindow.h"
#include "ui/translateresult.h"
#include "ui/variantslist.h"
#include "ui/widgets/reactiontable.h"
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>

using Icon = AppIcons::Kind;
MainWindow::MainWindow(VariantService *service, QWidget *parent)
    : MainWindow(service, nullptr, parent) {}
MainWindow::MainWindow(VariantService *service, TranslationManager *translator, QWidget *parent)
    : QMainWindow(parent), m_service(service), m_translator(translator) {
    setWindowTitle(QStringLiteral("M2MTools Lite[*]"));
    resize(1060, 600);
    setMinimumSize(760, 560);
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(16);
    auto *header = new QHBoxLayout;
    m_number = new QSpinBox;
    m_number->setRange(1, 999999);
    m_number->setObjectName("variantNumber");
    m_type = new QComboBox;
    m_type->setObjectName("conversionType");
    m_type->addItems({"", ""});
    auto *numberLabel = new QLabel;
    auto *typeLabel = new QLabel;
    header->addWidget(numberLabel);
    header->addWidget(m_number);
    header->addSpacing(20);
    header->addWidget(typeLabel);
    header->addWidget(m_type, 1);
    layout->addLayout(header);
    auto *counts = new QHBoxLayout;
    counts->addWidget(createCounts(true));
    counts->addWidget(createCounts(false));
    layout->addLayout(counts);
    auto *reactionBox = new QGroupBox;
    auto *reactionLayout = new QVBoxLayout(reactionBox);
    m_reaction = new ReactionTable;
    reactionLayout->addWidget(m_reaction);
    auto *hint = new QLabel;
    hint->setWordWrap(true);
    hint->setObjectName("reactionHint");
    hint->setMinimumHeight(hint->fontMetrics().lineSpacing() * 2 + 4);
    reactionLayout->addWidget(hint);
    auto *controls = new QHBoxLayout;
    auto *add = new QPushButton(AppIcons::icon(Icon::Add), QString());
    add->setObjectName("addReactionStep");
    auto *remove = new QPushButton(AppIcons::icon(Icon::Remove), QString());
    remove->setObjectName("removeReactionStep");
    controls->addWidget(add);
    controls->addWidget(remove);
    controls->addStretch();
    reactionLayout->addLayout(controls);
    layout->addWidget(reactionBox);
    m_feedback = new QLabel;
    m_feedback->setWordWrap(true);
    m_feedback->setObjectName("validationMessage");
    layout->addWidget(m_feedback);
    layout->addStretch();
    auto *bottom = new QHBoxLayout;
    auto *fresh = new QPushButton(AppIcons::icon(Icon::New), QString());
    auto *open = new QPushButton(AppIcons::icon(Icon::Open), QString());
    auto *store = new QPushButton(AppIcons::icon(Icon::Save), QString());
    store->setObjectName("saveVariantButton");
    bottom->addWidget(fresh);
    bottom->addWidget(open);
    bottom->addStretch();
    bottom->addWidget(store);
    layout->addLayout(bottom);
    m_translations.append([=] {
        numberLabel->setText(tr("Variant number"));
        typeLabel->setText(tr("Conversion"));
        m_type->setItemText(0, variantTypeDisplayName(VariantType::MealyToMoore));
        m_type->setItemText(1, variantTypeDisplayName(VariantType::MooreToMealy));
        reactionBox->setTitle(tr("Automaton reaction"));
        hint->setText(tr("One column is one step. Input: x0–x9. Outputs: y0–y9 separated by "
                         "commas; empty or '-' means no output. Order is preserved."));
        add->setText(tr("Insert step after selection"));
        remove->setText(tr("Delete selected step"));
        fresh->setText(tr("New variant"));
        open->setText(tr("Variants list"));
        store->setText(tr("Save variant"));
        m_reaction->retranslate();
    });
    connect(add, &QPushButton::clicked, m_reaction, &ReactionTable::insertStep);
    connect(remove, &QPushButton::clicked, m_reaction, &ReactionTable::removeStep);
    connect(store, &QPushButton::clicked, this, [this] { save(); });
    connect(fresh, &QPushButton::clicked, this, &MainWindow::newVariant);
    connect(open, &QPushButton::clicked, this, &MainWindow::openVariant);
    auto changed = [this] {
        if (!m_loading) {
            m_dirty = true;
            setWindowModified(true);
            m_feedback->clear();
        }
    };
    connect(m_number, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    connect(m_type, qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    for (auto *spin : m_mealy)
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    for (auto *spin : m_moore)
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, changed);
    connect(m_reaction, &QTableWidget::cellChanged, this, changed);
    connect(m_reaction->model(), &QAbstractItemModel::columnsInserted, this, changed);
    connect(m_reaction->model(), &QAbstractItemModel::columnsRemoved, this, changed);
    setupMenus();
    refreshTexts();
    ReactionVariant initial;
    initial.number = m_service->firstAvailableVariantNumber();
    loadData(initial);
}
QGroupBox *MainWindow::createCounts(bool mealy) {
    auto *group = new QGroupBox;
    auto *form = new QFormLayout(group);
    auto &values = mealy ? m_mealy : m_moore;
    for (int i = 0; i < 3; ++i) {
        values[i] = new QSpinBox;
        values[i]->setRange(1, i == 0 ? 999999 : 10);
        values[i]->setObjectName(
            QStringLiteral("%1%2Count")
                .arg(mealy ? "mealy" : "moore", QStringList{"States", "Inputs", "Outputs"}[i]));
        auto *label = new QLabel;
        form->addRow(label, values[i]);
        m_translations.append([this, label, i] {
            label->setText(i == 0   ? tr("States")
                           : i == 1 ? tr("Input signals")
                                    : tr("Output signals"));
        });
    }
    m_translations.append([this, group, mealy] {
        group->setTitle(mealy ? tr("Mealy — counts") : tr("Moore — counts"));
    });
    return group;
}
void MainWindow::setupMenus() {
    AppIcons::configureMenuBar(menuBar());
    auto *file = menuBar()->addMenu(AppIcons::icon(Icon::File), QString());
    auto *settings = menuBar()->addMenu(AppIcons::icon(Icon::Settings), QString());
    auto *language = settings->addMenu(AppIcons::icon(Icon::Language), QString());
    auto action = [this](QMenu *menu, Icon icon, const char *text, const QKeySequence &key,
                         std::function<void()> callback) {
        auto *a = menu->addAction(AppIcons::icon(icon), QString());
        a->setIconVisibleInMenu(true);
        a->setShortcut(key);
        m_translations.append([this, a, text] { a->setText(tr(text)); });
        connect(a, &QAction::triggered, this, callback);
        return a;
    };
    action(file, Icon::New, QT_TR_NOOP("New variant"), QKeySequence::New, [this] { newVariant(); });
    action(file, Icon::Save, QT_TR_NOOP("Save variant"), QKeySequence::Save, [this] { save(); });
    action(file, Icon::Open, QT_TR_NOOP("Variants list"), QKeySequence::Open,
           [this] { openVariant(); });
    file->addSeparator();
    action(file, Icon::Import, QT_TR_NOOP("Import database"), {}, [this] { importArchive(); });
    action(file, Icon::Export, QT_TR_NOOP("Export database"), {}, [this] { exportArchive(); });
    file->addSeparator();
    action(file, Icon::Exit, QT_TR_NOOP("Exit"), QKeySequence::Quit, [this] { close(); });
    auto *langs = new QActionGroup(this);
    auto *en = action(language, Icon::English, QT_TR_NOOP("English"), {}, [this] {
        if (m_translator)
            showResult(m_translator->setLocale("en"));
    });
    auto *ru = action(language, Icon::Russian, QT_TR_NOOP("Russian"), {}, [this] {
        if (m_translator)
            showResult(m_translator->setLocale("ru_RU"));
    });
    for (auto *a : {en, ru}) {
        a->setCheckable(true);
        langs->addAction(a);
    }
    m_translations.append([this, file, settings, language, en, ru] {
        file->setTitle(tr("File"));
        settings->setTitle(tr("Settings"));
        language->setTitle(tr("Language"));
        const bool russian = m_translator && m_translator->currentLocale() == "ru_RU";
        ru->setChecked(russian);
        en->setChecked(!russian);
    });
    action(settings, Icon::Log, QT_TR_NOOP("Show logger"), {}, [this] {
        if (!m_log)
            m_log = new LogWindow(this);
        m_log->show();
        m_log->raise();
    });
}
void MainWindow::refreshTexts() {
    for (const auto &translate : m_translations)
        translate();
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
    m_dirty = false;
    setWindowModified(false);
    m_feedback->clear();
    m_loading = false;
}
void MainWindow::showResult(const Result &result) {
    Logger::log(result);
    m_feedback->setText(translateResult(result));
}
bool MainWindow::save() {
    m_reaction->setFocus(); // Finish editing before reading the sequence.
    const auto data = readData();
    const auto valid = data.validate();
    if (!valid.ok()) {
        showResult(valid);
        m_reaction->markError(valid.offender(), valid.message());
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
    m_dirty = false;
    setWindowModified(false);
    m_feedback->setText(tr("Variant #%1 saved").arg(data.number));
    return true;
}
bool MainWindow::confirmDiscard() {
    if (!m_dirty)
        return true;
    const auto answer = QMessageBox::question(
        this, tr("Unsaved changes"), tr("Save changes before continuing?"),
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
    if (!confirmDiscard())
        return;
    VariantsList dialog(m_service, this);
    connect(&dialog, &VariantsList::variantDeleted, this, [this](int number) {
        if (m_number->value() == number) {
            ReactionVariant data;
            data.number = m_service->firstAvailableVariantNumber();
            loadData(data);
        }
    });
    connect(&dialog, &VariantsList::variantsSwapped, this, [this](int a, int b) {
        m_loading = true;
        if (m_number->value() == a)
            m_number->setValue(b);
        else if (m_number->value() == b)
            m_number->setValue(a);
        m_loading = false;
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
    const auto path = QFileDialog::getOpenFileName(this, tr("Import database"), {},
                                                   tr("M2MTools Lite archives (*.m2mlite)"));
    if (path.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Import database"),
                              tr("Replace the current Lite database with the archive contents?")) !=
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
    auto path = QFileDialog::getSaveFileName(this, tr("Export database"), "BazaVar.m2mlite",
                                             tr("M2MTools Lite archives (*.m2mlite)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(".m2mlite", Qt::CaseInsensitive))
        path += ".m2mlite";
    showResult(m_service->exportArchive(path));
}
