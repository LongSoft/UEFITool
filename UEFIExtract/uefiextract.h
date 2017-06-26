/* uefiextract.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __UEFIEXTRACT_H__
#define __UEFIEXTRACT_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QFileInfo>

#include "../basetypes.h"
#include "../ffsengine.h"

class UEFIExtract : public QObject
{
    Q_OBJECT

public:
    explicit UEFIExtract(QObject *parent = 0);
    ~UEFIExtract();

	UINT8 init(const QString & path);
    UINT8 extract(QString path, QString guid = QString());

private:
    FfsEngine* ffsEngine;
	QFileInfo fileInfo;

};

#endif
