/* hexviewdialog.h

  Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef HEXVIEWDIALOG_H
#define HEXVIEWDIALOG_H

#include <QDialog>
#include "../common/treemodel.h"
#include "qhexview5/qhexview.h"
#include "ui_hexviewdialog.h"

class HexViewDialog : public QDialog
{
    Q_OBJECT

public:
    enum HexViewType {
        fullHexView,
        bodyHexView,
        uncompressedHexView
    };
    
    HexViewDialog(QWidget *parent = 0);
    ~HexViewDialog();
    Ui::HexViewDialog* ui;

    void setItem(const UModelIndex & index, HexViewType dataType);
    void setFont(const QFont &font);
    
private:
    QHexView * hexView;
};

#endif // HEXVIEWDIALOG_H
