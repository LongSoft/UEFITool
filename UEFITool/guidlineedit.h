/* guidlineedit.h

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef GUIDLINEEDIT_H
#define GUIDLINEEDIT_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>

#include "../common/basetypes.h"

class GuidLineEdit : public QLineEdit
{
public:
    GuidLineEdit(QWidget * parent = 0);
    GuidLineEdit(const QString & contents, QWidget * parent = 0);
    ~GuidLineEdit();

protected:
    void keyPressEvent(QKeyEvent * event);

};

#endif // GUIDLINEEDIT_H
