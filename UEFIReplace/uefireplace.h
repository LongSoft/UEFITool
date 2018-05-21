/* uefireplace.h
Copyright (c) 2017, mxxxc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __UEFIREPLACE_H__
#define __UEFIREPLACE_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QUuid>

#include "../basetypes.h"
#include "../ffs.h"
#include "../ffsengine.h"

class UEFIReplace : public QObject
{
    Q_OBJECT

public:
    explicit UEFIReplace(QObject *parent = 0);
    ~UEFIReplace();

    UINT8 replace(const QString & inPath, const QByteArray & guid, const UINT8 sectionType, const QString & contentPath, const QString & outPath, bool replaceAsIs, bool replaceOnce);

private:
    UINT8 replaceInFile(const QModelIndex & index, const QByteArray & guid, const UINT8 sectionType, const QByteArray & contents, const UINT8 mode, bool replaceOnce);
    FfsEngine* ffsEngine;
    TreeModel* model;
};

#endif
