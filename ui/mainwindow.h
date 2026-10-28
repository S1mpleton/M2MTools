#pragma once
#include "core/reactionvariant.h"
#include "data/variantservice.h"
#include <QMainWindow>
#include <array>
#include <optional>
class QComboBox;
class QSpinBox;
class QLabel;
class QAction;
class ReactionTable;
class TranslationManager;
class LogWindow;
class VariantImagePanel;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    ~MainWindow() override;
    explicit MainWindow(VariantService *service, QWidget *parent = nullptr);
    MainWindow(VariantService *service, TranslationManager *translator, QWidget *parent);

  protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    Ui::MainWindow *ui = nullptr;
    bool m_initialSizeApplied = false;
    VariantService *m_service;
    TranslationManager *m_translator;
    ReactionTable *m_reaction;
    QSpinBox *m_number;
    QComboBox *m_type;
    std::array<QSpinBox *, 3> m_mealy, m_moore;
    QLabel *m_feedback;
    LogWindow *m_log = nullptr;
    ReactionVariant m_baseline;
    std::optional<Result> m_lastResult;
    VariantImagePanel *m_imagePanel;
    bool m_loading = false;
    void setupMenus();
    void refreshTexts();
    void loadData(const ReactionVariant &data);
    ReactionVariant readData() const;
    void showResult(const Result &result);
    void updateModified();
    bool confirmDiscard(QWidget *parent = nullptr);
    bool save();
    void newVariant();
    void openVariant();
    void importArchive();
    void exportArchive();
};
