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
    UINT8 remove(QModelIndex & index);
    UINT8 reconstructImageFile(QByteArray & out);
    QModelIndex getRootIndex();
    UINT8 findFileByGUID(const QModelIndex index, const QString guid, QModelIndex & result);
    UINT8 findSectionByIndex(const QModelIndex index, UINT8 type, QModelIndex & result);
    UINT8 dumpFileByGUID(QString guid, QByteArray & buf, UINT8 mode);
    UINT8 dumpSectionByGUID(QString guid, UINT8 type, QByteArray & buf, UINT8 mode);
    UINT8 getLastSibling(QModelIndex index, QModelIndex &result);
    UINT8 parseBIOSFile(QByteArray & buf);
private:
    FfsEngine* ffsEngine;
};
#endif // FFSUTIL_H
