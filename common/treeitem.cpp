/* treeitem.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "treeitem.h"
#include "types.h"

TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, 
    const UString & name, const UString & text, const UString & info,
    const UByteArray & header, const UByteArray & body, const UByteArray & tail,
    const BOOLEAN fixed, const BOOLEAN compressed, const UByteArray & parsingData,
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
        return U_ITEM_NOT_FOUND;
    childItems.insert(index, newItem);
    return U_SUCCESS;
}

UINT8 TreeItem::insertChildAfter(TreeItem *item, TreeItem *newItem)
{
    int index = childItems.indexOf(item);
    if (index == -1)
        return U_ITEM_NOT_FOUND;
    childItems.insert(index + 1, newItem);
    return U_SUCCESS;
}

QVariant TreeItem::data(int column) const
{
    switch (column)
    {
    case 0: // Name
        return itemName;
    case 1: // Action
        return actionTypeToUString(itemAction);
    case 2: // Type
        return itemTypeToUString(itemType);
    case 3: // Subtype
        return itemSubtypeToUString(itemType, itemSubtype);
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
