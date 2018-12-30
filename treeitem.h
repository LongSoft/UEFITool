/* treeitem.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __TREEITEM_H__
#define __TREEITEM_H__

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVariant>

#include "basetypes.h"

class TreeItem
{
public:
    TreeItem(const UINT8 type, const UINT8 subtype = 0, const UINT8 compression = COMPRESSION_ALGORITHM_NONE,
        const QString &name = QString(), const QString &text = QString(), const QString &info = QString(),
        const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(),
        TreeItem *parent = 0);
    ~TreeItem();

    // Operations with items
    void appendChild(TreeItem *item);
    void prependChild(TreeItem *item);
    UINT8 insertChildBefore(TreeItem *item, TreeItem *newItem);
    UINT8 insertChildAfter(TreeItem *item, TreeItem *newItem);

    // Model support operations
    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parent();

    // Reading operations for item parameters
    QString name() const;
    void setName(const QString &text);

    UINT8 type() const;
    void setType(const UINT8 type);

    UINT8 subtype() const;
    void setSubtype(const UINT8 subtype);

    QString text() const;
    void setText(const QString &text);

    QByteArray header() const;
    bool hasEmptyHeader() const;

    QByteArray body() const;
    bool hasEmptyBody() const;

    QString info() const;
    void addInfo(const QString &info);
    void setInfo(const QString &info);
    
    UINT8 action() const;
    void setAction(const UINT8 action);

    UINT8 compression() const;

    UINT32 dictionarySize() const;
    void setDictionarySize(const UINT32 dictionarySize);

private:
    QList<TreeItem*> childItems;
    UINT8      itemAction;
    UINT8      itemType;
    UINT8      itemSubtype;
    UINT8      itemCompression;
    UINT32     itemDictionarySize;
    QString    itemName;
    QString    itemText;
    QString    itemInfo;
    QByteArray itemHeader;
    QByteArray itemBody;
    TreeItem *parentItem;
};

#endif
