/* treemodel.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "treemodel.h"

#if defined(QT_CORE_LIB)
QVariant TreeModel::data(const UModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::UserRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole)
        return (const char*)item->data(index.column()).toLocal8Bit();
    else
        return (const char*)item->info().toLocal8Bit();
}

Qt::ItemFlags TreeModel::flags(const UModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section)
        {
        case 0:
            return tr("Name");
        case 1:
            return tr("Action");
        case 2:
            return tr("Type");
        case 3:
            return tr("Subtype");
        case 4:
            return tr("Text");
        }
    }

    return QVariant();
}
#else
UString TreeModel::data(const UModelIndex &index, int role) const
{
    if (!index.isValid())
        return UString();

    if (role != 0 && role != 0x0100)
        return UString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == 0)
        return item->data(index.column());
    else
        return item->info();
}

UString TreeModel::headerData(int section, int orientation,
    int role) const
{
    if (orientation == 1 && role == 0) {
        switch (section)
        {
        case 0:
            return UString("Name");
        case 1:
            return UString("Action");
        case 2:
            return UString("Type");
        case 3:
            return UString("Subtype");
        case 4:
            return UString("Text");
        }
    }

    return UString();
}
#endif

int TreeModel::columnCount(const UModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

UModelIndex TreeModel::index(int row, int column, const UModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return UModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return UModelIndex();
}

UModelIndex TreeModel::parent(const UModelIndex &index) const
{
    if (!index.isValid())
        return UModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    if (childItem == rootItem)
        return UModelIndex();

    TreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return UModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const UModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

UINT8 TreeModel::type(const UModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->type();
}

UINT8 TreeModel::subtype(const UModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->subtype();
}

UByteArray TreeModel::header(const UModelIndex &index) const
{
    if (!index.isValid())
        return UByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->header();
}

bool TreeModel::hasEmptyHeader(const UModelIndex &index) const
{
    if (!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyHeader();
}

UByteArray TreeModel::body(const UModelIndex &index) const
{
    if (!index.isValid())
        return UByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->body();
}

bool TreeModel::hasEmptyBody(const UModelIndex &index) const
{
    if (!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyBody();
}

UByteArray TreeModel::tail(const UModelIndex &index) const
{
    if (!index.isValid())
        return UByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->tail();
}

bool TreeModel::hasEmptyTail(const UModelIndex &index) const
{
    if (!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyTail();
}

UByteArray TreeModel::parsingData(const UModelIndex &index) const
{
    if (!index.isValid())
        return UByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->parsingData();
}

bool TreeModel::hasEmptyParsingData(const UModelIndex &index) const
{
    if (!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyParsingData();
}

UString TreeModel::name(const UModelIndex &index) const
{
    if (!index.isValid())
        return UString();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->name();
}

UString TreeModel::text(const UModelIndex &index) const
{
    if (!index.isValid())
        return UString();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->text();
}

UString TreeModel::info(const UModelIndex &index) const
{
    if (!index.isValid())
        return UString();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->info();
}

UINT8 TreeModel::action(const UModelIndex &index) const
{
    if (!index.isValid())
        return Actions::NoAction;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->action();
}

bool TreeModel::fixed(const UModelIndex &index) const
{
    if (!index.isValid())
        return false;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->fixed();
}

bool TreeModel::compressed(const UModelIndex &index) const
{
    if (!index.isValid())
        return false;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->compressed();
}

void TreeModel::setFixed(const UModelIndex &index, const bool fixed)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setFixed(fixed);

    if (!item->parent())
        return;

    if (fixed) {
        if (item->compressed() && item->parent()->compressed() == FALSE) {
            item->setFixed(item->parent()->fixed());
            return;
        }

        if (item->parent()->type() != Types::Root)
            item->parent()->setFixed(fixed);
    }

    emit dataChanged(index, index);
}

void TreeModel::setCompressed(const UModelIndex &index, const bool compressed)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setCompressed(compressed);

    emit dataChanged(index, index);
}


void TreeModel::setSubtype(const UModelIndex & index, const UINT8 subtype)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setSubtype(subtype);
    emit dataChanged(index, index);
}

void TreeModel::setName(const UModelIndex &index, const UString &data)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setName(data);
    emit dataChanged(index, index);
}

void TreeModel::setType(const UModelIndex &index, const UINT8 data)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setType(data);
    emit dataChanged(index, index);
}

void TreeModel::setText(const UModelIndex &index, const UString &data)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setText(data);
    emit dataChanged(index, index);
}

void TreeModel::setInfo(const UModelIndex &index, const UString &data)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setInfo(data);
    emit dataChanged(index, index);
}

void TreeModel::addInfo(const UModelIndex &index, const UString &data, const bool append)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->addInfo(data, append);
    emit dataChanged(index, index);
}

void TreeModel::setAction(const UModelIndex &index, const UINT8 action)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setAction(action);
    emit dataChanged(index, index);
}

void TreeModel::setParsingData(const UModelIndex &index, const UByteArray &data)
{
    if (!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setParsingData(data);
    emit dataChanged(this->index(0, 0), index);
}

UModelIndex TreeModel::addItem(const UINT8 type, const UINT8 subtype,
    const UString & name, const UString & text, const UString & info,
    const UByteArray & header, const UByteArray & body, const UByteArray & tail,
    const bool fixed, const UByteArray & parsingData,
    const UModelIndex & parent, const UINT8 mode)
{
    TreeItem *item = 0;
    TreeItem *parentItem = 0;
    int parentColumn = 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
    {
        if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER) {
            item = static_cast<TreeItem*>(parent.internalPointer());
            parentItem = item->parent();
            parentColumn = parent.parent().column();
        }
        else {
            parentItem = static_cast<TreeItem*>(parent.internalPointer());
            parentColumn = parent.column();
        }
    }

    TreeItem *newItem = new TreeItem(type, subtype, name, text, info, header, body, tail, fixed, this->compressed(parent), parsingData, parentItem);
     
    if (mode == CREATE_MODE_APPEND) {
        emit layoutAboutToBeChanged();
        parentItem->appendChild(newItem);
    }
    else if (mode == CREATE_MODE_PREPEND) {
        emit layoutAboutToBeChanged();
        parentItem->prependChild(newItem);
    }
    else if (mode == CREATE_MODE_BEFORE) {
        emit layoutAboutToBeChanged();
        parentItem->insertChildBefore(item, newItem);
    }
    else if (mode == CREATE_MODE_AFTER) {
        emit layoutAboutToBeChanged();
        parentItem->insertChildAfter(item, newItem);
    }
    else {
        delete newItem;
        return UModelIndex();
    }

    emit layoutChanged();

    UModelIndex created = createIndex(newItem->row(), parentColumn, newItem);
    setFixed(created, fixed); // Non-trivial logic requires additional call
    return created;
}

UModelIndex TreeModel::findParentOfType(const UModelIndex& index, UINT8 type) const
{
    if (!index.isValid())
        return UModelIndex();

    TreeItem *item;
    UModelIndex parent = index;

    for (item = static_cast<TreeItem*>(parent.internalPointer());
        item != NULL && item != rootItem && item->type() != type;
        item = static_cast<TreeItem*>(parent.internalPointer()))
            parent = parent.parent();
    if (item != NULL && item != rootItem)
        return parent;

    return UModelIndex();
}