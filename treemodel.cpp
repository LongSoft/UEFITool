/* treemodel.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "treeitem.h"
#include "treemodel.h"

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem(Types::Root);
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::UserRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole)
        return item->data(index.column());
    else
        return item->info();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
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

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    if (childItem == rootItem)
        return QModelIndex();

    TreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
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

UINT8 TreeModel::type(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->type();
}

UINT8 TreeModel::subtype(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->subtype();
}

QByteArray TreeModel::header(const QModelIndex &index) const
{
    if(!index.isValid())
        return QByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->header();
}

bool TreeModel::hasEmptyHeader(const QModelIndex &index) const
{
    if(!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyHeader();
}

QByteArray TreeModel::body(const QModelIndex &index) const
{
    if(!index.isValid())
        return QByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->body();
}

bool TreeModel::hasEmptyBody(const QModelIndex &index) const
{
    if(!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyBody();
}

QByteArray TreeModel::tail(const QModelIndex &index) const
{
    if(!index.isValid())
        return QByteArray();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->tail();
}

bool TreeModel::hasEmptyTail(const QModelIndex &index) const
{
    if(!index.isValid())
        return true;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->hasEmptyTail();
}

QString TreeModel::info(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->info();
}

UINT8 TreeModel::action(const QModelIndex &index) const
{
    if(!index.isValid())
        return Actions::NoAction;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->action();
}

UINT8 TreeModel::compression(const QModelIndex &index) const
{
    if(!index.isValid())
        return COMPRESSION_ALGORITHM_UNKNOWN;
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->compression();
}

void TreeModel::setSubtype(const QModelIndex & index, UINT8 subtype)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setSubtype(subtype);
    emit dataChanged(index, index);
}

void TreeModel::setNameString(const QModelIndex &index, const QString &data)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setName(data);
    emit dataChanged(index, index);
}

void TreeModel::setTypeString(const QModelIndex &index, const QString &data)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setTypeName(data);
    emit dataChanged(index, index);
}

void TreeModel::setSubtypeString(const QModelIndex &index, const QString &data)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setSubtypeName(data);
    emit dataChanged(index, index);
}

void TreeModel::setTextString(const QModelIndex &index, const QString &data)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setText(data);
    emit dataChanged(index, index);
}

QString TreeModel::nameString(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->data(0).toString();
}

QString TreeModel::actionString(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->data(1).toString();}

QString TreeModel::typeString(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->data(2).toString();
}

QString TreeModel::subtypeString(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->data(3).toString();
}

QString TreeModel::textString(const QModelIndex &index) const
{
    if(!index.isValid())
        return QString();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->data(4).toString();
}


void TreeModel::setAction(const QModelIndex &index, const UINT8 action)
{
    if(!index.isValid())
        return;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setAction(action);
    emit dataChanged(this->index(0,0), index);
}

QModelIndex TreeModel::addItem(const UINT8 type, const UINT8 subtype, const UINT8 compression,
                               const QString & name, const QString & text, const QString & info,
                               const QByteArray & header, const QByteArray & body, const QByteArray & tail,
                               const QModelIndex & parent, const UINT8 mode)
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

    TreeItem *newItem = new TreeItem(type, subtype, compression, name, text, info, header, body, tail, parentItem);
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
        return QModelIndex();
    }

    emit layoutChanged();

    return createIndex(newItem->row(), parentColumn, newItem);
}

QModelIndex TreeModel::findParentOfType(const QModelIndex& index, UINT8 type) const
{
    if(!index.isValid())
        return QModelIndex();

    TreeItem *item;
    QModelIndex parent = index;

    for(item = static_cast<TreeItem*>(parent.internalPointer());
        item != NULL && item != rootItem && item->type() != type;
        item = static_cast<TreeItem*>(parent.internalPointer()))
        parent = parent.parent();
    if (item != NULL && item != rootItem)
        return parent;

    return QModelIndex();
}
