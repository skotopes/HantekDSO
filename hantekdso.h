#ifndef HANTEKDSO_H
#define HANTEKDSO_H

#include <QMainWindow>
#include "hantekdsowidget.h"

class HantekDSO : public QMainWindow
{
    Q_OBJECT

public:
    explicit HantekDSO(QWidget *parent = 0);
    ~HantekDSO();

protected:
    virtual bool queryClose();

private:
    HantekDSOWidget *widget;
};

#endif // HANTEKDSO_H
