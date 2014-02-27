/* treemodel.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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
#include "types.h"

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

    QString nameString(const QModelIndex &index) const;
    QString actionString(const QModelIndex &index) const;
    QString typeString(const QModelIndex &index) const;
    QString subtypeString(const QModelIndex &index) const;
    QString textString(const QModelIndex &index) const;

    void setAction(const QModelIndex &index, const UINT8 action);
    void setTypeString(const QModelIndex &index, const QString &text);
    void setSubtypeString(const QModelIndex &index, const QString &text);
    void setNameString(const QModelIndex &index, const QString &text);
    void setTextString(const QModelIndex &index, const QString &text);

    void setSubtype(const QModelIndex & index, UINT8 subtype);

    UINT8 type(const QModelIndex &index) const;
    UINT8 subtype(const QModelIndex &index) const;
    QByteArray header(const QModelIndex &index) const;
    bool hasEmptyHeader(const QModelIndex &index) const;
    QByteArray body(const QModelIndex &index) const;
    bool hasEmptyBody(const QModelIndex &index) const;
    QByteArray tail(const QModelIndex &index) const;
    bool hasEmptyTail(const QModelIndex &index) const;
    QString info(const QModelIndex &index) const;
    UINT8 action(const QModelIndex &index) const;
    UINT8 compression(const QModelIndex &index) const;

    QModelIndex addItem(const UINT8 type, const UINT8 subtype = 0, const UINT8 compression = COMPRESSION_ALGORITHM_NONE,
                        const QString & name = QString(), const QString & text = QString(), const QString & info = QString(),
                        const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(), const QByteArray & tail = QByteArray(),
                        const QModelIndex & parent = QModelIndex(), const UINT8 mode = CREATE_MODE_APPEND);

    QModelIndex findParentOfType(const QModelIndex & index, UINT8 type) const;

private:
    TreeItem *rootItem;
};

#endif
