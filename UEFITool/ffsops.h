/* fssops.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __FFSOPS_H__
#define __FFSOPS_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QModelIndex>

#include "..\common\basetypes.h"
#include "..\common\treemodel.h"
#include "..\common\ffs.h"
#include "..\common\utility.h"

class FfsOperations : public QObject
{
    Q_OBJECT

public:
    explicit FfsOperations(const TreeModel * treeModel, QObject *parent = 0);
    ~FfsOperations();

    QVector<QPair<QString, QModelIndex> > getMessages() const;
    void clearMessages();

    STATUS extract(const QModelIndex & index, QString & name, QByteArray & extracted, const UINT8 mode);

private:
    const TreeModel* model;
    QVector<QPair<QString, QModelIndex> > messagesVector;

    void msg(const QString & message, const QModelIndex &index = QModelIndex());
};

#endif
