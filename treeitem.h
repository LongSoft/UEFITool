/* treeitem.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
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

#include "basetypes.h"

class TreeItem
{
public:
    TreeItem(const UINT8 type, const UINT8 subtype = 0, const QString & name = QString(), const QString & typeName = QString(), const QString & subtypeName = QString(), 
        const QString & text = QString(), const QString & info = QString(), const QByteArray & header = QByteArray(), const QByteArray & body = QByteArray(), TreeItem *parent = 0);
    ~TreeItem();

    void appendChild(TreeItem *item);
    void removeChild(TreeItem *item);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QString data(int column) const;
    int row() const;
    TreeItem *parent();

    UINT8 type();
    UINT8 subtype();
    QByteArray header();
    QByteArray body();
    QString info();
    bool hasEmptyHeader();
    bool hasEmptyBody();

    void setName(const QString &text);
    void setText(const QString &text);
    void setTypeName(const QString &text);
    void setSubtypeName(const QString &text);
    void setInfo(const QString &text);

private:
    QList<TreeItem*> childItems;
    UINT8 itemType;
    UINT8 itemSubtype;
    QByteArray itemHeader;
    QByteArray itemBody;
    QString itemName;
    QString itemTypeName;
    QString itemSubtypeName;
    QString itemText;
    QString itemInfo;
    TreeItem *parentItem;
};

#endif
