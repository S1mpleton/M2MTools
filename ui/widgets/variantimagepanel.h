#pragma once
#include "core/variantimage.h"
#include <QGroupBox>
#include <QPixmap>

namespace Ui {
class VariantImagePanel;
}

class VariantImagePanel final : public QGroupBox {
    Q_OBJECT
  public:
    explicit VariantImagePanel(QWidget *parent = nullptr);
    ~VariantImagePanel() override;
    VariantImage image() const {
        return m_image;
    }
    void setImage(const VariantImage &image);
    bool loadFromFile(const QString &path);

  signals:
    void imageChanged();
    void loadFailed(const Result &result);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;

  private:
    void chooseImage();
    void refreshPreview();
    Ui::VariantImagePanel *ui;
    VariantImage m_image;
    QPixmap m_pixmap;
};
