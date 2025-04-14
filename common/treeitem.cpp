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
                   const UINT32 headerSize, const UINT32 bodySize, const UINT32 tailSize,
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
itemHeaderSize(headerSize),
itemBodySize(bodySize),
itemTailSize(tailSize),
itemFixed(fixed),
itemCompressed(compressed),
parentItem(parent)
{
}

TreeItem::~TreeItem() {
    auto begin = childItems.begin();
    while (begin != childItems.end()) {
        delete *begin;
        ++begin;
    }
}

const char * TreeItem::content(UINT32 dataOffset) const
{
    if (!itemContent.isEmpty()) {
        return itemContent.constData() + dataOffset;
    }
    auto o = itemOffset;
    auto p = parentItem;
    while (p && ((!p->itemCompressed && p->itemContent.isEmpty()) || (p->itemCompressed && p->itemUncompressedData.isEmpty()))) {
        o += p->itemOffset;
        p = p->parentItem;
    }
    if (!p)
        return nullptr;
    return !p->itemCompressed ? p->content(o + dataOffset) : p->itemUncompressedData.constData() + o + dataOffset - p->itemHeaderSize;
}

bool TreeItem::compressedInherited() const {
    auto p = this;
    while (p && p->itemType != Types::Root && !p->itemCompressed)
        p = p->parentItem;
    return p ? p->itemCompressed : false;
}

UINT8 TreeItem::insertChildBefore(TreeItem *item, TreeItem *newItem)
{
    auto found = std::find(childItems.begin(), childItems.end(), item);
    if (found == childItems.end())
        return U_ITEM_NOT_FOUND;
    childItems.insert(found, newItem);
    return U_SUCCESS;
}

UINT8 TreeItem::insertChildAfter(TreeItem *item, TreeItem *newItem)
{
    auto found = std::find(childItems.begin(), childItems.end(), item);
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
        auto iter = parentItem->childItems.begin();
        for (int i = 0; i < (int)parentItem->childItems.size(); ++i, ++iter) {
            if (const_cast<TreeItem*>(this) == *iter)
                return i;
        }
    }
    return 0;
}

TreeItem* TreeItem::child(int row)
{
    auto child = childItems.begin();
    std::advance(child, row);
    return *child;
}
