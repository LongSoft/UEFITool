/** @file
  This file declares GUIDed section extraction protocol.

  This interface provides a means of decoding a GUID defined encapsulation 
  section. There may be multiple different GUIDs associated with the GUIDed
  section extraction protocol. That is, all instances of the GUIDed section
  extraction protocol must have the same interface structure.

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at:
    http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  File Name: EfiGuidedSectionExtraction.h

  @par Revision Reference: PI
  Version 1.00.

**/

#ifndef __EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__
#define __EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL_H__

//
// Forward reference for pure ANSI compatability

typedef struct _EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL;


/**
  The ExtractSection() function processes the input section and
  allocates a buffer from the pool in which it returns the section
  contents. If the section being extracted contains
  authentication information (the section's
  GuidedSectionHeader.Attributes field has the
  EFI_GUIDED_SECTION_AUTH_STATUS_VALID bit set), the values
  returned in AuthenticationStatus must reflect the results of
  the authentication operation. Depending on the algorithm and
  size of the encapsulated data, the time that is required to do
  a full authentication may be prohibitively long for some
  classes of systems. To indicate this, use
  EFI_SECURITY_POLICY_PROTOCOL_GUID, which may be published by
  the security policy driver (see the Platform Initialization
  Driver Execution Environment Core Interface Specification for
  more details and the GUID definition). If the
  EFI_SECURITY_POLICY_PROTOCOL_GUID exists in the handle
  database, then, if possible, full authentication should be
  skipped and the section contents simply returned in the
  OutputBuffer. In this case, the
  EFI_AUTH_STATUS_PLATFORM_OVERRIDE bit AuthenticationStatus
  must be set on return. ExtractSection() is callable only from
  TPL_NOTIFY and below. Behavior of ExtractSection() at any
  EFI_TPL above TPL_NOTIFY is undefined. Type EFI_TPL is
  defined in RaiseTPL() in the UEFI 2.0 specification.

  
  @param This   Indicates the
                EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL instance.
  
  @param InputSection Buffer containing the input GUIDed section
                      to be processed. OutputBuffer OutputBuffer
                      is allocated from boot services pool
                      memory and contains the new section
                      stream. The caller is responsible for
                      freeing this buffer.

  @param OutputSize   A pointer to a caller-allocated UINTN in
                      which the size of OutputBuffer allocation
                      is stored. If the function returns
                      anything other than EFI_SUCCESS, the value
                      of OutputSize is undefined.

  @param AuthenticationStatus A pointer to a caller-allocated
                              UINT32 that indicates the
                              authentication status of the
                              output buffer. If the input
                              section's
                              GuidedSectionHeader.Attributes
                              field has the
                              EFI_GUIDED_SECTION_AUTH_STATUS_VAL
                              bit as clear, AuthenticationStatus
                              must return zero. Both local bits
                              (19:16) and aggregate bits (3:0)
                              in AuthenticationStatus are
                              returned by ExtractSection().
                              These bits reflect the status of
                              the extraction operation. The bit
                              pattern in both regions must be
                              the same, as the local and
                              aggregate authentication statuses
                              have equivalent meaning at this
                              level. If the function returns
                              anything other than EFI_SUCCESS,
                              the value of AuthenticationStatus
                              is undefined.


  @retval EFI_SUCCESS The InputSection was successfully
                      processed and the section contents were
                      returned.

  @retval EFI_OUT_OF_RESOURCES  The system has insufficient
                                resources to process the
                                request.

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction
                                Protocol.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_EXTRACT_GUIDED_SECTION)(
  IN CONST  EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL  *This,
  IN CONST  VOID                                    *InputSection,
  OUT       VOID                                    **OutputBuffer,
  OUT       UINTN                                   *OutputSize,
  OUT       UINT32                                  *AuthenticationStatus
);


/**
  
  Takes the GUIDed section as input and produces the section
  stream data. See the ExtractSection() function description.

**/
struct _EFI_GUIDED_SECTION_EXTRACTION_PROTOCOL {
  EFI_EXTRACT_GUIDED_SECTION  ExtractSection;
};

//
// Protocol GUID definition. Each GUIDed section extraction protocol has the
// same interface but with different GUID. All the GUIDs is defined here.
// May add multiple GUIDs here.
//
#define EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID \
  { \
    0xFC1BCDB0, 0x7D31, 0x49aa, {0x93, 0x6A, 0xA4, 0x60, 0x0D, 0x9D, 0xD0, 0x83 } \
  }

//
// may add other GUID here
//
extern EFI_GUID gEfiCrc32GuidedSectionExtractionProtocolGuid;

#endif
