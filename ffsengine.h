/* ffsengine.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FFSENGINE_H__
#define __FFSENGINE_H__

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QModelIndex>
#include <QByteArray>
#include <QQueue>
#include <QVector>

#include "basetypes.h"
#include "treemodel.h"
#include "peimage.h"

#ifndef _CONSOLE
#include "messagelistitem.h"
#endif

class TreeModel;

QString errorMessage(UINT8 errorCode);

struct PatchData {
    UINT8 type;
    UINT32 offset;
    QByteArray hexFindPattern;
    QByteArray hexReplacePattern;
};

class FfsEngine : public QObject
{
    Q_OBJECT

public:
    // Default constructor and destructor
    FfsEngine(QObject *parent = 0);
    ~FfsEngine(void);

    // Returns model for Qt view classes
    TreeModel* treeModel() const;

#ifndef _CONSOLE
    // Returns message items queue
    QQueue<MessageListItem> messages() const;
    // Clears message items queue
    void clearMessages();
#endif

    // Firmware image parsing
    UINT8 parseImageFile(const QByteArray & buffer);
    UINT8 parseIntelImage(const QByteArray & intelImage, QModelIndex & index, const QModelIndex & parent = QModelIndex());
    UINT8 parseGbeRegion(const QByteArray & gbe, QModelIndex & index, const QModelIndex & parent, const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseMeRegion(const QByteArray & me, QModelIndex & index, const QModelIndex & parent, const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseBiosRegion(const QByteArray & bios, QModelIndex & index, const QModelIndex & parent, const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parsePdrRegion(const QByteArray & pdr, QModelIndex & index, const QModelIndex & parent, const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseEcRegion(const QByteArray & ec, QModelIndex & index, const QModelIndex & parent, const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseBios(const QByteArray & bios, const QModelIndex & parent = QModelIndex());
    UINT8 parseVolume(const QByteArray & volume, QModelIndex & index, const QModelIndex & parent = QModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseFile(const QByteArray & file, QModelIndex & index, const UINT8 revision = 2, const UINT8 erasePolarity = ERASE_POLARITY_UNKNOWN, const QModelIndex & parent = QModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);
    UINT8 parseSections(const QByteArray & body, const QModelIndex & parent = QModelIndex());
    UINT8 parseSection(const QByteArray & section, QModelIndex & index, const QModelIndex & parent = QModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);

    // Compression routines
    UINT8 decompress(const QByteArray & compressed, const UINT8 compressionType, QByteArray & decompressedData, UINT8 * algorithm = NULL);
    UINT8 compress(const QByteArray & data, const UINT8 algorithm, const UINT32 dictionarySize, QByteArray & compressedData);

    // Construction routines
    UINT8 reconstructImageFile(QByteArray &reconstructed);
    UINT8 reconstruct(const QModelIndex &index, QByteArray & reconstructed);
    UINT8 reconstructIntelImage(const QModelIndex& index, QByteArray & reconstructed);
    UINT8 reconstructRegion(const QModelIndex& index, QByteArray & reconstructed, bool includeHeader = true);
    UINT8 reconstructPadding(const QModelIndex& index, QByteArray & reconstructed);
    UINT8 reconstructVolume(const QModelIndex& index, QByteArray & reconstructed);
    UINT8 reconstructFile(const QModelIndex& index, const UINT8 revision, const UINT8 erasePolarity, const UINT32 base, QByteArray& reconstructed);
    UINT8 reconstructSection(const QModelIndex& index, const UINT32 base, QByteArray & reconstructed);

    // Operations on tree items
    UINT8 extract(const QModelIndex & index, QByteArray & extracted, const UINT8 mode);
    UINT8 create(const QModelIndex & index, const UINT8 type, const QByteArray & header, const QByteArray & body, const UINT8 mode, const UINT8 action, const UINT8 algorithm = COMPRESSION_ALGORITHM_NONE);
    UINT8 insert(const QModelIndex & index, const QByteArray & object, const UINT8 mode);
    UINT8 replace(const QModelIndex & index, const QByteArray & object, const UINT8 mode);
    UINT8 remove(const QModelIndex & index);
    UINT8 rebuild(const QModelIndex & index);
    UINT8 doNotRebuild(const QModelIndex & index);
    UINT8 dump(const QModelIndex & index, const QString & path, const QString & filter = QString());
    UINT8 patch(const QModelIndex & index, const QVector<PatchData> & patches);

    // Search routines
    UINT8 findHexPattern(const QModelIndex & index, const QByteArray & hexPattern, const UINT8 mode);
    UINT8 findGuidPattern(const QModelIndex & index, const QByteArray & guidPattern, const UINT8 mode);
    UINT8 findTextPattern(const QModelIndex & index, const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive);

private:
    TreeModel *model;

    // PEI Core entry point
    UINT32 oldPeiCoreEntryPoint;
    UINT32 newPeiCoreEntryPoint;

    // Parsing helpers
    UINT32 getPaddingType(const QByteArray & padding);
    void  parseAprioriRawSection(const QByteArray & body, QString & parsed);
    UINT8 parseDepexSection(const QByteArray & body, QString & parsed);
    UINT8 findNextVolume(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & nextVolumeOffset);
    UINT8 getVolumeSize(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize);
    UINT8 getSectionSize(const QByteArray & file, const UINT32 sectionOffset, UINT32 & sectionSize);

    // Reconstruction helpers
    UINT8 constructPadFile(const QByteArray &guid, const UINT32 size, const UINT8 revision, const UINT8 erasePolarity, QByteArray & pad);
    UINT8 growVolume(QByteArray & header, const UINT32 size, UINT32 & newSize);

    // Rebase routines
    UINT8 getBase(const QByteArray& file, UINT32& base);
    UINT8 getEntryPoint(const QByteArray& file, UINT32 &entryPoint);
    UINT8 rebase(QByteArray & executable, const UINT32 base, const QModelIndex & index);
    void  rebasePeiFiles(const QModelIndex & index);

    // Patch routines
    UINT8 patchVtf(QByteArray &vtf);

    // Patch helpers
    UINT8 patchViaOffset(QByteArray & data, const UINT32 offset, const QByteArray & hexReplacePattern);
    UINT8 patchViaPattern(QByteArray & data, const QByteArray & hexFindPattern, const QByteArray & hexReplacePattern);

#ifndef _CONSOLE
    QQueue<MessageListItem> messageItems;
#endif
    // Message helper
    void msg(const QString & message, const QModelIndex &index = QModelIndex());

    // Internal operations
    bool hasIntersection(const UINT32 begin1, const UINT32 end1, const UINT32 begin2, const UINT32 end2);
    UINT32 crc32(UINT32 initial, const UINT8* buffer, UINT32 length);

    // Recursive dump
    bool dumped;
    UINT8 recursiveDump(const QModelIndex & index, const QString & path, const QString & filter);
};

#endif
