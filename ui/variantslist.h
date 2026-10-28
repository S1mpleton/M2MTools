#ifndef VARIANTSLIST_H
#define VARIANTSLIST_H

#include "data/variantservice.h"
#include <QDialog>
#include <optional>

class QComboBox;
class QPushButton;

namespace Ui {
class VariantsList;
}

class VariantsList : public QDialog
{
    Q_OBJECT

public:
    explicit VariantsList(VariantService* service, QWidget *parent = nullptr);
    ~VariantsList();

    int selectedVariantNumber() const { return m_selectedNumber; }
    VariantType selectedVariantType() const { return m_selectedType; }

    bool hasSelection() const { return m_hasSelection; }

signals:
    void variantDeleted(int number);
    void variantsSwapped(int firstNumber, int secondNumber);

private slots:
    void onLoadClicked();
    void onDeleteClicked();
    void onSelectionChanged();
    void onSwapClicked();

private:
    Ui::VariantsList *ui;
    VariantService* m_service;

    // Current selection
    int m_selectedNumber = 0;
    VariantType m_selectedType = VariantType::MooreToMealy;
    bool m_hasSelection = false;

    QComboBox* m_swapFirst = nullptr;
    QComboBox* m_swapSecond = nullptr;
    QPushButton* m_swapButton = nullptr;

    void populateTable();
    void configureTable();
    void configureSwapControls();
    void populateSwapControls(const QList<VariantRow>& variants);

    struct VariantRef {
        int number;
        VariantType type;
    };
    std::optional<VariantRef> variantRefAt(int row) const;

};

#endif // VARIANTSLIST_H
