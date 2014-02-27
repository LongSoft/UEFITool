/* treeitem.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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




TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, const UINT8 compression,
                   const QString & name, const QString & text, const QString & info,
                   const QByteArray & header, const QByteArray & body, const QByteArray & tail,
                   TreeItem *parent)
{
    itemAction = Actions::NoAction;
    itemType = type;
    itemSubtype = subtype;
    itemCompression = compression;
    itemName = name;
    itemText = text;
    itemInfo = info;
    itemHeader = header;
    itemBody = body;
    itemTail = tail;
    parentItem = parent;

    // Set default names
    setDefaultNames();
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

void TreeItem::setDefaultNames()
{
    itemTypeName = itemTypeToQString(itemType);
    itemSubtypeName = itemSubtypeToQString(itemType, itemSubtype);
}

void TreeItem::appendChild(TreeItem *item)
{
    childItems.append(item);
}

void TreeItem::prependChild(TreeItem *item)
{
    childItems.prepend(item);
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

TreeItem *TreeItem::child(int row)
{
    return childItems.value(row, NULL);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::columnCount() const
{
    return 5;
}

QVariant TreeItem::data(int column) const
{
    switch(column)
    {
    case 0: //Name
        return itemName;
    case 1: //Action
        return actionTypeToQString(itemAction);
    case 2: //Type
        return itemTypeName;
    case 3: //Subtype
        return itemSubtypeName;
    case 4: //Text
        return itemText;
    default:
        return QVariant();
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

QString TreeItem::info() const
{
    return itemInfo;
}

int TreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

UINT8 TreeItem::type() const
{
    return itemType;
}

UINT8 TreeItem::subtype() const
{
    return itemSubtype;
}

UINT8 TreeItem::compression() const
{
    return itemCompression;
}

QByteArray TreeItem::header() const
{
    return itemHeader;
}

QByteArray TreeItem::body() const
{
    return itemBody;
}

QByteArray TreeItem::tail() const
{
    return itemTail;
}

bool TreeItem::hasEmptyHeader() const
{
    return itemHeader.isEmpty();
}

bool TreeItem::hasEmptyBody() const
{
    return itemBody.isEmpty();
}

bool TreeItem::hasEmptyTail() const
{
    return itemTail.isEmpty();
}

UINT8 TreeItem::action() const
{
    return itemAction;
}

void TreeItem::setAction(const UINT8 action)
{
    itemAction = action;

    // On insert action, set insert action for children
    if (action == Actions::Insert)
        for(int i = 0; i < childCount(); i++)
            child(i)->setAction(Actions::Insert);

    // Set rebuild action for parent, if it has no action now
    if (parentItem && parentItem->type() != Types::Root
        && parentItem->action() == Actions::NoAction)
        parentItem->setAction(Actions::Rebuild);
}

void TreeItem::setSubtype(const UINT8 subtype)
{
    itemSubtype = subtype;
    itemSubtypeName = itemSubtypeToQString(itemType, itemSubtype);
}
