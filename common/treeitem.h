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

#include <list>
#include <iterator>

#include "basetypes.h"
#include "ubytearray.h"
#include "ustring.h"

class TreeItem
{
public:
    TreeItem(const UINT32 offset, const UINT8 type, const UINT8 subtype, const UString &name, const UString &text, const UString &info,
        const UByteArray & header, const UByteArray & body, const UByteArray & tail,
        const bool fixed, const bool compressed,
        TreeItem *parent = 0);
    ~TreeItem();                                                               // Non-trivial implementation in CPP file

    // Operations with items
    void appendChild(TreeItem *item) { childItems.push_back(item); }
    void prependChild(TreeItem *item) { childItems.push_front(item); };
    UINT8 insertChildBefore(TreeItem *item, TreeItem *newItem);                // Non-trivial implementation in CPP file
    UINT8 insertChildAfter(TreeItem *item, TreeItem *newItem);                 // Non-trivial implementation in CPP file

    // Model support operations
    TreeItem *child(int row);                                                  // Non-trivial implementation in CPP file
    int childCount() const {return (int)childItems.size(); }
    int columnCount() const { return 5; }
    UString data(int column) const;                                            // Non-trivial implementation in CPP file
    int row() const;                                                           // Non-trivial implementation in CPP file
    TreeItem *parent() { return parentItem; }

    // Getters and setters for item parameters
    UINT32 offset() const { return itemOffset; }
    void setOffset(const UINT32 offset) { itemOffset = offset; }

    UINT8 type() const  { return itemType; }
    void setType(const UINT8 type) { itemType = type; }

    UINT8 subtype() const { return itemSubtype; }
    void setSubtype(const UINT8 subtype) { itemSubtype = subtype; }

    UString name() const  { return itemName; }
    void setName(const UString &text) { itemName = text; }

    UString text() const { return itemText; }
    void setText(const UString &text) { itemText = text; }

    UByteArray header() const { return itemHeader; }
    bool hasEmptyHeader() const { return itemHeader.isEmpty(); }

    UByteArray body() const { return itemBody; };
    bool hasEmptyBody() const { return itemBody.isEmpty(); }

    UByteArray tail() const { return itemTail; };
    bool hasEmptyTail() const { return itemTail.isEmpty(); }

    UString info() const { return itemInfo; }
    void addInfo(const UString &info, const bool append) { if (append) itemInfo += info; else itemInfo = info + itemInfo; }
    void setInfo(const UString &info) { itemInfo = info; }
    
    UINT8 action() const {return itemAction; }
    void setAction(const UINT8 action) { itemAction = action; }

    bool fixed() const { return itemFixed; }
    void setFixed(const bool fixed) { itemFixed = fixed; }

    bool compressed() const { return itemCompressed; }
    void setCompressed(const bool compressed) { itemCompressed = compressed; }

    UByteArray parsingData() const { return itemParsingData; };
    bool hasEmptyParsingData() const { return itemParsingData.isEmpty(); }
    void setParsingData(const UByteArray & pdata) { itemParsingData = pdata; }

    UINT8 marking() const { return itemMarking; }
    void setMarking(const UINT8 marking) { itemMarking = marking; }

private:
    std::list<TreeItem*> childItems;
    UINT32     itemOffset;
    UINT8      itemAction;
    UINT8      itemType;
    UINT8      itemSubtype;
    UINT8      itemMarking;
    UString    itemName;
    UString    itemText;
    UString    itemInfo;
    UByteArray itemHeader;
    UByteArray itemBody;
    UByteArray itemTail;
    bool       itemFixed;
    bool       itemCompressed;
    UByteArray itemParsingData;
    TreeItem*  parentItem;
};

#endif // TREEITEM_H
