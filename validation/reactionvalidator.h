#pragma once
#include <QValidator>

class ReactionValidator : public QValidator {
  public:
    explicit ReactionValidator(bool outputs, QObject *parent = nullptr);
    State validate(QString &input, int &position) const override;

  private:
    bool m_outputs;
};
