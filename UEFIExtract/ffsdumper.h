/* ffsdumper.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSDUMPER_H
#define FFSDUMPER_H

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
    enum DumpMode {
        DUMP_CURRENT,
        DUMP_ALL,
        DUMP_BODY,
        DUMP_HEADER,
        DUMP_INFO
    };

    static const UINT8 IgnoreSectionType = 0xFF;

    explicit FfsDumper(TreeModel * treeModel) : model(treeModel), dumped(false) {}
    ~FfsDumper() {};

    USTATUS dump(const QModelIndex & root, const QString & path, const DumpMode dumpMode = DUMP_CURRENT, const UINT8 sectionType = IgnoreSectionType, const QString & guid = QString());

private:
    USTATUS recursiveDump(const QModelIndex & root, const QString & path, const DumpMode dumpMode, const UINT8 sectionType, const QString & guid);
    TreeModel* model;
    bool dumped;
};
#endif // FFSDUMPER_H
