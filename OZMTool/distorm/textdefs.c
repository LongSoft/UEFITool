/*
textdefs.c

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


#include "textdefs.h"

#ifndef DISTORM_LIGHT

static uint8_t Nibble2ChrTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
#define NIBBLE_TO_CHR Nibble2ChrTable[t]

void _FASTCALL_ str_hex_b(_WString* s, unsigned int x)
{
	/*
	 * def prebuilt():
	 * 	s = ""
	 * 	for i in xrange(256):
	 * 		if ((i % 0x10) == 0):
	 * 			s += "\r\n"
	 * 		s += "\"%02x\", " % (i)
	 * 	return s
	 */
	static int8_t TextBTable[256][3] = {
		"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
		"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
		"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
		"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
		"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
		"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
		"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
		"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
		"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
		"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
		"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
		"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
		"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
		"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
		"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
		"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
	};

	/*
	 * Fixed length of 3 including null terminate character.
	 */
	memcpy(&s->p[s->length], TextBTable[x & 255], 3);
	s->length += 2;
}

void _FASTCALL_ str_code_hb(_WString* s, unsigned int x)
{
	static int8_t TextHBTable[256][5] = {
	/*
	 * def prebuilt():
	 * 	s = ""
	 * 	for i in xrange(256):
	 * 		if ((i % 0x10) == 0):
	 * 			s += "\r\n"
	 * 		s += "\"0x%x\", " % (i)
	 * 	return s
	 */
		"0x0", "0x1", "0x2", "0x3", "0x4", "0x5", "0x6", "0x7", "0x8", "0x9", "0xa", "0xb", "0xc", "0xd", "0xe", "0xf",
		"0x10", "0x11", "0x12", "0x13", "0x14", "0x15", "0x16", "0x17", "0x18", "0x19", "0x1a", "0x1b", "0x1c", "0x1d", "0x1e", "0x1f",
		"0x20", "0x21", "0x22", "0x23", "0x24", "0x25", "0x26", "0x27", "0x28", "0x29", "0x2a", "0x2b", "0x2c", "0x2d", "0x2e", "0x2f",
		"0x30", "0x31", "0x32", "0x33", "0x34", "0x35", "0x36", "0x37", "0x38", "0x39", "0x3a", "0x3b", "0x3c", "0x3d", "0x3e", "0x3f",
		"0x40", "0x41", "0x42", "0x43", "0x44", "0x45", "0x46", "0x47", "0x48", "0x49", "0x4a", "0x4b", "0x4c", "0x4d", "0x4e", "0x4f",
		"0x50", "0x51", "0x52", "0x53", "0x54", "0x55", "0x56", "0x57", "0x58", "0x59", "0x5a", "0x5b", "0x5c", "0x5d", "0x5e", "0x5f",
		"0x60", "0x61", "0x62", "0x63", "0x64", "0x65", "0x66", "0x67", "0x68", "0x69", "0x6a", "0x6b", "0x6c", "0x6d", "0x6e", "0x6f",
		"0x70", "0x71", "0x72", "0x73", "0x74", "0x75", "0x76", "0x77", "0x78", "0x79", "0x7a", "0x7b", "0x7c", "0x7d", "0x7e", "0x7f",
		"0x80", "0x81", "0x82", "0x83", "0x84", "0x85", "0x86", "0x87", "0x88", "0x89", "0x8a", "0x8b", "0x8c", "0x8d", "0x8e", "0x8f",
		"0x90", "0x91", "0x92", "0x93", "0x94", "0x95", "0x96", "0x97", "0x98", "0x99", "0x9a", "0x9b", "0x9c", "0x9d", "0x9e", "0x9f",
		"0xa0", "0xa1", "0xa2", "0xa3", "0xa4", "0xa5", "0xa6", "0xa7", "0xa8", "0xa9", "0xaa", "0xab", "0xac", "0xad", "0xae", "0xaf",
		"0xb0", "0xb1", "0xb2", "0xb3", "0xb4", "0xb5", "0xb6", "0xb7", "0xb8", "0xb9", "0xba", "0xbb", "0xbc", "0xbd", "0xbe", "0xbf",
		"0xc0", "0xc1", "0xc2", "0xc3", "0xc4", "0xc5", "0xc6", "0xc7", "0xc8", "0xc9", "0xca", "0xcb", "0xcc", "0xcd", "0xce", "0xcf",
		"0xd0", "0xd1", "0xd2", "0xd3", "0xd4", "0xd5", "0xd6", "0xd7", "0xd8", "0xd9", "0xda", "0xdb", "0xdc", "0xdd", "0xde", "0xdf",
		"0xe0", "0xe1", "0xe2", "0xe3", "0xe4", "0xe5", "0xe6", "0xe7", "0xe8", "0xe9", "0xea", "0xeb", "0xec", "0xed", "0xee", "0xef",
		"0xf0", "0xf1", "0xf2", "0xf3", "0xf4", "0xf5", "0xf6", "0xf7", "0xf8", "0xf9", "0xfa", "0xfb", "0xfc", "0xfd", "0xfe", "0xff"
	};

	if (x < 0x10) {	/* < 0x10 has a fixed length of 4 including null terminate. */
		memcpy(&s->p[s->length], TextHBTable[x & 255], 4);
		s->length += 3;
	} else { /* >= 0x10 has a fixed length of 5 including null terminate. */
		memcpy(&s->p[s->length], TextHBTable[x & 255], 5);
		s->length += 4;
	}
}

void _FASTCALL_ str_code_hdw(_WString* s, uint32_t x)
{
	int8_t* buf;
	int i = 0, shift = 0;
	unsigned int t = 0;

	buf = (int8_t*)&s->p[s->length];

	buf[0] = '0';
	buf[1] = 'x';
	buf += 2;

	for (shift = 28; shift != 0; shift -= 4) {
		t = (x >> shift) & 0xf;
		if (i | t) buf[i++] = NIBBLE_TO_CHR;
	}
	t = x & 0xf;
	buf[i++] = NIBBLE_TO_CHR;

	s->length += i + 2;
	buf[i] = '\0';
}

void _FASTCALL_ str_code_hqw(_WString* s, uint8_t src[8])
{
	int8_t* buf;
	int i = 0, shift = 0;
	uint32_t x = RULONG(&src[sizeof(int32_t)]);
	int t;

	buf = (int8_t*)&s->p[s->length];
	buf[0] = '0';
	buf[1] = 'x';
	buf += 2;

	for (shift = 28; shift != -4; shift -= 4) {
		t = (x >> shift) & 0xf;
		if (i | t) buf[i++] = NIBBLE_TO_CHR;
	}

	x = RULONG(src);
	for (shift = 28; shift != 0; shift -= 4) {
		t = (x >> shift) & 0xf;
		if (i | t) buf[i++] = NIBBLE_TO_CHR;
	}
	t = x & 0xf;
	buf[i++] = NIBBLE_TO_CHR;

	s->length += i + 2;
	buf[i] = '\0';
}

#ifdef SUPPORT_64BIT_OFFSET
void _FASTCALL_ str_off64(_WString* s, OFFSET_INTEGER x)
{
	int8_t* buf;
	int i = 0, shift = 0;
	OFFSET_INTEGER t = 0;

	buf = (int8_t*)&s->p[s->length];

	buf[0] = '0';
	buf[1] = 'x';
	buf += 2;

	for (shift = 60; shift != 0; shift -= 4) {
		t = (x >> shift) & 0xf;
		if (i | t) buf[i++] = NIBBLE_TO_CHR;
	}
	t = x & 0xf;
	buf[i++] = NIBBLE_TO_CHR;

	s->length += i + 2;
	buf[i] = '\0';
}
#endif /* SUPPORT_64BIT_OFFSET */

#endif /* DISTORM_LIGHT */
