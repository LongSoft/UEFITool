/* fssops.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSOPS_H
#define FFSOPS_H

#include <vector>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QModelIndex>

#include "basetypes.h"
#include "treemodel.h"
#include "ffs.h"
#include "utility.h"

class FfsOperations
{
public:
    explicit FfsOperations(TreeModel * treeModel);
    ~FfsOperations();

    std::vector<std::pair<QString, QModelIndex> > getMessages() const;
    void clearMessages();

    STATUS extract(const QModelIndex & index, QString & name, QByteArray & extracted, const UINT8 mode);
    STATUS replace(const QModelIndex & index, const QString & data, const UINT8 mode);
    
    STATUS remove(const QModelIndex & index);
    STATUS rebuild(const QModelIndex & index);

private:
    TreeModel* model;
    std::vector<std::pair<QString, QModelIndex> > messagesVector;

    void msg(const QString & message, const QModelIndex &index = QModelIndex());
};

#endif // FFSOPS_H
