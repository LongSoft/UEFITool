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

#include <math.h>

#include "treemodel.h"

#include "basetypes.h"
#include "descriptor.h"
#include "ffs.h"
#include "Tiano/EfiTianoCompress.h"
#include "Tiano/EfiTianoDecompress.h"
#include "LZMA/LzmaCompress.h"
#include "LZMA/LzmaDecompress.h"

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
    //void saveImageFile();
    void populateUi(const QModelIndex &current/*, const QModelIndex &previous*/);
    void resizeTreeViewColums(/*const QModelIndex &index*/);
    void saveAll();
    void saveBody();

private:
    Ui::UEFITool * ui;
    TreeModel * treeModel;
    QModelIndex currentIndex;

    void debug(const QString & text);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    QModelIndex addTreeItem(UINT8 type, UINT8 subtype, const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(), 
        const QModelIndex & parent = QModelIndex());

    UINT8 parseInputFile(const QByteArray & buffer);
    UINT8* parseRegion(const QByteArray & flashImage, UINT8 regionSubtype, const UINT16 regionBase, const UINT16 regionLimit, QModelIndex & index);
    UINT8 parseBios(const QByteArray & bios, const QModelIndex & parent = QModelIndex());
    INT32 getNextVolumeIndex(const QByteArray & bios, INT32 volumeIndex = 0);
    UINT32 getVolumeSize(const QByteArray & bios, INT32 volumeIndex);
    UINT8 parseVolume(const QByteArray & volume, UINT32 volumeBase, UINT8 revision, bool erasePolarity, const QModelIndex & parent = QModelIndex());
    UINT8 parseFile(const QByteArray & file, UINT8 revision, bool erasePolarity, const QModelIndex & parent = QModelIndex());
};

#endif
