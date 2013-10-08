/* treemodel.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#ifndef __TREEMODEL_H__
#define __TREEMODEL_H__

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include "basetypes.h"
#include "treeitemtypes.h"

class TreeItem;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(QObject *parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    
    QModelIndex addItem(UINT8 type, UINT8 subtype = 0, const QByteArray &header = QByteArray(), const QByteArray &body = QByteArray(), const QModelIndex &parent = QModelIndex());
    bool removeItem(const QModelIndex &index);

    QByteArray header(const QModelIndex& index);
    bool hasEmptyHeader(const QModelIndex& index);
    QByteArray body(const QModelIndex& index);
    bool hasEmptyBody(const QModelIndex& index);

private:
    QModelIndex findParentOfType(UINT8 type, const QModelIndex& index);
    bool setItemName(const QString &data, const QModelIndex &index);
    bool setItemText(const QString &data, const QModelIndex &index);
    TreeItem *rootItem;
};

#endif
