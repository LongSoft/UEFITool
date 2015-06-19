/* fssbuilder.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __FFSBUILDER_H__
#define __FFSBUILDER_H__

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QModelIndex>

#include "../common/basetypes.h"
#include "../common/treemodel.h"
#include "../common/descriptor.h"
#include "../common/ffs.h"
#include "../common/utility.h"

class FfsBuilder : public QObject
{
    Q_OBJECT

public:
    explicit FfsBuilder(const TreeModel * treeModel, QObject *parent = 0);
    ~FfsBuilder();

    QVector<QPair<QString, QModelIndex> > getMessages() const;
    void clearMessages();

    STATUS build(const QModelIndex & root, QByteArray & image);

private:
    const TreeModel* model;
    QVector<QPair<QString, QModelIndex> > messagesVector;
    void msg(const QString & message, const QModelIndex &index = QModelIndex());

    // UEFI standard structures
    STATUS buildCapsule(const QModelIndex & index, QByteArray & capsule);
    STATUS buildIntelImage(const QModelIndex & index, QByteArray & intelImage);
    STATUS buildRegion(const QModelIndex & index, QByteArray & region);
    STATUS buildRawArea(const QModelIndex & index, QByteArray & rawArea, bool addHeader = true);
    STATUS buildPadding(const QModelIndex & index, QByteArray & padding);
    STATUS buildVolume(const QModelIndex & index, QByteArray & volume);
    STATUS buildNonUefiData(const QModelIndex & index, QByteArray & data);
    STATUS buildFreeSpace(const QModelIndex & index, QByteArray & freeSpace);
    STATUS buildPadFile(const QModelIndex & index, QByteArray & padFile);
    STATUS buildFile(const QModelIndex & index, QByteArray & file);
    STATUS buildSection(const QModelIndex & index, QByteArray & section);
    
    // Utility functions
    STATUS erase(const QModelIndex & index, QByteArray & erased);
};

#endif
