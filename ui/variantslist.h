#ifndef VARIANTSLIST_H
#define VARIANTSLIST_H

#include "data/variantservice.h"
#include <QDialog>

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

private slots:
    void onLoadClicked();
    void onDeleteClicked();
    void onSelectionChanged();

private:
    Ui::VariantsList *ui;
    VariantService* m_service;

    // Храним выбор
    int m_selectedNumber = 0;
    VariantType m_selectedType = VariantType::MooreToMealy;
    bool m_hasSelection = false;

    void populateTable();
    void configureTable();

    struct VariantRef {
        int number;
        VariantType type;
    };
    std::optional<VariantRef> variantRefAt(int row) const;

};

#endif // VARIANTSLIST_H
