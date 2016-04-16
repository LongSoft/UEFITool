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
typedef unsigned int UINTN; 

#define CONST  const
#define VOID   void
#define STATIC static

#ifndef TRUE
#define TRUE  ((BOOLEAN)(1==1))
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN)(0==1))
#endif

typedef UINT8 STATUS;
#define ERR_SUCCESS                         0
#define ERR_INVALID_PARAMETER               1
#define ERR_BUFFER_TOO_SMALL                2
#define ERR_OUT_OF_RESOURCES                3
#define ERR_OUT_OF_MEMORY                   4
#define ERR_FILE_OPEN                       5
#define ERR_FILE_READ                       6
#define ERR_FILE_WRITE                      7
#define ERR_ITEM_NOT_FOUND                  8
#define ERR_UNKNOWN_ITEM_TYPE               9
#define ERR_INVALID_FLASH_DESCRIPTOR        10
#define ERR_INVALID_REGION                  11
#define ERR_EMPTY_REGION                    12
#define ERR_BIOS_REGION_NOT_FOUND           13
#define ERR_VOLUMES_NOT_FOUND               14
#define ERR_INVALID_VOLUME                  15
#define ERR_VOLUME_REVISION_NOT_SUPPORTED   16
#define ERR_COMPLEX_BLOCK_MAP               17
#define ERR_UNKNOWN_FFS                     18
#define ERR_INVALID_FILE                    19
#define ERR_INVALID_SECTION                 20
#define ERR_UNKNOWN_SECTION                 21
#define ERR_STANDARD_COMPRESSION_FAILED     22
#define ERR_CUSTOMIZED_COMPRESSION_FAILED   23
#define ERR_STANDARD_DECOMPRESSION_FAILED   24
#define ERR_CUSTOMIZED_DECOMPRESSION_FAILED 25
#define ERR_UNKNOWN_COMPRESSION_TYPE        26
#define ERR_DEPEX_PARSE_FAILED              27
#define ERR_UNKNOWN_EXTRACT_MODE            28
#define ERR_UNKNOWN_REPLACE_MODE            29
#define ERR_UNKNOWN_IMAGE_TYPE              30
#define ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE 31
#define ERR_UNKNOWN_RELOCATION_TYPE         32
#define ERR_DIR_ALREADY_EXIST               33
#define ERR_DIR_CREATE                      34
#define ERR_TRUNCATED_IMAGE                 35
#define ERR_INVALID_CAPSULE                 36
#define ERR_STORES_NOT_FOUND                37
#define ERR_NOT_IMPLEMENTED                 0xFF

// UDK porting definitions
#define IN
#define OUT
#define EFIAPI
#define EFI_STATUS UINTN
#define EFI_SUCCESS ERR_SUCCESS
#define EFI_INVALID_PARAMETER ERR_INVALID_PARAMETER
#define EFI_OUT_OF_RESOURCES ERR_OUT_OF_RESOURCES
#define EFI_BUFFER_TOO_SMALL ERR_BUFFER_TOO_SMALL
#define EFI_ERROR(X) (X)

// Compression algorithms
#define COMPRESSION_ALGORITHM_UNKNOWN     0
#define COMPRESSION_ALGORITHM_NONE        1
#define COMPRESSION_ALGORITHM_EFI11       2
#define COMPRESSION_ALGORITHM_TIANO       3
#define COMPRESSION_ALGORITHM_UNDECIDED   4
#define COMPRESSION_ALGORITHM_LZMA        5
#define COMPRESSION_ALGORITHM_IMLZMA      6

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
    UINT8 Data[16];
} EFI_GUID;

// EFI Time
typedef struct EFI_TIME_ {
    UINT16  Year;       // Year:       2000 - 20XX
    UINT8   Month;      // Month:      1 - 12
    UINT8   Day;        // Day:        1 - 31
    UINT8   Hour;       // Hour:       0 - 23
    UINT8   Minute;     // Minute:     0 - 59
    UINT8   Second;     // Second:     0 - 59
UINT8: 8;
    UINT32  Nanosecond; // Nanosecond: 0 - 999,999,999
    INT16   TimeZone;   // TimeZone:   -1440 to 1440 or UNSPECIFIED (0x07FF)
    UINT8   Daylight;   // Daylight:   ADJUST_DAYLIGHT (1) or IN_DAYLIGHT (2) 
UINT8: 8;
} EFI_TIME;

#define ALIGN4(Value) (((Value)+3) & ~3)
#define ALIGN8(Value) (((Value)+7) & ~7)

#include <assert.h>
#define ASSERT(x) assert(x)

//Hexarg macros
#define hexarg(X) arg(QString("%1").arg((X),0,16).toUpper())
#define hexarg2(X, Y) arg(QString("%1").arg((X),(Y),16,QLatin1Char('0')).toUpper())

// SHA256 hash size in bytes
#define SHA256_HASH_SIZE 0x20

#endif // BASETYPES_H
