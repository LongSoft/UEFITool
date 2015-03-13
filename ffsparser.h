/* ffsparser.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FFSPARSER_H__
#define __FFSPARSER_H__

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
#include "utility.h"
#include "peimage.h"

#ifndef _CONSOLE
#include "messagelistitem.h"
#endif

class TreeModel;

//The whole parsing data is an information needed for each level of image reconstruction
//routines without the need of backward traversal

//typedef struct _CAPSULE_PARSING_DATA {
//} CAPSULE_PARSING_DATA;

//typedef struct _IMAGE_PARSING_DATA {
//} IMAGE_PARSING_DATA;

//typedef struct _PADDING_PARSING_DATA {
//} PADDING_PARSING_DATA;

typedef struct _VOLUME_PARSING_DATA {
    EFI_GUID extendedHeaderGuid;
    UINT32  alignment;
    UINT8   revision;
    BOOLEAN hasExtendedHeader;
    BOOLEAN hasZeroVectorCRC32;
    BOOLEAN isWeakAligned;
} VOLUME_PARSING_DATA;

//typedef struct _FREE_SPACE_PARSING_DATA {
//} FREE_SPACE_PARSING_DATA;

typedef struct _FILE_PARSING_DATA {
    UINT16  tail;
    BOOLEAN hasTail;
} FILE_PARSING_DATA;

typedef struct _COMPRESSED_SECTION_PARSING_DATA {
    UINT32 uncompressedSize;
    UINT8 compressionType;
    UINT8 algorithm;
} COMPRESSED_SECTION_PARSING_DATA;

typedef struct _GUIDED_SECTION_PARSING_DATA {
    EFI_GUID guid;
    UINT32 attributes;
} GUIDED_SECTION_PARSING_DATA;

typedef struct _FREEFORM_GUIDED_SECTION_PARSING_DATA {
    EFI_GUID guid;
} FREEFORM_GUIDED_SECTION_PARSING_DATA;

typedef struct _SECTION_PARSING_DATA {
    union {
        COMPRESSED_SECTION_PARSING_DATA compressed;
        GUIDED_SECTION_PARSING_DATA guidDefined;
        FREEFORM_GUIDED_SECTION_PARSING_DATA freeformSubtypeGuid;
    };
} SECTION_PARSING_DATA;

typedef struct _PARSING_DATA {
    BOOLEAN fixed;
    BOOLEAN isOnFlash;
    UINT8   emptyByte;
    UINT8   ffsVersion;
    UINT32  offset;
    UINT64  address;
    union {
        //CAPSULE_PARSING_DATA capsule;
        //IMAGE_PARSING_DATA image;
        //PADDING_PARSING_DATA padding;
        VOLUME_PARSING_DATA volume;
        //FREE_SPACE_PARSING_DATA freeSpace;
        FILE_PARSING_DATA file;
        SECTION_PARSING_DATA section;
    };
} PARSING_DATA;

class FfsParser : public QObject
{
    Q_OBJECT

public:
    // Default constructor and destructor
    FfsParser(QObject *parent = 0);
    ~FfsParser(void);

    // Returns model for Qt view classes
    TreeModel* treeModel() const;

#ifndef _CONSOLE
    // Returns message items queue
    QQueue<MessageListItem> messages() const;
    // Clears message items queue
    void clearMessages();
#endif

    // Firmware image parsing
    STATUS parseImageFile(const QByteArray & imageFile);
    STATUS parseRawArea(const QByteArray & bios, const QModelIndex & index);
    STATUS parseVolumeHeader(const QByteArray & volume, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseVolumeBody(const QModelIndex & index);
    STATUS parseFileHeader(const QByteArray & file, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseFileBody(const QModelIndex & index);
    STATUS parseSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseSectionBody(const QModelIndex & index);

    // Search routines TODO: move to another class
    STATUS findHexPattern(const QModelIndex & index, const QByteArray & hexPattern, const UINT8 mode);
    STATUS findGuidPattern(const QModelIndex & index, const QByteArray & guidPattern, const UINT8 mode);
    STATUS findTextPattern(const QModelIndex & index, const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive);

    STATUS extract(const QModelIndex & index, QString & name, QByteArray & extracted, const UINT8 mode);

private:
    TreeModel *model;

    STATUS parseIntelImage(const QByteArray & intelImage, const QModelIndex & parent, QModelIndex & index);
    STATUS parseGbeRegion(const QByteArray & gbe, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseMeRegion(const QByteArray & me, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseBiosRegion(const QByteArray & bios, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parsePdrRegion(const QByteArray & pdr, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);

    STATUS parsePadFileBody(const QModelIndex & index);
    STATUS parseSections(QByteArray sections, const QModelIndex & index);

    STATUS parseCommonSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseCompressedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseFreeformGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseVersionSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parsePostcodeSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);

    STATUS parseCompressedSectionBody(const QModelIndex & index);
    STATUS parseGuidedSectionBody(const QModelIndex & index);
    STATUS parseVersionSectionBody(const QModelIndex & index);
    STATUS parseDepexSectionBody(const QModelIndex & index);
    STATUS parseUiSectionBody(const QModelIndex & index);
    STATUS parseRawSectionBody(const QModelIndex & index);

    UINT8 getPaddingType(const QByteArray & padding);
    STATUS parseAprioriRawSection(const QByteArray & body, QString & parsed);
    STATUS findNextVolume(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & nextVolumeOffset);
    STATUS getVolumeSize(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize);
    UINT32 getFileSize(const QByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion);
    UINT32 getSectionSize(const QByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion);

    // Internal operations
    BOOLEAN hasIntersection(const UINT32 begin1, const UINT32 end1, const UINT32 begin2, const UINT32 end2);
    PARSING_DATA getParsingData(const QModelIndex & index = QModelIndex());
    QByteArray convertParsingData(const PARSING_DATA & pdata);

#ifndef _CONSOLE
    QQueue<MessageListItem> messageItems;
#endif
    // Message helper
    void msg(const QString & message, const QModelIndex &index = QModelIndex());

};

#endif
