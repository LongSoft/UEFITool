#ifndef GOTOBASEDIALOG_H
#define GOTOBASEDIALOG_H

#include <QObject>
#include <QDialog>
#include "ui_gotobasedialog.h"
class GoToBaseDialog : public QDialog
{
    Q_OBJECT

public:
    GoToBaseDialog(QWidget* parent = NULL):
        QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
        ui(new Ui::GoToBaseDialog) {
        ui->setupUi(this);
    }

    ~GoToBaseDialog() {delete ui;}

    Ui::GoToBaseDialog* ui;
};

#endif // GOTOBASEDIALOG_H
