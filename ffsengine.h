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
#include <QQueue>

#include "basetypes.h"
#include "treeitem.h"
#include "treemodel.h"

class TreeModel;

class FfsEngine : public QObject
{
    Q_OBJECT

public:
    // Default constructor and destructor
    FfsEngine(QObject *parent = 0);
    ~FfsEngine(void);
    // Returns model for Qt view classes
    TreeModel* model() const;
    // Returns current message
    QString message() const;

    // Firmware image parsing
    UINT8 parseInputFile(const QByteArray & buffer);
    UINT8 parseRegion(const QByteArray & flashImage, const UINT8 regionSubtype,  const UINT16 regionBase, 
                      const UINT16 regionLimit, const QModelIndex & parent, QModelIndex & regionIndex);
    UINT8 parseBios(const QByteArray & bios, const QModelIndex & parent = QModelIndex());
    UINT8 findNextVolume(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & nextVolumeOffset);
    UINT8 getVolumeSize(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & volumeSize);
    UINT8 parseVolume(const QByteArray & volume, const QModelIndex & parent = QModelIndex(), const UINT8 mode = INSERT_MODE_APPEND);
    UINT8 getFileSize(const QByteArray & volume, const UINT32 fileOffset, UINT32 & fileSize);
    UINT8 parseFile(const QByteArray & file, const UINT8 revision, const char empty = '\xFF', const QModelIndex & parent = QModelIndex(), const UINT8 mode = INSERT_MODE_APPEND);
    UINT8 getSectionSize(const QByteArray & file, const UINT32 sectionOffset, UINT32 & sectionSize);
    UINT8 parseSections(const QByteArray & body, const UINT8 revision, const char empty = '\xFF', const QModelIndex & parent = QModelIndex());
    UINT8 parseSection(const QByteArray & section, const UINT8 revision, const char empty = '\xFF', const QModelIndex & parent = QModelIndex(), const UINT8 mode = INSERT_MODE_APPEND);

    // Compression routines
    UINT8 decompress(const QByteArray & compressed, const UINT8 compressionType, QByteArray & decompressedData, UINT8 * algorithm = NULL);
    UINT8 compress(const QByteArray & data, const UINT8 algorithm, QByteArray & compressedData);
    
    // Construction routines
    UINT8 reconstructImage(QByteArray & reconstructed);
    UINT8 constructPadFile(const UINT32 size, const UINT8 revision, const char empty, QByteArray & pad);
    UINT8 reconstruct(TreeItem* item, QQueue<QByteArray> & queue, const UINT8 revision = 2, char empty = '\xFF');
    
    // Operations on tree items
    UINT8 insert(const QModelIndex & index, const QByteArray & object, const UINT8 type, const UINT8 mode);
    UINT8 remove(const QModelIndex & index);
    
    // Proxies to model operations
    QByteArray header(const QModelIndex & index) const;
    bool       hasEmptyHeader(const QModelIndex & index) const;
    QByteArray body(const QModelIndex & index) const;
    bool       hasEmptyBody(const QModelIndex & index) const;
    
    // Item-related operations
    bool        isOfType(UINT8 type, const QModelIndex & index) const;
    bool        isOfSubtype(UINT8 subtype, const QModelIndex & index) const;
    QModelIndex findParentOfType(UINT8 type, const QModelIndex& index) const;
    
    // Will be refactored later
    bool isCompressedFile(const QModelIndex & index) const;
    QByteArray decompressFile(const QModelIndex & index) const;

private:
    QString   text;
    TreeItem  *rootItem;
    TreeModel *treeModel;
    // Adds string to message
    void msg(const QString & message);
    // Internal operations used in insertInTree
    bool setTreeItemName(const QString & data, const QModelIndex & index);
    bool setTreeItemText(const QString & data, const QModelIndex & index);
};

#endif
