#include "ui/widgets/variantimagepanel.h"
#include "ui/appicons.h"
#include <QBuffer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QUrl>

VariantImagePanel::VariantImagePanel(QGroupBox *group, QLabel *preview, QLabel *fileLabel,
                                     QPushButton *loadButton, QPushButton *removeButton)
    : QObject(group), m_group(group), m_preview(preview), m_fileLabel(fileLabel),
      m_loadButton(loadButton), m_removeButton(removeButton) {
    m_group->installEventFilter(this);
    m_loadButton->setIcon(AppIcons::icon(AppIcons::Kind::Image));
    m_removeButton->setIcon(AppIcons::icon(AppIcons::Kind::Remove));
    m_preview->installEventFilter(this);
    m_fileLabel->installEventFilter(this);
    connect(m_loadButton, &QPushButton::clicked, this, &VariantImagePanel::chooseImage);
    connect(m_removeButton, &QPushButton::clicked, this, [this] {
        setImage({});
        emit imageChanged();
    });
    refreshPreview();
}

void VariantImagePanel::setImage(const VariantImage &image) {
    m_image = image;
    m_pixmap = {};
    if (!image.data.isEmpty()) {
        QBuffer buffer;
        buffer.setData(image.data);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        reader.setAutoTransform(true);
        m_pixmap = QPixmap::fromImage(reader.read());
    }
    refreshPreview();
}

bool VariantImagePanel::loadFromFile(const QString &path) {
    VariantImage loaded;
    const auto result = VariantImage::fromFile(path, &loaded);
    if (!result.ok()) {
        emit loadFailed(result);
        return false;
    }
    setImage(loaded);
    emit imageChanged();
    return true;
}

void VariantImagePanel::chooseImage() {
    const auto path =
        QFileDialog::getOpenFileName(m_group, tr("Select the task image"), {},
                                     tr("Images (*.png *.jpg *.jpeg *.bmp *.webp);;All files (*)"));
    if (!path.isEmpty())
        loadFromFile(path);
}

bool VariantImagePanel::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_group && event->type() == QEvent::LanguageChange)
        refreshPreview();
    if ((event->type() == QEvent::Resize || event->type() == QEvent::FontChange ||
         event->type() == QEvent::StyleChange) && (watched == m_preview || watched == m_fileLabel))
        refreshPreview();
    if (watched != m_preview)
        return QObject::eventFilter(watched, event);
    if (event->type() == QEvent::MouseButtonRelease &&
        static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
        chooseImage();
        return true;
    }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::Drop) {
        auto *drop = static_cast<QDropEvent *>(event);
        const auto urls = drop->mimeData()->urls();
        if (urls.size() == 1 && urls.first().isLocalFile()) {
            if (event->type() == QEvent::DragEnter || loadFromFile(urls.first().toLocalFile()))
                drop->acceptProposedAction();
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

void VariantImagePanel::refreshPreview() {
    m_fileLabel->setMinimumWidth(m_fileLabel->fontMetrics().horizontalAdvance(tr("No file selected")));
    m_removeButton->setEnabled(!m_image.data.isEmpty());
    if (m_image.data.isEmpty()) {
        m_preview->setPixmap({});
        m_preview->setText(tr("Drag an image here\nor click to select a file"));
        m_fileLabel->setText(tr("No file selected"));
        m_fileLabel->setToolTip({});
        return;
    }
    const auto area = m_preview->contentsRect().size() - QSize(8, 8);
    m_preview->setPixmap(m_pixmap.scaled(area.expandedTo(QSize(1, 1)), Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
    const auto title =
        tr("%1 · %2 KB").arg(m_image.fileName).arg((m_image.data.size() + 1023) / 1024);
    m_fileLabel->setText(m_fileLabel->fontMetrics().elidedText(
        title, Qt::ElideMiddle, m_fileLabel->contentsRect().width()));
    m_fileLabel->setToolTip(title);
}
