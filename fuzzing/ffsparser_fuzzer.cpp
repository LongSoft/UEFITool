/* ffsparser_fuzzer.cpp
 
 Copyright (c) 2023, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

#include "../common/ffsparser.h"

#define FUZZING_MIN_INPUT_SIZE 16
#define FUZZING_MAX_INPUT_SIZE (128 * 1024 * 1024)

extern "C" int LLVMFuzzerTestOneInput(const char *Data, long long Size) {
    // Do not overblow the inout file size, won't change much in practical sense
    if (Size > FUZZING_MAX_INPUT_SIZE || Size < FUZZING_MIN_INPUT_SIZE) return 0;

    // Create the FFS parser
    TreeModel* model = new TreeModel();
    FfsParser* ffsParser = new FfsParser(model);

    // Parse the image
    (void)ffsParser->parse(UByteArray(Data, (uint32_t)Size));

    delete model;
    delete ffsParser;

    return 0;
}
