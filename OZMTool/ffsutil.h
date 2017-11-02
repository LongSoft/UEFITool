/* ffsutil.h

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSUTIL_H
#define FFSUTIL_H

#include "../basetypes.h"
#include "../ffs.h"
#include "../ffsengine.h"

class FFSUtil
{
public:
    FFSUtil();
    ~FFSUtil();
    UINT8 insert(QModelIndex & index, QByteArray & object, UINT8 mode);
    UINT8 replace(QModelIndex & index, QByteArray & object, UINT8 mode);
    UINT8 extract(QModelIndex & index, QByteArray & extracted, UINT8 mode);
    UINT8 compress(QByteArray & data, UINT8 algorithm, QByteArray & compressedData);
    UINT8 decompress(QByteArray & compressed, UINT8 compressionType, QByteArray & decompressedData, UINT8 *algorithm);
    UINT8 remove(QModelIndex & index);
    UINT8 reconstructImageFile(QByteArray & out);

    UINT8 getRootIndex(QModelIndex & result);
    UINT8 findFileByGUID(const QModelIndex index, const QString guid, QModelIndex & result);
    UINT8 findSectionByIndex(const QModelIndex index, UINT8 type, QModelIndex & result);
    UINT8 dumpFileByGUID(QString guid, QByteArray & buf, UINT8 mode);
    UINT8 dumpSectionByGUID(QString guid, UINT8 type, QByteArray & buf, UINT8 mode);
    UINT8 getNameByGUID(QString guid, QString & name);
    UINT8 getLastVolumeIndex(QModelIndex & result);
    UINT8 getAmiBoardPE32Index(QModelIndex & result);
    UINT8 getLastSibling(QModelIndex index, QModelIndex &result);
    UINT8 injectDSDT(QByteArray dsdt);
    UINT8 injectFile(QByteArray file);
    UINT8 deleteFilesystemFfs();
    UINT8 compressDXE();
    UINT8 compressFFS(QByteArray ffs, QByteArray & out);
    UINT8 runFreeSomeSpace(int aggressivity);
    UINT8 parseBIOSFile(QByteArray & buf);
private:
    FfsEngine* ffsEngine;
};
#endif // FFSUTIL_H
