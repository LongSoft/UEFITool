/* sha256.h

Copyright (c) 2017, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef SHA256_H
#define SHA256_H
#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_DIGEST_SIZE 32

void sha256(const void *in, unsigned long inlen, void* out);
    
#ifdef __cplusplus
}
#endif
#endif // SHA256_H
