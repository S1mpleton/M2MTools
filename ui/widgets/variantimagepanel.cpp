#include "variantimagepanel.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

VariantImagePanel::VariantImagePanel(QWidget* parent)
    : QGroupBox(QStringLiteral("Изображение задания и состав"), parent)
{
    setObjectName(QStringLiteral("diagramGroup"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_preview = new QLabel(this);
    m_preview->setObjectName(QStringLiteral("diagramDropZone"));
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameShape(QFrame::StyledPanel);
    m_preview->setFrameShadow(QFrame::Sunken);
    m_preview->setMinimumSize(250, 96);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview->setAcceptDrops(true);
    m_preview->installEventFilter(this);
    layout->addWidget(m_preview, 3);

    auto* actions = new QVBoxLayout;
    actions->setSpacing(5);
    m_fileLabel = new QLabel(QStringLiteral("Файл не выбран"), this);
    m_fileLabel->setObjectName(QStringLiteral("imageFileLabel"));
    m_fileLabel->setWordWrap(true);
    m_countsLabel = new QLabel(this);
    m_countsLabel->setWordWrap(true);

    auto* loadButton = new QPushButton(
        QStringLiteral("Загрузить изображение"), this);
    loadButton->setObjectName(QStringLiteral("loadImageButton"));
    m_removeButton = new QPushButton(
        QStringLiteral("Удалить изображение"), this);
    m_removeButton->setObjectName(QStringLiteral("removeImageButton"));

    actions->addWidget(m_fileLabel);
    actions->addWidget(m_countsLabel);
    actions->addStretch();
    actions->addWidget(loadButton);
    actions->addWidget(m_removeButton);
    layout->addLayout(actions, 2);

    connect(loadButton, &QPushButton::clicked,
            this, &VariantImagePanel::chooseImage);
    connect(m_removeButton, &QPushButton::clicked,
            this, &VariantImagePanel::removeRequested);

    refreshPreview();
}

void VariantImagePanel::setImage(
    const QByteArray& imageData, const QString& fileName) {
    m_imageData = imageData;
    m_fileName = fileName;
    setInvalid(false);
    refreshPreview();
}

void VariantImagePanel::setCounts(int states, int inputs, int outputs) {
    m_countsLabel->setText(
        QStringLiteral("Состояний: %1  ·  Входов: %2  ·  Выходов: %3")
            .arg(states).arg(inputs).arg(outputs));
}

void VariantImagePanel::setInvalid(bool invalid) {
    m_preview->setStyleSheet(invalid
        ? QStringLiteral(
              "background: #fff1f2; color: #7f1d1d; "
              "border: 2px solid #dc2626;")
        : QString());
}

bool VariantImagePanel::eventFilter(QObject* watched, QEvent* event) {
    if (watched != m_preview)
        return QGroupBox::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonRelease) {
        chooseImage();
        return true;
    }
    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasUrls()) {
            dragEvent->acceptProposedAction();
            return true;
        }
    }
    if (event->type() == QEvent::Drop) {
        auto* dropEvent = static_cast<QDropEvent*>(event);
        const QList<QUrl> urls = dropEvent->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            emit imagePathSelected(urls.first().toLocalFile());
            dropEvent->acceptProposedAction();
            return true;
        }
    }
    if (event->type() == QEvent::Resize && !m_imageData.isEmpty()) {
        QTimer::singleShot(0, this, &VariantImagePanel::refreshPreview);
    }
    return QGroupBox::eventFilter(watched, event);
}

void VariantImagePanel::chooseImage() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Выберите изображение задания"), QString(),
        QStringLiteral(
            "Изображения (*.png *.jpg *.jpeg *.bmp *.webp);;Все файлы (*)"));
    if (!path.isEmpty()) emit imagePathSelected(path);
}

void VariantImagePanel::refreshPreview() {
    if (m_imageData.isEmpty()) {
        m_preview->setPixmap(QPixmap());
        m_preview->setText(QStringLiteral(
            "Перетащите изображение сюда\nили нажмите для выбора файла"));
        m_fileLabel->setText(QStringLiteral("Файл не выбран"));
        m_removeButton->setEnabled(false);
        return;
    }

    QPixmap image;
    image.loadFromData(m_imageData);
    const QSize target = m_preview->size() - QSize(12, 12);
    m_preview->setText(QString());
    m_preview->setPixmap(
        image.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_fileLabel->setText(
        QStringLiteral("%1 · %2 КБ")
            .arg(m_fileName)
            .arg(m_imageData.size() / 1024));
    m_removeButton->setEnabled(true);
}
