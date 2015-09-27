#include "hantekdso.h"


HantekDSO::HantekDSO(QWidget *parent) :
    QMainWindow(parent)
{
    widget = new HantekDSOWidget( this );
    setCentralWidget( widget );

}

HantekDSO::~HantekDSO()
{
}

bool HantekDSO::queryClose()
{
    widget->saveSetting();
    return true;
}
