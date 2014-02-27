/* treeitem.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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
             const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(), const QByteArray & tail = QByteArray(),
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
    UINT8 type() const;
    UINT8 subtype() const;
    QByteArray header() const;
    bool hasEmptyHeader() const;
    QByteArray body() const;
    bool hasEmptyBody() const;
    QByteArray tail() const;
    bool hasEmptyTail() const;
    QString info() const;
    UINT8 action() const;
    UINT8 compression() const;

    // Some values can be changed after item construction
    void setAction(const UINT8 action);
    void setSubtype(const UINT8 subtype);
    void setTypeName(const QString &text);
    void setSubtypeName(const QString &text);
    void setName(const QString &text);
    void setText(const QString &text);

private:
    // Set default names after construction
    // They can later be changed by set* methods
    void setDefaultNames();

    QList<TreeItem*> childItems;
    UINT8 itemAction;
    UINT8 itemType;
    UINT8 itemSubtype;
    UINT8 itemCompression;
    QByteArray itemHeader;
    QByteArray itemBody;
    QByteArray itemTail;
    QString itemName;
    QString itemTypeName;
    QString itemSubtypeName;
    QString itemText;
    QString itemInfo;
    TreeItem *parentItem;
};

#endif
