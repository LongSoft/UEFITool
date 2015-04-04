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
#include <QFileInfo>

#include "../common/basetypes.h"
#include "../common/treemodel.h"

class FfsDumper : public QObject
{
    Q_OBJECT

public:
    explicit FfsDumper(TreeModel * treeModel, QObject *parent = 0);
    ~FfsDumper();
	
    STATUS dump(const QModelIndex & root, const QString & path);
	
private:
    STATUS recursiveDump(const QModelIndex & root, const QString & path);
    TreeModel* model;
    bool dumped;
};

#endif
