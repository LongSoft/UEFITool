/* hexlineedit.h

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef HEXLINEEDIT_H
#define HEXLINEEDIT_H

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>

#include "../common/basetypes.h"

class HexLineEdit : public QLineEdit
{
     Q_OBJECT
     Q_PROPERTY(bool editAsGuid READ editAsGuid WRITE setEditAsGuid)

public:
    HexLineEdit(QWidget * parent = 0);
    HexLineEdit(const QString & contents, QWidget * parent = 0);
    ~HexLineEdit();

    void setEditAsGuid(bool editAsGuid)
    {
        m_editAsGuid = editAsGuid;
    }
    bool editAsGuid() const
    { return m_editAsGuid; }

private:
    bool m_editAsGuid;

protected:
    void keyPressEvent(QKeyEvent * event);

};

#endif // HEXLINEEDIT_H
