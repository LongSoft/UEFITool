/* fssbuilder.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSBUILDER_H
#define FFSBUILDER_H

#include <vector>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QModelIndex>

#include "basetypes.h"
#include "treemodel.h"
#include "descriptor.h"
#include "ffs.h"
#include "utility.h"

class FfsBuilder
{
public:
    explicit FfsBuilder(const TreeModel * treeModel);
    ~FfsBuilder();

    std::vector<std::pair<QString, QModelIndex> > getMessages() const;
    void clearMessages();

    STATUS build(const QModelIndex & root, QByteArray & image);

private:
    const TreeModel* model;
    std::vector<std::pair<QString, QModelIndex> > messagesVector;
    void msg(const QString & message, const QModelIndex &index = QModelIndex());

    STATUS buildCapsule(const QModelIndex & index, QByteArray & capsule);
    STATUS buildIntelImage(const QModelIndex & index, QByteArray & intelImage);
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

#endif // FFSBUILDER_H
