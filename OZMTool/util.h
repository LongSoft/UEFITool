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
#include "common.h"

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
UINT8 plistReadExecName(QByteArray plist, QString & name);
UINT8 plistReadBundleVersion(QByteArray plist, QString & version);
UINT8 checkAggressivityLevel(int aggressivity);
UINT8 convertBinary(QString input, QString guid, QString sectionName, QByteArray & out);
UINT8 convertKext(QString input, QString guid, QString basename, QByteArray & out);
UINT8 freeformCreate(QByteArray binary, QString guid, QString sectionName, QByteArray & fileOut);
UINT8 extractDSDTfromAmiboardInfo(QByteArray amiboardbuf, QByteArray & out);
UINT8 injectDSDTintoAmiboardInfo(QByteArray ami, QByteArray dsdtbuf, QByteArray & out);

#endif // UTIL_H
