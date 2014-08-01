/* ozmtool.h

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __OZMTOOL_H__
#define __OZMTOOL_H__

#include "util.h"
#include "../basetypes.h"

class OZMTool : public QObject
{
    Q_OBJECT

public:
    explicit OZMTool(QObject *parent = 0);

    UINT8 DSDTExtract(QString inputfile, QString outputdir);
    UINT8 DSDTInject(QString inputfile, QString dsdtfile, QString outputfile);
    UINT8 OZMUpdate(QString inputfile, QString recentBios, QString outputfile, int aggressivity, bool compressdxe);
    UINT8 OZMExtract(QString inputfile, QString outputdir);
    UINT8 OZMCreate(QString inputfile, QString outputfile, QString inputFFSdir, QString inputKextdir, QString inputEFIdir, QString inputDSDTfile, int aggressivity, bool compressdxe, bool compresskexts);
    UINT8 Kext2Ffs(QString inputdir, QString outputdir);
    UINT8 DSDT2Bios(QString inputfile, QString inputDSDTfile, QString outputfile);
private:
};

#endif
