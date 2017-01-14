/* ustring.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef USTRING_H
#define USTRING_H

#if defined (QT_CORE_LIB)
// Use Qt class, if Qt is available
#include <QString>
#define UString QString
#define findreplace replace
#else
// Use Bstrlib
#define BSTRLIB_DOESNT_THROW_EXCEPTIONS
#include "bstrlib/bstrwrap.h"
#define UString CBString
#endif // QT_CORE_LIB

UString usprintf(const char* fmt, ...);
UString urepeated(char c, int len);

#endif // USTRING_H
