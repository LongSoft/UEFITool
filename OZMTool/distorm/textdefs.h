/*
textdefs.h

diStorm3 - Powerful disassembler for X86/AMD64
http://ragestorm.net/distorm/
distorm at gmail dot com
Copyright (C) 2003-2012 Gil Dabah

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/


#ifndef TEXTDEFS_H
#define TEXTDEFS_H

#include "config.h"
#include "wstring.h"

#ifndef DISTORM_LIGHT

#define PLUS_DISP_CHR '+'
#define MINUS_DISP_CHR '-'
#define OPEN_CHR '['
#define CLOSE_CHR ']'
#define SP_CHR ' '
#define SEG_OFF_CHR ':'

/*
Naming Convention:

* get - returns a pointer to a string.
* str - concatenates to string.

* hex - means the function is used for hex dump (number is padded to required size) - Little Endian output.
* code - means the function is used for disassembled instruction - Big Endian output.
* off - means the function is used for 64bit offset - Big Endian output.

* h - '0x' in front of the string.

* b - byte
* dw - double word (can be used for word also)
* qw - quad word

* all numbers are in HEX.
*/

extern int8_t TextBTable[256][4];

void _FASTCALL_ str_hex_b(_WString* s, unsigned int x);
void _FASTCALL_ str_code_hb(_WString* s, unsigned int x);
void _FASTCALL_ str_code_hdw(_WString* s, uint32_t x);
void _FASTCALL_ str_code_hqw(_WString* s, uint8_t src[8]);

#ifdef SUPPORT_64BIT_OFFSET
void _FASTCALL_ str_off64(_WString* s, OFFSET_INTEGER x);
#endif

#endif /* DISTORM_LIGHT */

#endif /* TEXTDEFS_H */
