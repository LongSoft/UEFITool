#ifndef GOTOOFFSETDIALOG_H
#define GOTOOFFSETDIALOG_H

#include <QObject>
#include <QDialog>
#include "ui_gotooffsetdialog.h"
class GoToOffsetDialog : public QDialog
{
    Q_OBJECT

public:
    GoToOffsetDialog(QWidget* parent = NULL):
        QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
        ui(new Ui::GoToOffsetDialog) {
        ui->setupUi(this);
    }

    ~GoToOffsetDialog() {delete ui;}

    Ui::GoToOffsetDialog* ui;
};

#endif // GOTOOFFSETDIALOG_H
