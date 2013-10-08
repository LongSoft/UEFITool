/* basetypes.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#ifndef __BASETYPES_H__
#define __BASETYPES_H__

#include <stdarg.h>
#include <stdint.h>

typedef uint8_t   BOOLEAN;
typedef int8_t    INT8;
typedef uint8_t   UINT8;
typedef int16_t   INT16;
typedef uint16_t  UINT16;
typedef int32_t   INT32;
typedef uint32_t  UINT32;
typedef int64_t   INT64;
typedef uint64_t  UINT64;
typedef char      CHAR8;
typedef uint16_t  CHAR16;

#define CONST  const
#define VOID   void
#define STATIC static

#ifndef TRUE
#define TRUE  ((BOOLEAN)(1==1))
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN)(0==1))
#endif

#ifndef NULL
#define NULL  ((VOID *) 0)
#endif

#if _MSC_EXTENSIONS
  //
  // Microsoft* compiler requires _EFIAPI useage, __cdecl is Microsoft* specific C.
  // 
  #define EFIAPI __cdecl  
#endif

#if __GNUC__
  #define EFIAPI __attribute__((cdecl))    
#endif

#define ERR_SUCCESS                       0
#define ERR_INVALID_PARAMETER             1
#define ERR_BUFFER_TOO_SMALL              2
#define ERR_OUT_OF_RESOURCES              3
#define ERR_OUT_OF_MEMORY                 4
#define ERR_FILE_OPEN                     5
#define ERR_FILE_READ                     6
#define ERR_FILE_WRITE                    7
#define ERR_INVALID_FLASH_DESCRIPTOR      8
#define ERR_BIOS_REGION_NOT_FOUND         9
#define ERR_VOLUMES_NOT_FOUND             10
#define ERR_INVALID_VOLUME                11
#define ERR_VOLUME_REVISION_NOT_SUPPORTED 12
#define ERR_UNKNOWN_FFS                   13
#define ERR_INVALID_FILE                  14


// EFI GUID
typedef struct{
    UINT8 Data[16];
} EFI_GUID; 

#define ALIGN4(Value) (((Value)+3) & ~3)
#define ALIGN8(Value) (((Value)+7) & ~7)

#include <assert.h>
#define ASSERT(x) assert(x)

#endif
