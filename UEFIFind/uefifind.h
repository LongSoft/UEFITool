/* uefifind.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
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

#include "../common/basetypes.h"
#include "../common/ffsparser.h"
#include "../common/ffs.h"

class UEFIFind : public QObject
{
    Q_OBJECT

public:
    explicit UEFIFind(QObject *parent = 0);
    ~UEFIFind();

    STATUS init(const QString & path);
    STATUS find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result);

private:
    STATUS findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, QSet<QPair<QModelIndex, QModelIndex> > & files);
    QString guidToQString(const UINT8* guid);

    FfsParser* ffsParser;
    TreeModel* model;
    QFileInfo fileInfo;
    bool initDone;
};

#endif
