/* util.h

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef UTIL_H
#define UTIL_H

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
UINT8 getGUIDfromFile(QByteArray object, QString & name);
UINT8 dsdt2bios(QByteArray amiboardinfo, QByteArray dsdt, QByteArray & out);
UINT8 getInfoFromPlist(QByteArray plist, QString & name, QByteArray & out);
UINT8 parseKextDirectory(QString input, QList<kextEntry> & kextList);
UINT8 convertKexts(kextEntry entry, QByteArray & out);

#endif // UTIL_H
