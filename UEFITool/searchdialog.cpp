/* searchdialog.cpp

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include "searchdialog.h"

SearchDialog::SearchDialog(QWidget *parent) :
QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
ui(new Ui::SearchDialog),
hexValidator(QRegExp("([0-9a-fA-F\\. ])*")),
guidValidator(QRegExp("[0-9a-fA-F\\.]{8}-[0-9a-fA-F\\.]{4}-[0-9a-fA-F\\.]{4}-[0-9a-fA-F\\.]{4}-[0-9a-fA-F\\.]{12}"))
{
    // Create UI
    ui->setupUi(this);
    ui->hexEdit->setValidator(&hexValidator);
    ui->guidEdit->setValidator(&guidValidator);

    // Connect
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setEditFocus(int)));

    // Set initial focus
    setEditFocus(ui->tabWidget->currentIndex());
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::setEditFocus(int index)
{
    if (index == 0) // Hex pattern
        ui->hexEdit->setFocus();
    else if (index == 1) { // GUID
        ui->guidEdit->setFocus();
        ui->guidEdit->setCursorPosition(0);
    }
    else if (index == 2) // Text
        ui->textEdit->setFocus();
}