#pragma once
#include <QLabel>

// Keep wrapped text in the layout even when fonts, DPI or available width change.
// Promoted from QLabel in Designer; all content and layout remain in the .ui file.
class WrappingLabel : public QLabel {
    Q_OBJECT
  public:
    explicit WrappingLabel(QWidget *parent = nullptr);
    void refreshHeight();

  protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
};
