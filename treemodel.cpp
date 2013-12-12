/* treemodel.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#include "treeitem.h"
#include "treemodel.h"

TreeModel::TreeModel(TreeItem *root, QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = root;
}

TreeModel::~TreeModel()
{
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

    return QVariant();
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

UINT8 TreeModel::setItemName(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return ERR_INVALID_PARAMETER;
    
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setName(data);
    emit dataChanged(index, index);
    return ERR_SUCCESS;
}

UINT8 TreeModel::setItemText(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return ERR_INVALID_PARAMETER;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setText(data);
    emit dataChanged(index, index);
    return ERR_SUCCESS;
}

UINT8 TreeModel::setItemAction(const UINT8 action, const QModelIndex &index)
{
    if(!index.isValid())
        return ERR_INVALID_PARAMETER;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setAction(action);
    emit dataChanged(this->index(0,0), index);
    return ERR_SUCCESS;
}

QModelIndex TreeModel::addItem(const UINT8 type, const UINT8 subtype, const UINT8 compression, 
                               const QString & name, const QString & text, const QString & info, 
                               const QByteArray & header, const QByteArray & body, const QByteArray & tail,
                               const QModelIndex & index, const UINT8 mode)
{
    TreeItem *item = 0;
    TreeItem *parentItem = 0;
    int parentColumn = 0;

    if (!index.isValid())
        parentItem = rootItem;
    else
    {
        if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER) {
            item = static_cast<TreeItem*>(index.internalPointer());
            parentItem = item->parent();
            parentColumn = index.parent().column();
        }
        else {
            parentItem = static_cast<TreeItem*>(index.internalPointer());
            parentColumn = index.column();
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
    else 
        return QModelIndex();
    
    emit layoutChanged();
    
    return createIndex(newItem->row(), parentColumn, newItem);
}
