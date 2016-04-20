/* treeitem.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QObject>
#include "treeitem.h"
#include "types.h"

TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, 
    const QString & name, const QString & text, const QString & info,
    const QByteArray & header, const QByteArray & body, const QByteArray & tail,
    const BOOLEAN fixed, const BOOLEAN compressed, const QByteArray & parsingData,
    TreeItem *parent) : 
    itemAction(Actions::NoAction),
    itemType(type),
    itemSubtype(subtype),
    itemName(name),
    itemText(text),
    itemInfo(info),
    itemHeader(header),
    itemBody(body),
    itemTail(tail),
    itemParsingData(parsingData),
    itemFixed(fixed),
    itemCompressed(compressed),
    parentItem(parent)
{
    setFixed(fixed);
}

UINT8 TreeItem::insertChildBefore(TreeItem *item, TreeItem *newItem)
{
    int index = childItems.indexOf(item);
    if (index == -1)
        return ERR_ITEM_NOT_FOUND;
    childItems.insert(index, newItem);
    return ERR_SUCCESS;
}

UINT8 TreeItem::insertChildAfter(TreeItem *item, TreeItem *newItem)
{
    int index = childItems.indexOf(item);
    if (index == -1)
        return ERR_ITEM_NOT_FOUND;
    childItems.insert(index + 1, newItem);
    return ERR_SUCCESS;
}

QVariant TreeItem::data(int column) const
{
    switch (column)
    {
    case 0: // Name
        return itemName;
    case 1: // Action
        return actionTypeToQString(itemAction);
    case 2: // Type
        return itemTypeToQString(itemType);
    case 3: // Subtype
        return itemSubtypeToQString(itemType, itemSubtype);
    case 4: // Text
        return itemText;
    default:
        return QVariant();
    }
}

int TreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}
