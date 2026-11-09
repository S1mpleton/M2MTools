#pragma once
#include <QObject>
#include <QPointer>
#include <QWidget>

// Restore the original appearance when feedback is cleared; never style the whole form.
class FieldFeedback : public QObject {
  public:
    explicit FieldFeedback(QObject *parent);
    void mark(QWidget *widget, const QString &message);
    void clear();
  protected:
    bool eventFilter(QObject *object, QEvent *event) override;


  private:
    struct Field {
        QPointer<QWidget> widget;
        QString style;
        QString tooltip;
    };
    QList<Field> m_fields;
    void apply(const Field &field);
};
