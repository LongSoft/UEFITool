/* treemodel.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QVariant>
#include <QObject>

#include "ustring.h"
#include "ubytearray.h"
#include "basetypes.h"
#include "types.h"
#include "umodelindex.h"

class TreeItem;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(QObject *parent = 0);
    ~TreeModel();

    QVariant data(const UModelIndex &index, int role) const;
    Qt::ItemFlags flags(const UModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    UModelIndex index(int row, int column,
        const UModelIndex &parent = UModelIndex()) const;
    UModelIndex parent(const UModelIndex &index) const;
    int rowCount(const UModelIndex &parent = UModelIndex()) const;
    int columnCount(const UModelIndex &parent = UModelIndex()) const;

    void setAction(const UModelIndex &index, const UINT8 action);
    void setType(const UModelIndex &index, const UINT8 type);
    void setSubtype(const UModelIndex &index, const UINT8 subtype);
    void setName(const UModelIndex &index, const UString &name);
    void setText(const UModelIndex &index, const UString &text);
    void setInfo(const UModelIndex &index, const UString &info);
    void addInfo(const UModelIndex &index, const UString &info, const bool append = TRUE);
    void setParsingData(const UModelIndex &index, const UByteArray &data);
    void setFixed(const UModelIndex &index, const bool fixed);
    void setCompressed(const UModelIndex &index, const bool compressed);
    
    UString name(const UModelIndex &index) const;
    UString text(const UModelIndex &index) const;
    UString info(const UModelIndex &index) const;
    UINT8 type(const UModelIndex &index) const;
    UINT8 subtype(const UModelIndex &index) const;
    UByteArray header(const UModelIndex &index) const;
    bool hasEmptyHeader(const UModelIndex &index) const;
    UByteArray body(const UModelIndex &index) const;
    bool hasEmptyBody(const UModelIndex &index) const;
    UByteArray tail(const UModelIndex &index) const;
    bool hasEmptyTail(const UModelIndex &index) const;
    UByteArray parsingData(const UModelIndex &index) const;
    bool hasEmptyParsingData(const UModelIndex &index) const;
    UINT8 action(const UModelIndex &index) const;
    bool fixed(const UModelIndex &index) const;
    
    bool compressed(const UModelIndex &index) const;

    UModelIndex addItem(const UINT8 type, const UINT8 subtype,
        const UString & name, const UString & text, const UString & info,
        const UByteArray & header, const UByteArray & body, const UByteArray & tail,
        const bool fixed, const UByteArray & parsingData = UByteArray(),
        const UModelIndex & parent = UModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);

    UModelIndex findParentOfType(const UModelIndex & index, UINT8 type) const;

private:
    TreeItem *rootItem;
};

#endif // TREEMODEL_H
