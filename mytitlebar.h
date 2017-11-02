#ifndef MYTITLEBAR_H
#define MYTITLEBAR_H

#include <QWidget>

class myTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit myTitleBar(QWidget *parent = 0);

protected:
 void paintEvent(QPaintEvent *);
 void mouseDoubleClickEvent( QMouseEvent * e );


signals:
 void toggleViewSignal();

public slots:
};

#endif // MYTITLEBAR_H
