#ifndef IERRORPRESENTER_H
#define IERRORPRESENTER_H

#include "core/Result.h"
#include <QObject>

class IErrorPresenter : public QObject {
    Q_OBJECT
public:
    explicit IErrorPresenter(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IErrorPresenter() = default;

    virtual void show(const Result& r) = 0;
    virtual void clear(const Result& result) = 0;
};

#endif // IERRORPRESENTER_H
