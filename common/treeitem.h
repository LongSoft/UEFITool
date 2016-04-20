/* treeitem.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVariant>

#include "basetypes.h"

class TreeItem
{
public:
    TreeItem(const UINT8 type, const UINT8 subtype, const QString &name, const QString &text, const QString &info,
        const QByteArray & header, const QByteArray & body, const QByteArray & tail,
        const BOOLEAN fixed, const BOOLEAN compressed, const QByteArray & parsingData,
        TreeItem *parent = 0);
    ~TreeItem() { qDeleteAll(childItems); }

    // Operations with items
    void appendChild(TreeItem *item) { childItems.append(item); }
    void prependChild(TreeItem *item) { childItems.prepend(item); };
    UINT8 insertChildBefore(TreeItem *item, TreeItem *newItem);                // Non-trivial implementation in CPP file
    UINT8 insertChildAfter(TreeItem *item, TreeItem *newItem);                 // Non-trivial implementation in CPP file

    // Model support operations
    TreeItem *child(int row) { return childItems.value(row, NULL); }
    int childCount() const {return childItems.count(); }
    int columnCount() const { return 5; }
    QVariant data(int column) const;                                           // Non-trivial implementation in CPP file
    int row() const;                                                           // Non-trivial implementation in CPP file
    TreeItem *parent() { return parentItem; }

    // Reading operations for item parameters
    QString name() const  { return itemName; }
    void setName(const QString &text) { itemName = text; }

    UINT8 type() const  { return itemType; }
    void setType(const UINT8 type) { itemType = type; }

    UINT8 subtype() const { return itemSubtype; }
    void setSubtype(const UINT8 subtype) { itemSubtype = subtype; }

    QString text() const { return itemText; }
    void setText(const QString &text) { itemText = text; }

    QByteArray header() const { return itemHeader; }
    bool hasEmptyHeader() const { return itemHeader.isEmpty(); }

    QByteArray body() const { return itemBody; };
    bool hasEmptyBody() const { return itemBody.isEmpty(); }

    QByteArray tail() const { return itemTail; };
    bool hasEmptyTail() const { return itemTail.isEmpty(); }

    QByteArray parsingData() const { return itemParsingData; }
    bool hasEmptyParsingData() const { return itemParsingData.isEmpty(); }
    void setParsingData(const QByteArray & data) { itemParsingData = data; }

    QString info() const { return itemInfo; }
    void addInfo(const QString &info, const BOOLEAN append) { if (append) itemInfo.append(info); else itemInfo.prepend(info); }
    void setInfo(const QString &info) { itemInfo = info; }
    
    UINT8 action() const {return itemAction; }
    void setAction(const UINT8 action) { itemAction = action; }

    BOOLEAN fixed() const { return itemFixed; }
    void setFixed(const bool fixed) { itemFixed = fixed; }

    BOOLEAN compressed() const { return itemCompressed; }
    void setCompressed(const bool compressed) { itemCompressed = compressed; }

private:
    QList<TreeItem*> childItems;
    UINT8      itemAction;
    UINT8      itemType;
    UINT8      itemSubtype;
    QString    itemName;
    QString    itemText;
    QString    itemInfo;
    QByteArray itemHeader;
    QByteArray itemBody;
    QByteArray itemTail;
    QByteArray itemParsingData;
    bool       itemFixed;
    bool       itemCompressed;
    TreeItem*  parentItem;
};

#endif // TREEITEM_H
