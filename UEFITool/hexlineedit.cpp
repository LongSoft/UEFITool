/* hexlineedit.cpp
 
 Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

#include "hexlineedit.h"

#if QT_VERSION_MAJOR >= 6
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

HexLineEdit::HexLineEdit(QWidget * parent)
:QLineEdit(parent)
{
    m_editAsGuid = false;
}

HexLineEdit::HexLineEdit(const QString & contents, QWidget * parent)
:QLineEdit(contents, parent)
{
    m_editAsGuid = false;
}

HexLineEdit::~HexLineEdit()
{
}

void HexLineEdit::keyPressEvent(QKeyEvent * event)
{
    QClipboard *clipboard;
    QString originalText;

    if (m_editAsGuid && (event == QKeySequence::Delete || event->key() == Qt::Key_Backspace))
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
            for (int i = pos; i < pos + selected.length(); i++)
                if (txt[i] != QChar('-'))
                    txt[i] = QChar('.');
        }
        else {
            txt[pos] = QChar('.');
        }
        
        setCursorPosition(0);
        insert(txt);
        setCursorPosition(pos);
        
        return;
    }

    if (event == QKeySequence::Paste)
    {
        clipboard = QApplication::clipboard();
        originalText = clipboard->text();
        QString cleanedHex = QString(originalText).replace(QString("0x"), QString(""), Qt::CaseInsensitive);
#if QT_VERSION_MAJOR >= 6
        cleanedHex.remove(QRegularExpression("[^a-fA-F\\d]+"));
#else
        cleanedHex.remove(QRegExp("[^a-fA-F\\d]+"));
#endif
        clipboard->setText(cleanedHex);
    }
    
    // Call original event handler
    QLineEdit::keyPressEvent(event);
}
