#include "ui/widgets/variantimagepanel.h"
#include "ui/appicons.h"
#include "ui_variantimagepanel.h"
#include <QBuffer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QImageReader>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QUrl>

VariantImagePanel::VariantImagePanel(QWidget *parent)
    : QGroupBox(parent), ui(new Ui::VariantImagePanel) {
    ui->setupUi(this);
    ui->loadImageButton->setIcon(AppIcons::icon(AppIcons::Kind::Image));
    ui->removeImageButton->setIcon(AppIcons::icon(AppIcons::Kind::Remove));
    ui->diagramDropZone->installEventFilter(this);
    ui->imageFileLabel->installEventFilter(this);
    connect(ui->loadImageButton, &QPushButton::clicked, this, &VariantImagePanel::chooseImage);
    connect(ui->removeImageButton, &QPushButton::clicked, this, [this] {
        setImage({});
        emit imageChanged();
    });
    refreshPreview();
}

VariantImagePanel::~VariantImagePanel() {
    delete ui;
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
        QFileDialog::getOpenFileName(this, tr("Select the task image"), {},
                                     tr("Images (*.png *.jpg *.jpeg *.bmp *.webp);;All files (*)"));
    if (!path.isEmpty())
        loadFromFile(path);
}

bool VariantImagePanel::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize &&
        (watched == ui->diagramDropZone || watched == ui->imageFileLabel))
        refreshPreview();
    if (watched != ui->diagramDropZone)
        return QGroupBox::eventFilter(watched, event);
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
    return QGroupBox::eventFilter(watched, event);
}

void VariantImagePanel::refreshPreview() {
    ui->removeImageButton->setEnabled(!m_image.data.isEmpty());
    if (m_image.data.isEmpty()) {
        ui->diagramDropZone->setPixmap({});
        ui->diagramDropZone->setText(tr("Drag an image here\nor click to select a file"));
        ui->imageFileLabel->setText(tr("No file selected"));
        ui->imageFileLabel->setToolTip({});
        return;
    }
    const auto area = ui->diagramDropZone->contentsRect().size() - QSize(8, 8);
    ui->diagramDropZone->setPixmap(m_pixmap.scaled(area.expandedTo(QSize(1, 1)),
                                                   Qt::KeepAspectRatio, Qt::SmoothTransformation));
    const auto title =
        tr("%1 · %2 KB").arg(m_image.fileName).arg((m_image.data.size() + 1023) / 1024);
    ui->imageFileLabel->setText(ui->imageFileLabel->fontMetrics().elidedText(
        title, Qt::ElideMiddle, ui->imageFileLabel->contentsRect().width()));
    ui->imageFileLabel->setToolTip(title);
}

void VariantImagePanel::changeEvent(QEvent *event) {
    QGroupBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        refreshPreview();
    }
}
