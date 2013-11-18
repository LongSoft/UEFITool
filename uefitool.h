/* uefitool.h

  Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __UEFITOOL_H__
#define __UEFITOOL_H__

#include <QMainWindow>
#include <QByteArray>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QString>
#include <QUrl>

#include "basetypes.h"
#include "ffs.h"
#include "ffsengine.h"

namespace Ui {
class UEFITool;
}

class UEFITool : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit UEFITool(QWidget *parent = 0);
    ~UEFITool();

    void openImageFile(QString path);

private slots:
    void init();
    void openImageFile();
    void saveImageFile();
    void populateUi(const QModelIndex &current);
    void resizeTreeViewColums();
    void extract(const UINT8 mode);
    void extractAsIs();
    void extractBody();
    void extractUncompressed();
    void insert(const UINT8 mode);
    void insertInto();
    void insertBefore();
    void insertAfter();
    void replace();
    void remove();
    void rebuild();
	void changeToNone();
    void changeToEfi11();
    void changeToTiano();
    void changeToLzma();
    void about();
    void aboutQt();
    void scrollTreeView(QListWidgetItem* item);

private:
    Ui::UEFITool * ui;
    FfsEngine* ffsEngine;
    QQueue<MessageListItem> messageItems;
	
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void showMessage();
};

#endif
