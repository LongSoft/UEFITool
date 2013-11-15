/* debuglistitem.cpp

  Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "debuglistitem.h"

DebugListItem::DebugListItem(QListWidget * parent, int type, const QModelIndex & index)
    : QListWidgetItem(parent, type)
{
    itemIndex = index;
}

DebugListItem::DebugListItem(const QString & text, QListWidget * parent, int type, const QModelIndex & index)
    : QListWidgetItem(text, parent, type)
{
    itemIndex = index;
}

DebugListItem::DebugListItem(const QIcon & icon, const QString & text, QListWidget * parent, int type, const QModelIndex & index)
    : QListWidgetItem(icon, text, parent, type)
{
    itemIndex = index;
}

DebugListItem::~DebugListItem()
{

}

QModelIndex DebugListItem::index() const
{
    return itemIndex;
}

void DebugListItem::setIndex(QModelIndex & index)
{
    itemIndex = index;
}