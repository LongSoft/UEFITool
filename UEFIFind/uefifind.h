/* uefifind.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef UEFIFIND_H
#define UEFIFIND_H

#include <iterator>
#include <set>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QUuid>

#include "../common/basetypes.h"
#include "../common/ffsparser.h"
#include "../common/ffs.h"

class UEFIFind
{
public:
    explicit UEFIFind();
    ~UEFIFind();

    USTATUS init(const QString & path);
    USTATUS find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result);

private:
    USTATUS findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, std::set<std::pair<QModelIndex, QModelIndex> > & files);
    QString guidToQString(const UINT8* guid);

    FfsParser* ffsParser;
    TreeModel* model;
    QFileInfo fileInfo;
    bool initDone;
};

#endif // UEFIFIND_H
