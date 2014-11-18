/* guidlineedit.cpp

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include "guidlineedit.h"

GuidLineEdit::GuidLineEdit(QWidget * parent)
    :QLineEdit(parent)
{
}

GuidLineEdit::GuidLineEdit(const QString & contents, QWidget * parent)
    :QLineEdit(contents, parent)
{
}

GuidLineEdit::~GuidLineEdit()
{
}

void GuidLineEdit::keyPressEvent(QKeyEvent * event)
{
    if (event == QKeySequence::Delete || event->key() == Qt::Key_Backspace)
    {
        int pos = cursorPosition();
        if (event->key() == Qt::Key_Backspace && pos > 0) {
            cursorBackward(false);
            pos = cursorPosition();
        }
        
        QString txt = text();
        QString selected = selectedText();

        if (!selected.isEmpty()) {
            pos = QLineEdit::selectionStart();
            for (int i = pos; i < pos + selected.count(); i++)
                if (txt[i] != QChar('-'))
                    txt[i] = QChar('.');
        }
        else 
            txt[pos] = QChar('.');

        setCursorPosition(0);
        insert(txt);
        setCursorPosition(pos);

        return;
    }

    // Call original event handler
    QLineEdit::keyPressEvent(event);
}