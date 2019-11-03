/* uefipatch.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __UEFIPATCH_H__
#define __UEFIPATCH_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QUuid>

#include "../basetypes.h"
#include "../ffs.h"
#include "../ffsengine.h"

class UEFIPatch : public QObject
{
    Q_OBJECT

public:
    explicit UEFIPatch(QObject *parent = 0);
    ~UEFIPatch();

    UINT8 patchFromFile(const QString & path, const QString & patches, const QString & outputPath);
    UINT8 patchFromArg(const QString & path, const QString & patch, const QString & outputPath);
private:
    UINT8 patchFile(const QModelIndex & index, const QByteArray & fileGuid, const UINT8 sectionType, const QVector<PatchData> & patches);
    FfsEngine* ffsEngine;
    TreeModel* model;
};

#endif
