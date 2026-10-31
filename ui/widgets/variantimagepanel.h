#pragma once
#include "core/variantimage.h"
#include <QObject>
class QGroupBox;
class QLabel;
class QPushButton;
#include <QPixmap>

class VariantImagePanel final : public QObject {
    Q_OBJECT
  public:
    VariantImagePanel(QGroupBox *group, QLabel *preview, QLabel *fileLabel, QPushButton *loadButton,
                      QPushButton *removeButton);
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

  private:
    void chooseImage();
    void refreshPreview();
    QGroupBox *m_group;
    QLabel *m_preview;
    QLabel *m_fileLabel;
    QPushButton *m_loadButton;
    QPushButton *m_removeButton;
    VariantImage m_image;
    QPixmap m_pixmap;
};
