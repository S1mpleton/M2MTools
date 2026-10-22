#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/automatondata.h"
#include "core/reactionsynchronizer.h"
#include "data/variantservice.h"
#include "infrastructure/translationmanager.h"
#include "ui/feedbackstyle.h"
#include "ui/logwindow.h"
#include "ui/presenters/tablecellerrorpresenter.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QComboBox;
class QLabel;
class QPushButton;
class QShortcut;
class QStackedWidget;
class QTableWidget;
QT_END_NAMESPACE

class VariantImagePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(VariantService* service, QWidget *parent = nullptr);
    MainWindow(VariantService* service, TranslationManager* translator,
               QWidget* parent);
    ~MainWindow() override;

private slots:
    void onLanguageSelected(QAction* action);
    void onShowLogClicked();
    void onExitAction();

    void onVariantTypeChanged(int index);
    void onVariantNumberChanged(int number);
    void onInitialStateChanged(const QString& state);
    void onStateNamesChanged(const QString& text);
    void onInputNamesChanged(const QString& text);
    void onOutputNamesChanged(const QString& text);

    void onTableCellChanged(int row, int col);
    void onReactionCellChanged(int row, int col);
    void onEditModeChanged(int index);
    void onAddReactionStep();
    void onRemoveReactionStep();

    void onLoadVariantPushButton();
    void onSavePushButtonClicked();
    void onNewVariantClicked();
    void onRemoveImageClicked();
    void onExportArchiveClicked();
    void onImportArchiveClicked();

private:
    Ui::MainWindow *ui;
    AutomatonData m_data;

    bool m_updatingTable = false;
    bool m_updatingReaction = false;
    bool m_updatingUi = false;
    bool m_initialStateWasEdited = false;
    quint64 m_messageRevision = 0;

    LogWindow* m_logWindow = nullptr;
    VariantService* m_variantService = nullptr;
    IErrorPresenter* m_stateNamesPresenter = nullptr;
    IErrorPresenter* m_inputNamesPresenter = nullptr;
    IErrorPresenter* m_outputNamesPresenter = nullptr;
    TableCellErrorPresenter* m_cellPresenter = nullptr;
    TranslationManager* m_translator = nullptr;

    QComboBox* m_initialStateComboBox = nullptr;
    QComboBox* m_editModeComboBox = nullptr;
    QStackedWidget* m_editorStack = nullptr;
    QTableWidget* m_reactionTable = nullptr;
    VariantImagePanel* m_imagePanel = nullptr;
    QLabel* m_validationLabel = nullptr;
    QLabel* m_initialStateLabel = nullptr;
    QLabel* m_editModeLabel = nullptr;
    QLabel* m_editModeHintLabel = nullptr;
    QLabel* m_reactionHintLabel = nullptr;
    QPushButton* m_newVariantButton = nullptr;
    QPushButton* m_addReactionStepButton = nullptr;
    QPushButton* m_removeReactionStepButton = nullptr;

    void setupModernUi();
    void setupPresenters();
    void setupConnections();
    void setupLanguageGroup();
    void retranslateDynamicUi();

    void refreshTable();
    void refreshData();
    void refreshInitialStates();
    void refreshImagePreview();
    void updateSummary();

    void syncReactionFromData();
    ReactionSyncReport syncDataFromReaction();
    QVector<ReactionStep> reactionSteps() const;
    void applyReactionIssues(const QVector<ReactionIssue>& issues);
    void insertReactionStepAfter(int column);
    void renumberReactionSteps();

    bool validateBeforeSave();
    void presentAutomatonValidation(const Result& result);
    bool loadImageFromPath(const QString& path);
    void showInlineMessage(const QString& text,
                           UiFeedbackLevel level = UiFeedbackLevel::Error,
                           int timeoutMs = 0);
    void clearInlineMessage();
    void cancelFieldPulse(QWidget* field);
    void pulseField(QWidget* field, UiFeedbackLevel level);
    void highlightAutomaticDataChanges(const AutomatonData& previousData);

protected:
    void changeEvent(QEvent *event) override;
};

#endif // MAINWINDOW_H
