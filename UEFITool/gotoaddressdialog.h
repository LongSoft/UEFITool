#ifndef GOTOADDRESSDIALOG_H
#define GOTOADDRESSDIALOG_H

#include <QObject>
#include <QDialog>
#include "ui_gotoaddressdialog.h"
class GoToAddressDialog : public QDialog
{
    Q_OBJECT

public:
    GoToAddressDialog(QWidget* parent = NULL) :
        QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
        ui(new Ui::GoToAddressDialog) {
        ui->setupUi(this);
    }

    ~GoToAddressDialog() { delete ui; }

    Ui::GoToAddressDialog* ui;
};

#endif // GOTOADDRESSDIALOG_H
