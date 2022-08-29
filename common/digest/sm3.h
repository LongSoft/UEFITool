/* sm3.h

Copyright (c) 2022, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2019 Huawei Technologies Co., Ltd
 */
#ifndef SM3_H
#define SM3_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void sm3(const void *in, unsigned long inlen, void* out);

#ifdef __cplusplus
}
#endif
#endif /* SM3_H */
