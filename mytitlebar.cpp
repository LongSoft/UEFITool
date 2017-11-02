#include "mytitlebar.h"
#include <QStyleOption>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>




myTitleBar::myTitleBar(QWidget *parent) : QWidget(parent)
{
this->setStyleSheet("QWidget  {color:white; background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1 stop:0 rgba(170, 0, 0, 255), stop:1 rgba(0, 0, 0, 255)); padding-bottom:5px;}  ");
}

void myTitleBar::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void myTitleBar::mouseDoubleClickEvent(QMouseEvent * e )
{
    if ( e->button() == Qt::LeftButton )
    {
        qDebug() << "double click pressed";

        emit toggleViewSignal();

    }
}



