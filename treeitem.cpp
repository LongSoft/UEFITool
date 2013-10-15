/* treeitem.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#include "treeitem.h"

TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, const UINT32 offset, const QString & name, const QString & typeName, const QString & subtypeName, 
                   const QString & text, const QString & info, const QByteArray & header, const QByteArray & body, TreeItem *parent)
{
    
    itemType = type;
    itemSubtype = subtype;
    itemOffset = offset;
    itemName = name;
    itemTypeName = typeName;
    itemSubtypeName = subtypeName;
    itemText = text;
    itemInfo = info;
    itemHeader = header;
    itemBody = body;
    parentItem = parent;
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    childItems.append(item);
}

void TreeItem::removeChild(TreeItem *item)
{
    childItems.removeAll(item);
}

TreeItem *TreeItem::child(int row)
{
    return childItems.value(row);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::columnCount() const
{
    return 4;
}

QString TreeItem::data(int column) const
{
    switch(column)
    {
    case 0: //Object
        return itemName;
        break;
    case 1: //Type
        return itemTypeName;
        break;
    case 2: //Subtype
        return itemSubtypeName;
        break;
    case 3: //Text
        return itemText;
        break;
    default:
        return "";
    }
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

void TreeItem::setName(const QString &text)
{
    itemName = text;
}

void TreeItem::setText(const QString &text)
{
    itemText = text;
}

void TreeItem::setTypeName(const QString &text)
{
    itemTypeName = text;
}

void TreeItem::setSubtypeName(const QString &text)
{
    itemSubtypeName = text;
}

QString TreeItem::info()
{
    return itemInfo;
}

void TreeItem::setInfo(const QString &text)
{
    itemInfo = text;
}

int TreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

UINT8 TreeItem::type()
{
    return itemType;
}

UINT8 TreeItem::subtype()
{
    return itemSubtype;
}

UINT32 TreeItem::offset()
{
    return itemOffset;
}

QByteArray TreeItem::header()
{
    return itemHeader;
}

QByteArray TreeItem::body()
{
    return itemBody;
}

bool TreeItem::hasEmptyHeader()
{
    return itemHeader.isEmpty();
}

bool TreeItem::hasEmptyBody()
{
    return itemBody.isEmpty();
}