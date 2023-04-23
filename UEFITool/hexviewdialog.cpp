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
    hexView = new QHexView(this);
    hexView->setReadOnly(true);
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

void HexViewDialog::setItem(const UModelIndex & index, HexViewType type)
{
    const TreeModel * model = (const TreeModel*)index.model();
    UString itemName = model->name(index);
    UString itemText = model->text(index);
    
    // Set hex data and dialog title
    QByteArray hexdata;
    UString dialogTitle;
    
    switch (type) {
        case fullHexView:
            dialogTitle = UString("Hex view: ");
            hexdata = model->header(index) + model->body(index) + model->tail(index);
            break;
        case bodyHexView:
            dialogTitle = UString("Body hex view: ");
            hexdata = model->body(index);
            break;
        case uncompressedHexView:
            dialogTitle = UString("Uncompressed hex view: ");
            hexdata = model->uncompressedData(index);
            break;
    }
    
    dialogTitle += itemText.isEmpty() ? itemName : itemName + " | " + itemText;
    setWindowTitle(dialogTitle);
    hexView->setData(hexdata);
    hexView->setFont(QApplication::font());
}
