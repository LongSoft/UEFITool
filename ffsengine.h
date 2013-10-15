/* ffsengine.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FFSENGINE_H__
#define __FFSENGINE_H__

#include <QObject>
#include <QModelIndex>
#include <QByteArray>

#include "basetypes.h"
#include "treemodel.h"

class TreeModel;

class FfsEngine : public QObject
{
    Q_OBJECT

public:
    FfsEngine(QObject *parent = 0);
    ~FfsEngine(void);

    TreeModel* model() const;
    QString message() const;

    UINT8 parseInputFile(const QByteArray & buffer);
    UINT8* parseRegion(const QByteArray & flashImage, UINT8 regionSubtype, const UINT16 regionBase, const UINT16 regionLimit, QModelIndex & index);
    UINT8 parseBios(const QByteArray & bios, const QModelIndex & parent = QModelIndex());
    INT32 findNextVolume(const QByteArray & bios, INT32 volumeOffset = 0);
    UINT32 getVolumeSize(const QByteArray & bios, INT32 volumeOffset);
    UINT8 parseVolume(const QByteArray & volume, UINT32 volumeBase, UINT8 revision, bool erasePolarity, const QModelIndex & parent = QModelIndex());
    UINT8 parseFile(const QByteArray & file, UINT8 revision, bool erasePolarity, const QModelIndex & parent = QModelIndex());

    QModelIndex addTreeItem(const UINT8 type, const UINT8 subtype = 0, const UINT32 offset = 0, 
        const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(), 
        const QModelIndex & parent = QModelIndex());
    bool removeItem(const QModelIndex &index);

    QByteArray header(const QModelIndex& index) const;
    bool hasEmptyHeader(const QModelIndex& index) const;
    QByteArray body(const QModelIndex& index) const;
    bool hasEmptyBody(const QModelIndex& index) const;
    
    bool isCompressedFile(const QModelIndex& index) const;
    QByteArray uncompressFile(const QModelIndex& index) const;

private:
    QString text;
    TreeItem *rootItem;
    TreeModel *treeModel;
    void msg(const QString & message);
    QModelIndex findParentOfType(UINT8 type, const QModelIndex& index) const;
    bool setTreeItemName(const QString &data, const QModelIndex &index);
    bool setTreeItemText(const QString &data, const QModelIndex &index);
};

#endif
