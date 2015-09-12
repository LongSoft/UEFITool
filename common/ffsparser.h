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
#include <QVector>

#include "basetypes.h"
#include "treemodel.h"
#include "utility.h"
#include "peimage.h"
#include "parsingdata.h"

class TreeModel;

class FfsParser : public QObject
{
    Q_OBJECT

public:
    // Default constructor and destructor
    FfsParser(TreeModel* treeModel, QObject *parent = 0);
    ~FfsParser();

    // Returns messages 
    QVector<QPair<QString, QModelIndex> > getMessages() const;
    // Clears messages
    void clearMessages();

    // Firmware image parsing
    STATUS parseImageFile(const QByteArray & imageFile, const QModelIndex & index);
    STATUS parseRawArea(const QByteArray & data, const QModelIndex & index);
    STATUS parseVolumeHeader(const QByteArray & volume, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseVolumeBody(const QModelIndex & index);
    STATUS parseFileHeader(const QByteArray & file, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseFileBody(const QModelIndex & index);
    STATUS parseSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseSectionBody(const QModelIndex & index);

    // Retuns index of the last VTF after parsing is done
    const QModelIndex getLastVtf() {return lastVtf;};

private:
    TreeModel *model;
    QVector<QPair<QString, QModelIndex> > messagesVector;
    QModelIndex lastVtf;

    STATUS parseIntelImage(const QByteArray & intelImage, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & root);
    STATUS parseGbeRegion(const QByteArray & gbe, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseMeRegion(const QByteArray & me, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseBiosRegion(const QByteArray & bios, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parsePdrRegion(const QByteArray & pdr, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);
    STATUS parseEcRegion(const QByteArray & ec, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index);

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
    STATUS parsePeImageSectionBody(const QModelIndex & index);
    STATUS parseTeImageSectionBody(const QModelIndex & index);

    UINT8  getPaddingType(const QByteArray & padding);
    STATUS parseAprioriRawSection(const QByteArray & body, QString & parsed);
    STATUS findNextVolume(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & nextVolumeOffset);
    STATUS getVolumeSize(const QByteArray & bios, const UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize);
    UINT32 getFileSize(const QByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion);
    UINT32 getSectionSize(const QByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion);

    STATUS performSecondPass(const QModelIndex & index);
    STATUS addMemoryAddressesRecursive(const QModelIndex & index, const UINT32 diff);
    /*STATUS parseFit(const QModelIndex & index);
    STATUS findFitRecursive(const QModelIndex & index, QModelIndex & found);*/

    // Internal operations
    BOOLEAN hasIntersection(const UINT32 begin1, const UINT32 end1, const UINT32 begin2, const UINT32 end2);

    // Message helper
    void msg(const QString & message, const QModelIndex &index = QModelIndex());

};

#endif
