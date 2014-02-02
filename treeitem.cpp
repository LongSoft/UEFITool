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
#include "ffs.h"
#include "descriptor.h"

QString itemTypeToQString(const UINT8 type)
{
    switch (type) {
    case Root:
        return QObject::tr("Root");
    case Image:
        return QObject::tr("Image");
    case Capsule:
        return QObject::tr("Capsule");
    case Region:
        return QObject::tr("Region");
    case Volume:
        return QObject::tr("Volume");
    case Padding:
        return QObject::tr("Padding");
    case File:
        return QObject::tr("File");
    case Section:
        return QObject::tr("Section");
    default:
        return QObject::tr("Unknown");
    }
}

QString itemSubtypeToQString(const UINT8 type, const UINT8 subtype)
{
    switch (type) {
    case Root:
    case Image:
        if (subtype == IntelImage)
            return QObject::tr("Intel");
        else if (subtype == BiosImage)
            return QObject::tr("BIOS");
        else
            return QObject::tr("Unknown");
    case Padding:
        return "";
    case Volume:
        if (subtype == BootVolume)
            return QObject::tr("Boot");
        else if (subtype == UnknownVolume)
            return QObject::tr("Unknown");
        else if (subtype == NvramVolume)
            return QObject::tr("NVRAM");
        else
            return "";
    case Capsule:
        if (subtype == AptioCapsule)
            return QObject::tr("AMI Aptio");
        else if (subtype == UefiCapsule)
            return QObject::tr("UEFI 2.0");
        else
            return QObject::tr("Unknown");
    case Region:
        return regionTypeToQString(subtype);
    case File:
        return fileTypeToQString(subtype);
    case Section:
        return sectionTypeToQString(subtype);
    default:
        return QObject::tr("Unknown");
    }
}

QString compressionTypeToQString(UINT8 algorithm)
{
    switch (algorithm) {
    case COMPRESSION_ALGORITHM_NONE:
        return QObject::tr("None");
    case COMPRESSION_ALGORITHM_EFI11:
        return QObject::tr("EFI 1.1");
    case COMPRESSION_ALGORITHM_TIANO:
        return QObject::tr("Tiano");
    case COMPRESSION_ALGORITHM_LZMA:
        return QObject::tr("LZMA");
    case COMPRESSION_ALGORITHM_IMLZMA:
        return QObject::tr("Intel modified LZMA");
    default:
        return QObject::tr("Unknown");
    }
}

TreeItem::TreeItem(const UINT8 type, const UINT8 subtype, const UINT8 compression,
                   const QString & name, const QString & text, const QString & info,
                   const QByteArray & header, const QByteArray & body, const QByteArray & tail,
                   TreeItem *parent)
{
    itemAction = NoAction;
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
        if (itemAction == Create)
            return QObject::tr("Create");
        if (itemAction == Insert)
            return QObject::tr("Insert");
        if (itemAction == Replace)
            return QObject::tr("Replace");
        if (itemAction == Remove)
            return QObject::tr("Remove");
        if (itemAction == Rebuild)
            return QObject::tr("Rebuild");
        if (itemAction == Rebase)
            return QObject::tr("Rebase");
        return QVariant();
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
    if (action == Insert)
        for(int i = 0; i < childCount(); i++)
            child(i)->setAction(Insert);

    // Set rebuild action for parent, if it has no action now
    if (parentItem && parentItem->type() != Root
            && parentItem->action() == NoAction)
        parentItem->setAction(Rebuild);
}

void TreeItem::setSubtype(const UINT8 subtype)
{
    itemSubtype = subtype;
    itemSubtypeName = itemSubtypeToQString(itemType, itemSubtype);
}
