/* hexspinbox.cpp

  Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include "hexspinbox.h"
#include <QDebug>

HexSpinBox::HexSpinBox(QWidget *parent) :
QSpinBox(parent), validator(QRegExp("0x([0-9a-fA-F]){1,8}"))
{
    this->setRange(INT_MIN, INT_MAX);
    this->setPrefix("0x");
}

QValidator::State HexSpinBox::validate(QString &text, int &pos) const
{
    return validator.validate(text, pos);
}

QString HexSpinBox::textFromValue(int val) const
{
    return QString::number((uint)val, 16).toUpper();
}

int HexSpinBox::valueFromText(const QString &text) const
{
    return (int)text.toUInt(NULL, 16);
}
