#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/automatondata.h"
#include "core/reactionsynchronizer.h"
#include "data/variantservice.h"
#include "infrastructure/translationmanager.h"
#include "ui/logwindow.h"
#include "ui/presenters/tablecellerrorpresenter.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;
QT_END_NAMESPACE

class VariantImagePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(VariantService* service, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onLanguageSelected(QAction* action);
    void onTypeCheckSelected(QAction* action);
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
    bool m_numberWasEdited = false;

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
    QPushButton* m_newVariantButton = nullptr;
    QPushButton* m_addReactionStepButton = nullptr;
    QPushButton* m_removeReactionStepButton = nullptr;
    QPushButton* m_exportArchiveButton = nullptr;
    QPushButton* m_importArchiveButton = nullptr;

    void setupModernUi();
    void setupPresenters();
    void setupConnections();
    void setupLanguageGroup();
    void setupTypeCheckGroup();

    void refreshTable();
    void refreshData();
    void refreshInitialStates();
    void refreshImagePreview();
    void updateSummary();

    void syncReactionFromData();
    ReactionSyncReport syncDataFromReaction();
    QVector<ReactionStep> reactionSteps() const;
    void applyReactionIssues(const QVector<ReactionIssue>& issues);

    bool validateBeforeSave();
    bool validateTransitionCompleteness();
    bool loadImageFromPath(const QString& path);
    void showInlineMessage(const QString& text, bool error = true);
    void clearInlineMessage();

protected:
    void changeEvent(QEvent *event) override;
};

#endif // MAINWINDOW_H
