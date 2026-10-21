#ifndef VARIANTIMAGEPANEL_H
#define VARIANTIMAGEPANEL_H

#include <QByteArray>
#include <QGroupBox>

class QLabel;
class QPushButton;

class VariantImagePanel final : public QGroupBox
{
    Q_OBJECT

public:
    explicit VariantImagePanel(QWidget* parent = nullptr);

    void setImage(const QByteArray& imageData, const QString& fileName);
    void setCounts(int states, int inputs, int outputs);
    void setInvalid(bool invalid);

signals:
    void imagePathSelected(const QString& path);
    void removeRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void chooseImage();
    void refreshPreview();
    void retranslateUi();

    QByteArray m_imageData;
    QString m_fileName;
    QLabel* m_preview = nullptr;
    QLabel* m_fileLabel = nullptr;
    QLabel* m_countsLabel = nullptr;
    QPushButton* m_loadButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    int m_stateCount = 0;
    int m_inputCount = 0;
    int m_outputCount = 0;
};

#endif // VARIANTIMAGEPANEL_H
