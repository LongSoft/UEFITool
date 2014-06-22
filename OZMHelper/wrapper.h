#ifndef WRAPPER_H
#define WRAPPER_H

#include "../basetypes.h"
#include "../ffsengine.h"

struct kextEntry {
    QString binaryPath;
    QString plistPath;
    QString basename;
    QString GUID;
    QString filename;
};

struct sectionEntry {
    QString name;
    QString GUID;
    BOOLEAN required;
};

class Wrapper
{
public:
    Wrapper();
    ~Wrapper();
    /* Generic stuff */
    UINT8 fileOpen(QString path, QByteArray & buf);
    UINT8 fileWrite(QString path, QByteArray & buf);
    BOOLEAN fileExists(QString path);
    UINT8 dirCreate(QString path);
    BOOLEAN dirExists(QString path);
    QString pathConcatenate(QString path, QString filename);
    UINT32 getDateTime();
    UINT16 getUInt16(QByteArray & buf, UINT32 start, bool fromBE);
    UINT32 getUInt32(QByteArray & buf, UINT32 start, bool fromBE);
    /* Specific stuff */
    UINT8 findFileByGUID(const QModelIndex index, const QString guid, QModelIndex & result);
    UINT8 findSectionByIndex(const QModelIndex index, UINT8 type, QModelIndex & result);
    UINT8 dumpFileByGUID(QString guid, QByteArray & buf, UINT8 mode);
    UINT8 dumpSectionByGUID(QString guid, UINT8 type, QByteArray & buf, UINT8 mode);
    UINT8 getLastSibling(QString guid, QModelIndex &result);
    UINT8 parseBIOSFile(QByteArray & buf);
    UINT8 getDSDTfromAMI(QByteArray in, QByteArray & out);
    UINT8 dsdt2bios(QByteArray amiboardinfo, QByteArray dsdt, QByteArray & out);
    UINT8 getInfoFromPlist(QByteArray plist, QString & name, QByteArray & out);
    UINT8 parseKextDirectory(QString input, QList<kextEntry> & kextList);
    UINT8 convertKexts(kextEntry entry, QByteArray & out);

private:
    FfsEngine* ffsEngine;
};

#endif // WRAPPER_H
