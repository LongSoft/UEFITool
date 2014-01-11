/* messagelistitem.h

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __MESSAGELISTITEM_H__
#define __MESSAGELISTITEM_H__

#include <QModelIndex>
#include <QListWidgetItem>

#include "basetypes.h"

class MessageListItem : public QListWidgetItem
{
public:
    MessageListItem(QListWidget * parent = 0, int type = Type, const QModelIndex & index = QModelIndex());
    MessageListItem(const QString & text, QListWidget * parent = 0, int type = Type, const QModelIndex & index = QModelIndex());
    MessageListItem(const QIcon & icon, const QString & text, QListWidget * parent = 0, int type = Type, const QModelIndex & index = QModelIndex());
    ~MessageListItem();
    
    QModelIndex index() const;
    void setIndex(QModelIndex & index);

private:
    QModelIndex itemIndex;
};

#endif
