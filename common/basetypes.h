/* basetypes.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef BASETYPES_H
#define BASETYPES_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef size_t USTATUS;
#define U_SUCCESS                         0
#define U_INVALID_PARAMETER               1
#define U_BUFFER_TOO_SMALL                2
#define U_OUT_OF_RESOURCES                3
#define U_OUT_OF_MEMORY                   4
#define U_FILE_OPEN                       5
#define U_FILE_READ                       6
#define U_FILE_WRITE                      7
#define U_ITEM_NOT_FOUND                  8
#define U_UNKNOWN_ITEM_TYPE               9
#define U_INVALID_FLASH_DESCRIPTOR        10
#define U_INVALID_REGION                  11
#define U_EMPTY_REGION                    12
#define U_BIOS_REGION_NOT_FOUND           13
#define U_VOLUMES_NOT_FOUND               14
#define U_INVALID_VOLUME                  15
#define U_VOLUME_REVISION_NOT_SUPPORTED   16
#define U_COMPLEX_BLOCK_MAP               17
#define U_UNKNOWN_FFS                     18
#define U_INVALID_FILE                    19
#define U_INVALID_SECTION                 20
#define U_UNKNOWN_SECTION                 21
#define U_STANDARD_COMPRESSION_FAILED     22
#define U_CUSTOMIZED_COMPRESSION_FAILED   23
#define U_STANDARD_DECOMPRESSION_FAILED   24
#define U_CUSTOMIZED_DECOMPRESSION_FAILED 25
#define U_GZIP_DECOMPRESSION_FAILED       26
#define U_UNKNOWN_COMPRESSION_TYPE        27
#define U_DEPEX_PARSE_FAILED              28
#define U_UNKNOWN_EXTRACT_MODE            29
#define U_UNKNOWN_REPLACE_MODE            30
#define U_UNKNOWN_IMAGE_TYPE              31
#define U_UNKNOWN_PE_OPTIONAL_HEADER_TYPE 32
#define U_UNKNOWN_RELOCATION_TYPE         33
#define U_DIR_ALREADY_EXIST               34
#define U_DIR_CREATE                      35
#define U_DIR_CHANGE                      36
#define U_TRUNCATED_IMAGE                 37
#define U_INVALID_CAPSULE                 38
#define U_STORES_NOT_FOUND                39
#define U_INVALID_IMAGE                   40
#define U_INVALID_RAW_AREA                41
#define U_INVALID_FIT                     42
#define U_INVALID_MICROCODE               43
#define U_INVALID_ACM                     44
#define U_INVALID_BG_KEY_MANIFEST         45
#define U_INVALID_BG_BOOT_POLICY          46
#define U_INVALID_TXT_CONF                47
#define U_ELEMENTS_NOT_FOUND              48
#define U_PEI_CORE_ENTRY_POINT_NOT_FOUND  49
#define U_INVALID_STORE_SIZE              50
#define U_UNKNOWN_COMPRESSION_ALGORITHM   51
#define U_NOTHING_TO_PATCH                52
#define U_UNKNOWN_PATCH_TYPE              53
#define U_PATCH_OFFSET_OUT_OF_BOUNDS      54
#define U_INVALID_SYMBOL                  55

#define U_INVALID_MANIFEST                251
#define U_UNKNOWN_MANIFEST_HEADER_VERSION 252
#define U_INVALID_ME_PARTITION_TABLE      253
#define U_INVALID_ME_PARTITION            254

#define U_NOT_IMPLEMENTED                 0xFF

// EDK2 porting definitions
typedef uint8_t      BOOLEAN;
typedef int8_t       INT8;
typedef uint8_t      UINT8;
typedef int16_t      INT16;
typedef uint16_t     UINT16;
typedef int32_t      INT32;
typedef uint32_t     UINT32;
typedef int64_t      INT64;
typedef uint64_t     UINT64;
typedef char         CHAR8;
typedef uint16_t     CHAR16;
typedef size_t       UINTN;
typedef ptrdiff_t    INTN;

#define CONST  const
#define VOID   void
#define STATIC static

#ifndef INT32_MAX
#define INT32_MAX 0x7fffffff
#endif

#ifndef TRUE
#define TRUE  ((BOOLEAN)(1==1))
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN)(0==1))
#endif

#define IN
#define OUT
#define EFIAPI
#define EFI_STATUS UINTN
#define EFI_SUCCESS U_SUCCESS
#define EFI_INVALID_PARAMETER U_INVALID_PARAMETER
#define EFI_OUT_OF_RESOURCES U_OUT_OF_RESOURCES
#define EFI_BUFFER_TOO_SMALL U_BUFFER_TOO_SMALL
#define EFI_ERROR(X) (X)

// Compression algorithms
#define COMPRESSION_ALGORITHM_UNKNOWN                0
#define COMPRESSION_ALGORITHM_NONE                   1
#define COMPRESSION_ALGORITHM_EFI11                  2
#define COMPRESSION_ALGORITHM_TIANO                  3
#define COMPRESSION_ALGORITHM_UNDECIDED              4
#define COMPRESSION_ALGORITHM_LZMA                   5
#define COMPRESSION_ALGORITHM_LZMA_INTEL_LEGACY      6
#define COMPRESSION_ALGORITHM_LZMAF86                7
#define COMPRESSION_ALGORITHM_GZIP                   8


// Item create modes
#define CREATE_MODE_APPEND    0
#define CREATE_MODE_PREPEND   1
#define CREATE_MODE_BEFORE    2
#define CREATE_MODE_AFTER     3

// Item extract modes
#define EXTRACT_MODE_AS_IS                 0
#define EXTRACT_MODE_BODY                  1
#define EXTRACT_MODE_BODY_UNCOMPRESSED     2

// Item replace modes
#define REPLACE_MODE_AS_IS    0
#define REPLACE_MODE_BODY     1

// Item patch modes
#define PATCH_MODE_HEADER     0
#define PATCH_MODE_BODY       1

// Patch types
#define PATCH_TYPE_OFFSET    'O'
#define PATCH_TYPE_PATTERN   'P'

// Erase polarity types
#define ERASE_POLARITY_FALSE   0
#define ERASE_POLARITY_TRUE    1
#define ERASE_POLARITY_UNKNOWN 0xFF

// Search modes
#define SEARCH_MODE_HEADER    1
#define SEARCH_MODE_BODY      2
#define SEARCH_MODE_ALL       3

// EFI GUID
typedef struct EFI_GUID_ {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

// EFI Time
typedef struct EFI_TIME_ {
    UINT16  Year;       // Year:       2000 - 20XX
    UINT8   Month;      // Month:      1 - 12
    UINT8   Day;        // Day:        1 - 31
    UINT8   Hour;       // Hour:       0 - 23
    UINT8   Minute;     // Minute:     0 - 59
    UINT8   Second;     // Second:     0 - 59
    UINT8   Reserved0;
    UINT32  Nanosecond; // Nanosecond: 0 - 999,999,999
    INT16   TimeZone;   // TimeZone:   -1440 to 1440 or UNSPECIFIED (0x07FF)
    UINT8   Daylight;   // Daylight:   ADJUST_DAYLIGHT (1) or IN_DAYLIGHT (2) 
    UINT8   Reserved1;
} EFI_TIME;

// Align to 4 or 8 bytes
#define ALIGN4(Value) (((Value)+3) & ~3)
#define ALIGN8(Value) (((Value)+7) & ~7)

// Unused parameter declaration
#define U_UNUSED_PARAMETER(x) ((void)x)

// Assert macro
#include <assert.h>
#define ASSERT(x) assert(x)

// SHA256 hash size in bytes
#define SHA256_HASH_SIZE 0x20

#endif // BASETYPES_H
