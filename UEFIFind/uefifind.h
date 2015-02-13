/* uefifind.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __UEFIFIND_H__
#define __UEFIFIND_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QSet>
#include <QString>
#include <QUuid>

#include "../basetypes.h"
#include "../ffsengine.h"
#include "../ffs.h"

class UEFIFind : public QObject
{
    Q_OBJECT

public:
    explicit UEFIFind(QObject *parent = 0);
    ~UEFIFind();

    UINT8 init(const QString & path);
    UINT8 find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result);

private:
    UINT8 findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, QSet<QPair<QModelIndex, QModelIndex> > & files);
    QString guidToQString(const UINT8* guid);

    FfsEngine* ffsEngine;
    TreeModel* model;
    QFileInfo fileInfo;
    bool initDone;
};

#endif
