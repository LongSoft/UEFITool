/* ustring.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "ustring.h"
#include <stdarg.h>

UString usprintf(const char* fmt, ...) 
{
    UString msg;
    va_list vl;
    va_start(vl, fmt);
    msg = msg.vsprintf(fmt, vl);
    va_end(vl);
    return msg;
};

