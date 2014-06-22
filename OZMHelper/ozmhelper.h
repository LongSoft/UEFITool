/* ozmhelper.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __OZMHELPER_H__
#define __OZMHELPER_H__

#include "wrapper.h"
#include "../basetypes.h"

class OZMHelper : public QObject
{
    Q_OBJECT

public:
    explicit OZMHelper(QObject *parent = 0);
    ~OZMHelper();

    UINT8 DSDTExtract(QString input, QString output);
    UINT8 OZMExtract(QString input, QString output);
    UINT8 OZMCreate(QString input, QString output, QString inputFFS, QString inputKext, QString inputDSDT);
    UINT8 FFSConvert(QString input, QString output);
    UINT8 DSDT2Bios(QString input, QString inputDSDT, QString output);
    UINT8 Test(QString input);

private:
    Wrapper* wrapper;
};

#endif
