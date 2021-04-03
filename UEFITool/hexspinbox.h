/* hexspinbox.h

  Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef HEXSPINBOX_H
#define HEXSPINBOX_H

#include <QSpinBox>

#if QT_VERSION_MAJOR >= 6
#include <QRegularExpressionValidator>
#else
#include <QRegExpValidator>
#endif

class HexSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    HexSpinBox(QWidget *parent = 0);

protected:
    QValidator::State validate(QString &text, int &pos) const;
    int valueFromText(const QString &text) const;
    QString textFromValue(int value) const;

private:
#if QT_VERSION_MAJOR >= 6
    QRegularExpressionValidator validator;
#else
    QRegExpValidator validator;
#endif
};

#endif // HEXSPINBOX_H
