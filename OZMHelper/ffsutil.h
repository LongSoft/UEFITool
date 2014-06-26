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
    QModelIndex getRootIndex();
    UINT8 insert(QModelIndex & index, QByteArray & object, UINT8 mode);
    UINT8 replace(QModelIndex & index, QByteArray & object, UINT8 mode);
    UINT8 reconstructImageFile(QByteArray & out);
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
