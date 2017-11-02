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

TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, const UINT8 compression,
    const QString & name, const QString & text, const QString & info,
    const QByteArray & header, const QByteArray & body,
    TreeItem *parent) : 
    itemAction(Actions::NoAction),
    itemType(type),
    itemSubtype(subtype),
    itemCompression(compression),
    itemName(name),
    itemText(text),
    itemInfo(info),
    itemHeader(header),
    itemBody(body),
    parentItem(parent)
{
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
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

TreeItem *TreeItem::parent()
{
    return parentItem;
}

QString TreeItem::name() const
{
    return itemName;
}

void TreeItem::setName(const QString &name)
{
    itemName = name;
}

QString TreeItem::text() const
{
    return itemText;
}

void TreeItem::setText(const QString &text)
{
    itemText = text;
}

QString TreeItem::info() const
{
    return itemInfo;
}

void TreeItem::addInfo(const QString &info)
{
    itemInfo += info;
}

void TreeItem::setInfo(const QString &info)
{
    itemInfo = info;
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

void TreeItem::setType(const UINT8 type)
{
    itemType = type;
}

UINT8 TreeItem::subtype() const
{
    return itemSubtype;
}

void TreeItem::setSubtype(const UINT8 subtype)
{
    itemSubtype = subtype;
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

bool TreeItem::hasEmptyHeader() const
{
    return itemHeader.isEmpty();
}

bool TreeItem::hasEmptyBody() const
{
    return itemBody.isEmpty();
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
        for (int i = 0; i < childCount(); i++)
            child(i)->setAction(Actions::Insert);

    // Set rebuild action for parent, if it has no action now
    if (parentItem && parentItem->type() != Types::Root
        && parentItem->action() == Actions::NoAction)
        parentItem->setAction(Actions::Rebuild);
}

