/* treemodel.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef TREEMODEL_H
#define TREEMODEL_H

enum ItemFixedState {
    Movable,
    Fixed
};

#if defined(QT_CORE_LIB)
// Use Qt classes
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QObject>
#if defined(QT_GUI_LIB)
#include <QBrush>
#endif

#include "ustring.h"
#include "ubytearray.h"
#include "basetypes.h"
#include "types.h"
#include "treeitem.h"

#define UModelIndex QModelIndex
#else
// Use own implementation 
#include "ustring.h"
#include "ubytearray.h"
#include "basetypes.h"
#include "types.h"
#include "treeitem.h"

class TreeModel;

class UModelIndex
{
    friend class TreeModel;

public:
    inline UModelIndex() : r(-1), c(-1), i(0), m(0) {}
    // compiler-generated copy/move ctors/assignment operators are fine!
    inline int row() const { return r; }
    inline int column() const { return c; }
    inline uint64_t internalId() const { return i; }
    inline void *internalPointer() const { return reinterpret_cast<void*>(i); }
    inline UModelIndex parent() const;
    inline UModelIndex child(int row, int column) const;
    inline CBString data(int role) const;
    inline const TreeModel *model() const { return m; }
    inline bool isValid() const { return (r >= 0) && (c >= 0) && (m != 0); }
    inline bool operator==(const UModelIndex &other) const { return (other.r == r) && (other.i == i) && (other.c == c) && (other.m == m); }
    inline bool operator!=(const UModelIndex &other) const { return !(*this == other); }
    inline bool operator<(const UModelIndex &other) const
    {
        return  r <  other.r
            || (r == other.r && (c <  other.c
            || (c == other.c && (i <  other.i
            || (i == other.i && m < other.m)))));
    }

private:
    inline UModelIndex(int arow, int acolumn, void *ptr, const TreeModel *amodel)
        : r(arow), c(acolumn), i(reinterpret_cast<uint64_t>(ptr)), m(amodel) {}
    inline UModelIndex(int arow, int acolumn, uint64_t id, const TreeModel *amodel)
        : r(arow), c(acolumn), i(id), m(amodel) {}
    int r, c;
    uint64_t i;
    const TreeModel *m;
};
#endif

#if defined(QT_CORE_LIB)
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    TreeItem *rootItem;
    bool markingEnabledFlag;

public:
    QVariant data(const UModelIndex &index, int role) const;
    Qt::ItemFlags flags(const UModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    TreeModel(QObject *parent = 0) : QAbstractItemModel(parent), markingEnabledFlag(true) {
        rootItem = new TreeItem(0, Types::Root, 0, UString(), UString(), UString(), UByteArray(), UByteArray(), UByteArray(), true, false);
    }

#else
#define emit

class TreeModel
{
private:
    TreeItem *rootItem;
    bool markingEnabledFlag;

    void dataChanged(const UModelIndex &, const UModelIndex &) {}
    void layoutAboutToBeChanged() {}
    void layoutChanged() {}

public:
    UString data(const UModelIndex &index, int role) const;
    UString headerData(int section, int orientation, int role = 0) const;

    TreeModel() : markingEnabledFlag(false) {
        rootItem = new TreeItem(0, Types::Root, 0, UString(), UString(), UString(), UByteArray(), UByteArray(), UByteArray(), TRUE, FALSE);
    }

    bool hasIndex(int row, int column, const UModelIndex &parent = UModelIndex()) const {
        if (row < 0 || column < 0)
            return false;
        return row < rowCount(parent) && column < columnCount(parent);
    }

    UModelIndex createIndex(int row, int column, void *data) const { return UModelIndex(row, column, data, this); }
#endif

    ~TreeModel() {
        delete rootItem;
    }

    bool markingEnabled() { return markingEnabledFlag; }
    void setMarkingEnabled(const bool enabled);

    UModelIndex index(int row, int column, const UModelIndex &parent = UModelIndex()) const;
    UModelIndex parent(const UModelIndex &index) const;
    int rowCount(const UModelIndex &parent = UModelIndex()) const;
    int columnCount(const UModelIndex &parent = UModelIndex()) const;

    UINT8 action(const UModelIndex &index) const;
    void setAction(const UModelIndex &index, const UINT8 action);

    UINT32 base(const UModelIndex &index) const;
    UINT32 offset(const UModelIndex &index) const;
    void setOffset(const UModelIndex &index, const UINT32 offset);

    UINT8 type(const UModelIndex &index) const;
    void setType(const UModelIndex &index, const UINT8 type);

    UINT8 subtype(const UModelIndex &index) const;
    void setSubtype(const UModelIndex &index, const UINT8 subtype);

    UString name(const UModelIndex &index) const;
    void setName(const UModelIndex &index, const UString &name);

    UString text(const UModelIndex &index) const;
    void setText(const UModelIndex &index, const UString &text);

    UString info(const UModelIndex &index) const;
    void setInfo(const UModelIndex &index, const UString &info);
    void addInfo(const UModelIndex &index, const UString &info, const bool append = TRUE);

    bool fixed(const UModelIndex &index) const;
    void setFixed(const UModelIndex &index, const bool fixed);

    bool compressed(const UModelIndex &index) const;
    void setCompressed(const UModelIndex &index, const bool compressed);

    UINT8 marking(const UModelIndex &index) const;
    void setMarking(const UModelIndex &index, const UINT8 marking);

    UByteArray header(const UModelIndex &index) const;
    bool hasEmptyHeader(const UModelIndex &index) const;

    UByteArray body(const UModelIndex &index) const;
    bool hasEmptyBody(const UModelIndex &index) const;

    UByteArray tail(const UModelIndex &index) const;
    bool hasEmptyTail(const UModelIndex &index) const;

    UByteArray parsingData(const UModelIndex &index) const;
    bool hasEmptyParsingData(const UModelIndex &index) const;
    void setParsingData(const UModelIndex &index, const UByteArray &pdata);

    UModelIndex addItem(const UINT32 offset, const UINT8 type, const UINT8 subtype,
        const UString & name, const UString & text, const UString & info,
        const UByteArray & header, const UByteArray & body, const UByteArray & tail,
        const ItemFixedState fixed,
        const UModelIndex & parent = UModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);

    UModelIndex findParentOfType(const UModelIndex & index, UINT8 type) const;
    UModelIndex findLastParentOfType(const UModelIndex & index, UINT8 type) const;
    UModelIndex findByBase(UINT32 base) const;
};

#if defined(QT_CORE_LIB)
// Nothing required here
#else
inline UModelIndex UModelIndex::parent() const { return m ? m->parent(*this) : UModelIndex(); }
inline UModelIndex UModelIndex::child(int row, int column) const { return m ? m->index(row, column, *this) : UModelIndex(); }
inline UString UModelIndex::data(int role) const { return m ? m->data(*this, role) : UString(); }
#endif

#endif // TREEMODEL_H
