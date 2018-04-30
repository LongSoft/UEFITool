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

TreeItem::TreeItem(const UINT32 offset, const UINT8 type, const UINT8 subtype, 
    const UString & name, const UString & text, const UString & info,
    const UByteArray & header, const UByteArray & body, const UByteArray & tail,
    const bool fixed, const bool compressed,
    TreeItem *parent) : 
    itemOffset(offset),
    itemAction(Actions::NoAction),
    itemType(type),
    itemSubtype(subtype),
    itemMarking(0),
    itemName(name),
    itemText(text),
    itemInfo(info),
    itemHeader(header),
    itemBody(body),
    itemTail(tail),
    itemFixed(fixed),
    itemCompressed(compressed),
    parentItem(parent)
{
}

TreeItem::~TreeItem() {
    std::list<TreeItem*>::iterator begin = childItems.begin();
    while (begin != childItems.end()) {
        delete *begin;
        ++begin;
    }
}

UINT8 TreeItem::insertChildBefore(TreeItem *item, TreeItem *newItem)
{
    std::list<TreeItem*>::iterator found = std::find(childItems.begin(), childItems.end(), item);
    if (found == childItems.end())
        return U_ITEM_NOT_FOUND;
    childItems.insert(found, newItem);
    return U_SUCCESS;
}

UINT8 TreeItem::insertChildAfter(TreeItem *item, TreeItem *newItem)
{
    std::list<TreeItem*>::iterator found = std::find(childItems.begin(), childItems.end(), item);
    if (found == childItems.end())
        return U_ITEM_NOT_FOUND;
    childItems.insert(++found, newItem);
    return U_SUCCESS;
}

UString TreeItem::data(int column) const
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
        return UString();
    }
}

int TreeItem::row() const
{
    if (parentItem) {
        std::list<TreeItem*>::const_iterator iter = parentItem->childItems.begin();
        for (int i = 0; i < (int)parentItem->childItems.size(); ++i, ++iter) {
            if (const_cast<TreeItem*>(this) == *iter)
                return i;
        }
    }
    return 0;
}

TreeItem* TreeItem::child(int row)
{ 
    std::list<TreeItem*>::iterator child = childItems.begin();  
    std::advance(child, row); 
    return *child; 
}