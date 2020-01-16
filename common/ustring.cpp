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

#if defined(QT_CORE_LIB)
UString usprintf(const char* fmt, ...) 
{
    UString msg;
    va_list vl;
    va_start(vl, fmt);
    msg.vsprintf(fmt, vl);
    va_end(vl);
    return msg;
};

UString urepeated(char c, int len)
{
    return UString(len, c);
}
#else
/* Give WATCOM C/C++, MSVC some latitude for their non-support of vsnprintf */
#if defined(__WATCOMC__) || defined(_MSC_VER)
#define exvsnprintf(r,b,n,f,a) {r = _vsnprintf (b,n,f,a);}
#else
#ifdef BSTRLIB_NOVSNP
/* This is just a hack.  If you are using a system without a vsnprintf, it is
not recommended that bformat be used at all. */
#define exvsnprintf(r,b,n,f,a) {vsprintf (b,f,a); r = -1;}
#define START_VSNBUFF (256)
#else

#if defined (__GNUC__) && !defined (__PPC__) && !defined(__WIN32__)
/* Something is making gcc complain about this prototype not being here, so
I've just gone ahead and put it in. */
extern "C" {
    extern int vsnprintf(char *buf, size_t count, const char *format, va_list arg);
}
#endif

#define exvsnprintf(r,b,n,f,a) {r = vsnprintf (b,n,f,a);}
#endif
#endif

#ifndef START_VSNBUFF
#define START_VSNBUFF (16)
#endif

UString usprintf(const char* fmt, ...)
{
    UString msg;
    bstring b;
    va_list arglist;
    int r, n;

    if (fmt == NULL) {
        msg = "<NULL>";
    }
    else {

        if ((b = bfromcstr("")) == NULL) {
            msg = "<NULL>";
        }
        else {
            if ((n = (int)(2 * (strlen)(fmt))) < START_VSNBUFF) n = START_VSNBUFF;
            for (;;) {
                if (BSTR_OK != balloc(b, n + 2)) {
                    b = bformat("<NULL>");
                    break;
                }

                va_start(arglist, fmt);
                exvsnprintf(r, (char *)b->data, n + 1, fmt, arglist);
                va_end(arglist);

                b->data[n] = '\0';
                b->slen = (int)(strlen)((char *)b->data);

                if (b->slen < n) break;
                if (r > n) n = r; else n += n;
            }
            msg = *b;
            bdestroy(b);
        }
    }

    return msg;
}

UString urepeated(char c, int len)
{
    return UString(c, len);
}
#endif
