#include "mealydelegate.h"
#include "editvalidator.h"
#include "compositevalidator.h"
#include "listnamesvalidator.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QEvent>
#include <QApplication>
#include <qabstractitemview.h>


MealyDelegate::MealyDelegate(const AutomatonData* data, QObject* parent)
    : QStyledItemDelegate(parent), m_data(data) {

}

QWidget* MealyDelegate::createContainer(QWidget* parent) {
    // === createContainer ===
    QWidget* container = new QWidget(parent);
    container->setAutoFillBackground(true);
    container->setObjectName("containerEdit");

    // === layout ===
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // === createStateEdit ===
    QLineEdit* stateEdit = new QLineEdit(container);

    stateEdit->setValidator(new EditValidator(stateEdit, QRegularExpression("[^a-zA-Zа-яА-ЯёЁ0-9]")));
    stateEdit->setObjectName("stateEdit");

    // === seporator ===
    QLabel* slash = new QLabel("/", container);
    slash->setAlignment(Qt::AlignCenter);

    // === createOutputEdit ===
    QLineEdit* outputEdit = new QLineEdit(container);

    CompositeValidator *composite = new CompositeValidator(outputEdit);
    composite->addValidator(new EditValidator());
    composite->addValidator(new ListNamesValidator());

    outputEdit->setValidator(composite);
    outputEdit->setObjectName("outputEdit");

    // === settings ===
    container->setFocusProxy(stateEdit);

    layout->addWidget(stateEdit, 1);
    layout->addWidget(slash, 0);
    layout->addWidget(outputEdit, 1);

    container->setLayout(layout);

    return container;
}

QWidget* MealyDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const {
    qDebug() << "=== createEditor ===";
    QWidget* container = createContainer(parent);

    auto* stateEdit  = container->findChild<QLineEdit*>("stateEdit");
    auto* outputEdit = container->findChild<QLineEdit*>("outputEdit");

    Q_ASSERT(stateEdit && outputEdit);

    stateEdit->installEventFilter(const_cast<MealyDelegate*>(this));
    outputEdit->installEventFilter(const_cast<MealyDelegate*>(this));

    return container;
}

void MealyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    qDebug() << "=== setEditorData ===";
    qDebug() << "  raw:" << index.data(Qt::EditRole).toString();

    QString text = index.data(Qt::EditRole).toString();
    QString statePart, outputPart;

    int slashPos = text.indexOf('/');

    qDebug() << "text =" << text;
    qDebug() << "text.toUtf8().toHex() =" << text.toUtf8().toHex();
    qDebug() << "slashPos =" << slashPos;

    if (slashPos >= 0) {
        statePart  = text.left(slashPos).trimmed();
        outputPart = text.mid(slashPos + 1).trimmed();
    } else {
        statePart = text.trimmed();
    }

    auto* container = qobject_cast<QWidget*>(editor);
    if (!container) return;

    auto* stateEdit  = container->findChild<QLineEdit*>("stateEdit");
    auto* outputEdit = container->findChild<QLineEdit*>("outputEdit");
    qDebug() << "stateEdit =" << stateEdit;
    qDebug() << "outputEdit =" << outputEdit;
    if (stateEdit)  stateEdit->setText(statePart);
    if (outputEdit) outputEdit->setText(outputPart);
}

void MealyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    qDebug() << "=== setModelData ===";
    auto* container = qobject_cast<QWidget*>(editor);
    if (!container) return;

    auto* stateEdit  = container->findChild<QLineEdit*>("stateEdit");
    auto* outputEdit = container->findChild<QLineEdit*>("outputEdit");
    if (!stateEdit || !outputEdit) return;

    QString stateText  = stateEdit->text().trimmed();
    QString outputText = outputEdit->text().trimmed();

    if (stateText.isEmpty()  || stateText == "-")  stateText = "—";
    if (outputText.isEmpty() || outputText == "-") outputText = "—";

    QString result = stateText + " / " + normalizeOutputList(outputText);
    model->setData(index, result, Qt::EditRole);
}

bool MealyDelegate::eventFilter(QObject *object, QEvent *event) {
    auto* lineEdit = qobject_cast<QLineEdit*>(object);
    if (lineEdit && (lineEdit->objectName() == "stateEdit" || lineEdit->objectName() == "outputEdit")) {
        if (event->type() == QEvent::FocusOut) {
            QWidget* container = lineEdit->parentWidget();
            if (container) {
                QMetaObject::invokeMethod(this, [this, container]() {
                    QWidget* currentFocus = QApplication::focusWidget();

                    if (!container->isAncestorOf(currentFocus) && currentFocus != container) {
                        emit commitData(container);
                        emit closeEditor(container);
                    }
                }, Qt::QueuedConnection);
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

QString MealyDelegate::normalizeOutputList(const QString& text) {
    if (text == "—") return text;
    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (QString& p : parts) p = p.trimmed();
    return parts.join(", ");
}

