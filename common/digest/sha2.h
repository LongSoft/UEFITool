/* sha2.h

Copyright (c) 2017, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef SHA2_H
#define SHA2_H
#ifdef __cplusplus
extern "C" {
#endif

void sha256(const void *in, unsigned long inlen, void* out);
void sha384(const void *in, unsigned long inlen, void* out);
void sha512(const void *in, unsigned long inlen, void* out);

#ifdef __cplusplus
}
#endif
#endif // SHA2_H
