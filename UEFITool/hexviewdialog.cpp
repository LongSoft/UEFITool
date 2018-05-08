/* hexviewdialog.cpp

  Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include "hexviewdialog.h"

HexViewDialog::HexViewDialog(QWidget *parent) :
QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
ui(new Ui::HexViewDialog),
hexView(NULL)
{
    // Create UI
    ui->setupUi(this);
    hexView = new QHexEdit(this);
    hexView->setReadOnly(true);
    hexView->setUpperCase(true);
    ui->layout->addWidget(hexView);
}

HexViewDialog::~HexViewDialog()
{
    delete hexView;
    delete ui;
}

void HexViewDialog::setFont(const QFont &font)
{
    hexView->setFont(font);
}

void HexViewDialog::setItem(const UModelIndex & index, bool bodyOnly)
{
    const TreeModel * model = (const TreeModel*)index.model();
    
    // Set dialog title
    UString itemName = model->name(index);
    UString itemText = model->text(index);
    setWindowTitle(UString("Hex view: ") + (itemText.isEmpty() ? itemName : itemName + " | " + itemText));
    
    // Set hex data
    QByteArray hexdata;
    if (bodyOnly) hexdata = model->body(index);
    else hexdata = model->header(index) + model->body(index) + model->tail(index);
    hexView->setData(hexdata);
}