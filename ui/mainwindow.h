#pragma once
#include "core/reactionvariant.h"
#include "data/variantservice.h"
#include <QMainWindow>
#include <array>
#include <functional>
class QComboBox;
class QSpinBox;
class QLabel;
class QGroupBox;
class QAction;
class ReactionTable;
class TranslationManager;
class LogWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(VariantService *service, QWidget *parent = nullptr);
    MainWindow(VariantService *service, TranslationManager *translator, QWidget *parent);

  protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    VariantService *m_service;
    TranslationManager *m_translator;
    ReactionTable *m_reaction;
    QSpinBox *m_number;
    QComboBox *m_type;
    std::array<QSpinBox *, 3> m_mealy, m_moore;
    QLabel *m_feedback;
    LogWindow *m_log = nullptr;
    bool m_dirty = false;
    bool m_loading = false;
    QList<std::function<void()>> m_translations;
    QGroupBox *createCounts(bool mealy);
    void setupMenus();
    void refreshTexts();
    void loadData(const ReactionVariant &data);
    ReactionVariant readData() const;
    void showResult(const Result &result);
    bool confirmDiscard();
    bool save();
    void newVariant();
    void openVariant();
    void importArchive();
    void exportArchive();
};
