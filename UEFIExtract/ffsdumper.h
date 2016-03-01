/* ffsdumper.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __FFSDUMPER_H__
#define __FFSDUMPER_H__

#include <QObject>
#include <QDir>
#include <QByteArray>
#include <QString>
#include <QModelIndex>

#include "../common/basetypes.h"
#include "../common/treemodel.h"
#include "../common/ffs.h"

class FfsDumper
{
public:
    explicit FfsDumper(TreeModel * treeModel);
    ~FfsDumper();

    STATUS dump(const QModelIndex & root, const QString & path, const QString & guid = QString());

private:
    STATUS recursiveDump(const QModelIndex & root, const QString & path, const QString & guid);
    TreeModel* model;
    bool dumped;
};

#endif
