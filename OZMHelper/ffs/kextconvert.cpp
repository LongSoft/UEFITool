#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <IndustryStandard/PeImage.h>

#include "CommonLib.h"
#include "Compress.h"
#include "Crc32.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"

#include "kextconvert.h"

#define UTILITY_NAME            "GenSec"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   1

STATIC CHAR8      *mSectionTypeName[] = {
  NULL,                                 // 0x00 - reserved
  "EFI_SECTION_COMPRESSION",            // 0x01
  "EFI_SECTION_GUID_DEFINED",           // 0x02
  NULL,                                 // 0x03 - reserved
  NULL,                                 // 0x04 - reserved
  NULL,                                 // 0x05 - reserved
  NULL,                                 // 0x06 - reserved
  NULL,                                 // 0x07 - reserved
  NULL,                                 // 0x08 - reserved
  NULL,                                 // 0x09 - reserved
  NULL,                                 // 0x0A - reserved
  NULL,                                 // 0x0B - reserved
  NULL,                                 // 0x0C - reserved
  NULL,                                 // 0x0D - reserved
  NULL,                                 // 0x0E - reserved
  NULL,                                 // 0x0F - reserved
  "EFI_SECTION_PE32",                   // 0x10
  "EFI_SECTION_PIC",                    // 0x11
  "EFI_SECTION_TE",                     // 0x12
  "EFI_SECTION_DXE_DEPEX",              // 0x13
  "EFI_SECTION_VERSION",                // 0x14
  "EFI_SECTION_USER_INTERFACE",         // 0x15
  "EFI_SECTION_COMPATIBILITY16",        // 0x16
  "EFI_SECTION_FIRMWARE_VOLUME_IMAGE",  // 0x17
  "EFI_SECTION_FREEFORM_SUBTYPE_GUID",  // 0x18
  "EFI_SECTION_RAW",                    // 0x19
  NULL,                                 // 0x1A
  "EFI_SECTION_PEI_DEPEX",              // 0x1B
  "EFI_SECTION_SMM_DEPEX"               // 0x1C
};

STATIC CHAR8      *mCompressionTypeName[]    = { "PI_NONE", "PI_STD" };

#define EFI_GUIDED_SECTION_NONE 0x80
STATIC CHAR8      *mGUIDedSectionAttribue[]  = { "NONE", "PROCESSING_REQUIRED", "AUTH_STATUS_VALID"};

STATIC CHAR8 *mAlignName[] = {
  "1", "2", "4", "8", "16", "32", "64", "128", "256", "512",
  "1K", "2K", "4K", "8K", "16K", "32K", "64K"
};

STATIC UINT32 mFfsValidAlign[] = {0, 8, 16, 128, 512, 1024, 4096, 32768, 65536};

//
// Crc32 GUID section related definitions.
//
typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

typedef struct {
  EFI_GUID_DEFINED_SECTION2 GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER2;

STATIC EFI_GUID  mZeroGuid                 = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
STATIC EFI_GUID  mEfiCrc32SectionGuid      = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

KextConvert::KextConvert()
{
}

KextConvert::~KextConvert()
{
}


/*++

Routine Description:

  This function calculates the value needed for a valid UINT8 checksum

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 8 bit checksum value needed.

--*/
UINT8
CalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  return (UINT8) (0x100 - CalculateSum8 (Buffer, Size));
}

/*++

Routine Description::

  This function calculates the UINT8 sum for the requested region.

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 8 bit checksum value needed.

--*/
UINT8
CalculateSum8 (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )

{
  UINTN Index;
  UINT8 Sum;

  Sum = 0;

  //
  // Perform the byte sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT8) (Sum + Buffer[Index]);
  }

  return Sum;
}

/*++

Routine Description:

  Write ascii string as unicode string format to FILE

Arguments:

  String      - Pointer to string that is written to FILE.
  UniString   - Pointer to unicode string

Returns:

  NULL

--*/
VOID Ascii2UnicodeString (CHAR8 *String, CHAR16   *UniString)
{
  while (*String != '\0') {
    *(UniString++) = (CHAR16) *(String++);
  }
  //
  // End the UniString with a NULL.
  //
  *UniString = '\0';
}


/*++

Routine Description:

  Get the contents of all section files specified in InputFileName
  into FileBuffer.

Arguments:

  InputFileName  - Name of the input file.

  InputFileAlign - Alignment required by the input file data.

  InputFileNum   - Number of input files. Should be at least 1.

  FileBuffer     - Output buffer to contain data

  BufferLength   - On input, this is size of the FileBuffer.
                   On output, this is the actual length of the data.

  MaxAlignment   - The max alignment required by all the input file datas.

  PeSectionNum   - Calculate the number of Pe/Te Section in this FFS file.

Returns:

  EFI_SUCCESS on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1 or BufferLength point is NULL.
  EFI_ABORTED if unable to open input file.
  EFI_BUFFER_TOO_SMALL FileBuffer is not enough to contain all file data.
--*/
/*
STATIC EFI_STATUS GetSectionContents (
  IN  CHAR8   **InputFileName,
  IN  UINT32  *InputFileAlign,
  IN  UINT32  InputFileNum,
  OUT UINT8   *FileBuffer,
  OUT UINT32  *BufferLength,
  OUT UINT32  *MaxAlignment,
  OUT UINT8   *PESectionNum
  )
*/
STATIC EFI_STATUS GetSectionContents(QByteArray input[], UINT32 *InputFileAlign, UINT32 InputFileNum,
                                     UINT8 *FileBuffer, UINT32  *BufferLength, UINT32 *MaxAlignment, UINT8 *PESectionNum)
{
  UINT32                     Size;
  UINT32                     Offset;
  UINT32                     FileSize;
  UINT32                     Index;
//  FILE                       *InFile;
  EFI_COMMON_SECTION_HEADER  *SectHeader;
  EFI_COMMON_SECTION_HEADER2 TempSectHeader;
  EFI_TE_IMAGE_HEADER        TeHeader;
  UINT32                     TeOffset;
  EFI_GUID_DEFINED_SECTION   GuidSectHeader;
  EFI_GUID_DEFINED_SECTION2  GuidSectHeader2;
  UINT32                     HeaderSize;

  Size          = 0;
  Offset        = 0;
  TeOffset      = 0;

  //
  // Go through our array of file names and copy their contents
  // to the output buffer.
  //
  for (Index = 0; Index < InputFileNum; Index++) {
    //
    // make sure section ends on a DWORD boundary
    //
    while ((Size & 0x03) != 0) {
      Size++;
    }

    //
    // Get the Max alignment of all input file datas
    //
    if (*MaxAlignment < InputFileAlign [Index]) {
      *MaxAlignment = InputFileAlign [Index];
    }

    //
    // Open file and read contents
    //
//    InFile = fopen (InputFileName[Index], "rb");
//    if (InFile == NULL) {
//      Error (NULL, 0, 0001, "Error opening file", InputFileName[Index]);
//      return EFI_ABORTED;
//    }

//    fseek (InFile, 0, SEEK_END);
//    FileSize = ftell (InFile);
//    fseek (InFile, 0, SEEK_SET);
      FileSize = input[Index].size();
      DebugMsg (NULL, 0, 9, "Input section files",
              "the input section is size %u bytes", (unsigned) FileSize);

    //
    // Check this section is Te/Pe section, and Calculate the numbers of Te/Pe section.
    //
    TeOffset = 0;
    if (FileSize >= MAX_FFS_SIZE) {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER2);
    } else {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER);
    }
//    fread (&TempSectHeader, 1, HeaderSize, InFile);
      memcpy(&TempSectHeader,input[Index].data(), HeaderSize);
    if (TempSectHeader.Type == EFI_SECTION_TE) {
      (*PESectionNum) ++;
//      fread (&TeHeader, 1, sizeof (TeHeader), InFile);
        memcpy(&TeHeader, &input[Index].data()[HeaderSize], sizeof(TeHeader));
      if (TeHeader.Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
        TeOffset = TeHeader.StrippedSize - sizeof (TeHeader);
      }
    } else if (TempSectHeader.Type == EFI_SECTION_PE32) {
      (*PESectionNum) ++;
    } else if (TempSectHeader.Type == EFI_SECTION_GUID_DEFINED) {
//      fseek (InFile, 0, SEEK_SET);
      if (FileSize >= MAX_SECTION_SIZE) {
//        fread (&GuidSectHeader2, 1, sizeof (GuidSectHeader2), InFile);
          memcpy(&GuidSectHeader2, input[Index].data(), sizeof (GuidSectHeader2));
        if ((GuidSectHeader2.Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
          HeaderSize = GuidSectHeader2.DataOffset;
        }
      } else {
//        fread (&GuidSectHeader, 1, sizeof (GuidSectHeader), InFile);
          memcpy(&GuidSectHeader, input[Index].data(), sizeof (GuidSectHeader));
        if ((GuidSectHeader.Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
          HeaderSize = GuidSectHeader.DataOffset;
        }
      }
      (*PESectionNum) ++;
    } else if (TempSectHeader.Type == EFI_SECTION_COMPRESSION ||
               TempSectHeader.Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
      //
      // for the encapsulated section, assume it contains Pe/Te section
      //
      (*PESectionNum) ++;
    }

//    fseek (InFile, 0, SEEK_SET);

    //
    // Revert TeOffset to the converse value relative to Alignment
    // This is to assure the original PeImage Header at Alignment.
    //
    if ((TeOffset != 0) && (InputFileAlign [Index] != 0)) {
      TeOffset = InputFileAlign [Index] - (TeOffset % InputFileAlign [Index]);
      TeOffset = TeOffset % InputFileAlign [Index];
    }

    //
    // make sure section data meet its alignment requirement by adding one raw pad section.
    // But the different sections have the different section header. Necessary or not?
    // Based on section type to adjust offset? Todo
    //
    if ((InputFileAlign [Index] != 0) && (((Size + HeaderSize + TeOffset) % InputFileAlign [Index]) != 0)) {
      Offset = (Size + sizeof (EFI_COMMON_SECTION_HEADER) + HeaderSize + TeOffset + InputFileAlign [Index] - 1) & ~(InputFileAlign [Index] - 1);
      Offset = Offset - Size - HeaderSize - TeOffset;

      if (FileBuffer != NULL && ((Size + Offset) < *BufferLength)) {
        //
        // The maximal alignment is 64K, the raw section size must be less than 0xffffff
        //
        memset (FileBuffer + Size, 0, Offset);
        SectHeader          = (EFI_COMMON_SECTION_HEADER *) (FileBuffer + Size);
        SectHeader->Type    = EFI_SECTION_RAW;
        SectHeader->Size[0] = (UINT8) (Offset & 0xff);
        SectHeader->Size[1] = (UINT8) ((Offset & 0xff00) >> 8);
        SectHeader->Size[2] = (UINT8) ((Offset & 0xff0000) >> 16);
      }
      DebugMsg (NULL, 0, 9, "Pad raw section for section data alignment",
                "Pad Raw section size is %u", (unsigned) Offset);

      Size = Size + Offset;
    }

    //
    // Now read the contents of the file into the buffer
    // Buffer must be enough to contain the file content.
    //
    if ((FileSize > 0) && (FileBuffer != NULL) && ((Size + FileSize) <= *BufferLength)) {
//      if (fread (FileBuffer + Size, (size_t) FileSize, 1, InFile) != 1) {
        memcpy(&FileBuffer[Size], input[Index].data(), (size_t) FileSize);
//        Error (NULL, 0, 0004, "Error reading file", InputFileName[Index]);
//        fclose (InFile);
//        return EFI_ABORTED;
//      }
    }


//    fclose (InFile);
    Size += FileSize;
  }

  //
  // Set the actual length of the data.
  //
  if (Size > *BufferLength) {
    *BufferLength = Size;
    return EFI_BUFFER_TOO_SMALL;
  } else {
    *BufferLength = Size;
    return EFI_SUCCESS;
  }
}


// GenFfs -t EFI_FV_FILETYPE_DRIVER -g GUID -o output -i pe32 -i userinterface
EFI_STATUS GenFFS(UINT8 type, QString GUID, QByteArray inputPE32, QByteArray userinterface, QByteArray & out)
{
    UINT8  PeSectionNum;
    UINT8 *FileBuffer;
    UINT32 Index;
    UINT32 HeaderSize;
    UINT32 FfsAlign;
    UINT32 InputFileNum;
    UINT32 MaxAlignment;
    UINT32 FileSize;
    UINT32 *InputFileAlign;
    EFI_FFS_FILE_ATTRIBUTES FfsAttrib;
    EFI_FFS_FILE_HEADER2    FfsFileHeader;
    EFI_FV_FILETYPE         FfsFiletype;
    EFI_STATUS Status;
    EFI_GUID FileGuid = {0};

    QByteArray inputFile[] = {inputPE32, userinterface};


    Index          = 0;
    FfsAttrib      = 0;
    InputFileAlign = NULL;
    FileBuffer     = NULL;
    FileSize       = 0;
    MaxAlignment   = 1;
    Status         = EFI_SUCCESS;
    PeSectionNum   = 0;
    HeaderSize     = 0;
    FfsAlign       = 0;
    InputFileNum   = 2;
    FfsFiletype    = type;

    Status = StringToGuid (GUID.toLocal8Bit().data(), &FileGuid);
    if (EFI_ERROR (Status)) {
      printf ("Invalid GUID: %s\n", qPrintable(GUID));
      return STATUS_ERROR;
    }

    //
    // Allocate Input file name buffer and its alignment buffer.
    //

    InputFileAlign = (UINT32 *) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));
    if (InputFileAlign == NULL) {
        printf("Resource: memory cannot be allocated!\n");
        return STATUS_ERROR;
    }
    memset (InputFileAlign, 0, MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));

    //
    // Output input parameter information
    //
    VerboseMsg ("FFS File Guid is %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  (unsigned) FileGuid.Data1,
                  FileGuid.Data2,
                  FileGuid.Data3,
                  FileGuid.Data4[0],
                  FileGuid.Data4[1],
                  FileGuid.Data4[2],
                  FileGuid.Data4[3],
                  FileGuid.Data4[4],
                  FileGuid.Data4[5],
                  FileGuid.Data4[6],
                  FileGuid.Data4[7]);
    if ((FfsAttrib & FFS_ATTRIB_FIXED) != 0) {
      VerboseMsg ("FFS File has the fixed file attribute");
    }
    if ((FfsAttrib & FFS_ATTRIB_CHECKSUM) != 0) {
      VerboseMsg ("FFS File requires the checksum of the whole file");
    }

    for (Index = 0; Index < InputFileNum; Index ++) {
      if (InputFileAlign[Index] == 0) {
        //
        // Minimum alignment is 1 byte.
        //
        InputFileAlign[Index] = 1;
      }
      VerboseMsg ("the %dth input section alignment is %u", Index, (unsigned) InputFileAlign[Index]);
    }

    //
    // Calculate the size of all input section files.
    //
    Status = GetSectionContents (
//               InputFileName,
               inputFile,
               InputFileAlign,
               InputFileNum,
               FileBuffer,
               &FileSize,
               &MaxAlignment,
               &PeSectionNum
               );

    if ((FfsFiletype == EFI_FV_FILETYPE_SECURITY_CORE ||
        FfsFiletype == EFI_FV_FILETYPE_PEI_CORE ||
        FfsFiletype == EFI_FV_FILETYPE_DXE_CORE) && (PeSectionNum != 1)) {
      printf("Invalid parameter: Fv File type must have one and only one Pe or Te section, but %u Pe/Te section are input\n", PeSectionNum);
      return STATUS_ERROR;
    }

    if ((FfsFiletype == EFI_FV_FILETYPE_PEIM ||
        FfsFiletype == EFI_FV_FILETYPE_DRIVER ||
        FfsFiletype == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER ||
        FfsFiletype == EFI_FV_FILETYPE_APPLICATION) && (PeSectionNum < 1)) {
      printf("Invalid parameter: Fv File type must have at least one Pe or Te section, but no Pe/Te section is input\n");
      return STATUS_ERROR;
    }

    if (Status == EFI_BUFFER_TOO_SMALL) {
      FileBuffer = (UINT8 *) malloc (FileSize);
      if (FileBuffer == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        return STATUS_ERROR;
      }
      memset (FileBuffer, 0, FileSize);

      //
      // read all input file contents into a buffer
      //
      Status = GetSectionContents (
                 inputFile,
                 InputFileAlign,
                 InputFileNum,
                 FileBuffer,
                 &FileSize,
                 &MaxAlignment,
                 &PeSectionNum
                 );
    }

    if (EFI_ERROR (Status)) {
      return STATUS_ERROR;
    }

    //
    // Create Ffs file header.
    //
    memset (&FfsFileHeader, 0, sizeof (EFI_FFS_FILE_HEADER2));
    memcpy (&FfsFileHeader.Name, &FileGuid, sizeof (EFI_GUID));
    FfsFileHeader.Type       = FfsFiletype;
    //
    // Update FFS Alignment based on the max alignment required by input section files
    //
    VerboseMsg ("the max alignment of all input sections is %u", (unsigned) MaxAlignment);
    for (Index = 0; Index < sizeof (mFfsValidAlign) / sizeof (UINT32) - 1; Index ++) {
      if ((MaxAlignment > mFfsValidAlign [Index]) && (MaxAlignment <= mFfsValidAlign [Index + 1])) {
        break;
      }
    }
    if (FfsAlign < Index) {
      FfsAlign = Index;
    }
    VerboseMsg ("the alignment of the generated FFS file is %u", (unsigned) mFfsValidAlign [FfsAlign + 1]);

    //
    // Now FileSize includes the EFI_FFS_FILE_HEADER
    //
    if (FileSize + sizeof (EFI_FFS_FILE_HEADER) >= MAX_FFS_SIZE) {
      HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
      FileSize += sizeof (EFI_FFS_FILE_HEADER2);
      FfsFileHeader.ExtendedSize = FileSize;
      memset(FfsFileHeader.Size, 0, sizeof (UINT8) * 3);
      FfsAttrib |= FFS_ATTRIB_LARGE_FILE;
    } else {
      HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
      FileSize += sizeof (EFI_FFS_FILE_HEADER);
      FfsFileHeader.Size[0]  = (UINT8) (FileSize & 0xFF);
      FfsFileHeader.Size[1]  = (UINT8) ((FileSize & 0xFF00) >> 8);
      FfsFileHeader.Size[2]  = (UINT8) ((FileSize & 0xFF0000) >> 16);
    }
    VerboseMsg ("the size of the generated FFS file is %u bytes", (unsigned) FileSize);

    FfsFileHeader.Attributes = (EFI_FFS_FILE_ATTRIBUTES) (FfsAttrib | (FfsAlign << 3));

    //
    // Fill in checksums and state, these must be zero for checksumming
    //
    // FileHeader.IntegrityCheck.Checksum.Header = 0;
    // FileHeader.IntegrityCheck.Checksum.File = 0;
    // FileHeader.State = 0;
    //
    FfsFileHeader.IntegrityCheck.Checksum.Header = CalculateChecksum8 (
                                                     (UINT8 *) &FfsFileHeader,
                                                     HeaderSize
                                                     );

    if (FfsFileHeader.Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Ffs header checksum = zero, so only need to calculate ffs body.
      //
      FfsFileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                     FileBuffer,
                                                     FileSize - HeaderSize
                                                     );
    } else {
      FfsFileHeader.IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    }

    FfsFileHeader.State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

    //
    // Open output file to write ffs data.
    //
    //
    // write header
    //
//    fwrite (&FfsFileHeader, 1, HeaderSize, FfsFile);
    out.clear();
    out.reserve(FileSize);
//    memcpy(out.data(),&FfsFileHeader, HeaderSize);
    out.append((const char*)&FfsFileHeader, HeaderSize);
    //
    // write data
    //
//    fwrite (FileBuffer, 1, FileSize - HeaderSize, FfsFile);
//    memcpy(out.data(), FileBuffer, FileSize - HeaderSize);
    out.append((const char*)FileBuffer, FileSize-HeaderSize);

//    fclose (FfsFile);

    if (InputFileAlign != NULL) {
      free (InputFileAlign);
    }
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }

    return STATUS_SUCCESS;
}

STATUS GenSectionUserInterface(QString name, QByteArray &out)
{
    UINT8 *Buffer;
    UINT32 Index;
    EFI_USER_INTERFACE_SECTION *UiSect;


    Index = sizeof (EFI_COMMON_SECTION_HEADER);

    // StringBuffer is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
//    Index += (strlen (StringBuffer) * 2) + 2;
    Index += (strlen(name.toLocal8Bit().constData()) * 2) +2;

    Buffer = (UINT8 *) malloc (Index);
    if (Buffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allcoated");
      return 1;
    }

    UiSect = (EFI_USER_INTERFACE_SECTION *) Buffer;
    UiSect->CommonHeader.Type     = EFI_SECTION_USER_INTERFACE;
    UiSect->CommonHeader.Size[0]  = (UINT8) (Index & 0xff);
    UiSect->CommonHeader.Size[1]  = (UINT8) ((Index & 0xff00) >> 8);
    UiSect->CommonHeader.Size[2]  = (UINT8) ((Index & 0xff0000) >> 16);
//    Ascii2UnicodeString (StringBuffer, UiSect->FileNameString);
    Ascii2UnicodeString (name.toLocal8Bit().data(), UiSect->FileNameString);

    printf("the size of the created section file is %u bytes\n", (unsigned) Index);

    out.clear();
    out.reserve(Index);
    out.append((const char*)Buffer, Index);
//    memcpy(&out.constData(), Buffer, Index);

    return STATUS_SUCCESS;
}

/*++

Routine Description:

  Generate a leaf section of type other than EFI_SECTION_VERSION
  and EFI_SECTION_USER_INTERFACE. Input file must be well formed.
  The function won't validate the input file's contents. For
  common leaf sections, the input file may be a binary file.
  The utility will add section header to the file.

Arguments:

  InputFileName  - Name of the input file.

  InputFileNum   - Number of input files. Should be 1 for leaf section.

  SectionType    - A valid section type string

  OutFileBuffer  - Buffer pointer to Output file contents

Returns:

  STATUS_ERROR            - can't continue
  STATUS_SUCCESS          - successful return

--*/
STATUS GenSectionPE32 (QByteArray inputbinary, QByteArray & out)
{
  UINT32                    InputFileLength;
//  FILE                      *InFile;
  UINT8                     *Buffer;
  UINT32                    TotalLength;
  UINT32                    HeaderLength;
  EFI_COMMON_SECTION_HEADER *CommonSect;

  Buffer  = NULL;

  InputFileLength = inputbinary.size();
  TotalLength     = sizeof (EFI_COMMON_SECTION_HEADER) + InputFileLength;
  //
  // Size must fit in 3 bytes
  //
  if (TotalLength >= MAX_SECTION_SIZE) {
    printf("Invalid paramter: file size (0x%X) exceeds section size limit(%uM).\n", (unsigned) TotalLength, MAX_SECTION_SIZE>>20);
    return STATUS_ERROR;
  }

  HeaderLength = sizeof (EFI_COMMON_SECTION_HEADER);
  if (TotalLength >= MAX_SECTION_SIZE) {
    TotalLength = sizeof (EFI_COMMON_SECTION_HEADER2) + InputFileLength;
    HeaderLength = sizeof (EFI_COMMON_SECTION_HEADER2);
  }
  printf("the size of the created section file is %u bytes\n", (unsigned) TotalLength);
  //
  // Fill in the fields in the local section header structure
  //
  Buffer = (UINT8 *) malloc ((size_t) TotalLength);
  if (Buffer == NULL) {
    printf ("memory cannot be allcoated\n");
    return STATUS_ERROR;
  }
  CommonSect = (EFI_COMMON_SECTION_HEADER *) Buffer;
  CommonSect->Type     = EFI_SECTION_PE32;
  if (TotalLength < MAX_SECTION_SIZE) {
    CommonSect->Size[0]  = (UINT8) (TotalLength & 0xff);
    CommonSect->Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
    CommonSect->Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
  } else {
    memset(CommonSect->Size, 0xff, sizeof(UINT8) * 3);
    ((EFI_COMMON_SECTION_HEADER2 *)CommonSect)->ExtendedSize = TotalLength;
  }

  memcpy(&Buffer[HeaderLength], inputbinary.data(), InputFileLength);

  //
  // Set OutFileBuffer
  //
  out.clear();
  out.reserve(TotalLength);
//  memcpy(out.data(), Buffer, TotalLength);
  out.append((const char*) Buffer, TotalLength);

  return STATUS_SUCCESS;
}

UINT8 KextConvert::createFFS(QString name, QString GUID, QByteArray inputbinary, QByteArray & output)
{
    QByteArray out;
    QByteArray pe32;
    QByteArray userinterface;

    UINT8 SectType;
    UINT32 InputLength;
    EFI_STATUS Status;
    EFI_COMMON_SECTION_HEADER *SectionHeader;

    //
    // GenSec -s EFI_SECTION_PE32 -o pe32 inputbinary
    //
//    SectType = EFI_SECTION_PE32;
/*
    Status = GenSectionCommonLeafSection (
              InputFileName,
              SectType,
              &OutFileBuffer
              );
*/
    out.clear();
    Status = GenSectionPE32(inputbinary, out);

    if (Status != EFI_SUCCESS) {
        printf("Status is not successful: Status value is 0x%X", (int) Status);
        return 1;
    }

    // Get output file length
//    SectionHeader = (EFI_COMMON_SECTION_HEADER *)OutFileBuffer;
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)out.data();
    InputLength = *(UINT32 *)SectionHeader->Size & 0x00ffffff;
    if (InputLength == 0xffffff) {
        InputLength = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
    }

    // Write the output file
    pe32 = out.mid(0, InputLength);

//   fwrite (OutFileBuffer, InputLength, 1, OutFile);

    //
    // GenSec -s EFI_SECTION_USER_INTERFACE -n basename+version -o userinterface
    //
//    SectType = EFI_SECTION_USER_INTERFACE;
    out.clear();
    Status = GenSectionUserInterface(name, out);

    if (Status != EFI_SUCCESS) {
        printf("Status is not successful: Status value is 0x%X", (int) Status);
        return 1;
    }

    // Get output file length
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)out.data();
    InputLength = *(UINT32 *)SectionHeader->Size & 0x00ffffff;
    if (InputLength == 0xffffff) {
        InputLength = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
    }

    printf("Userinterface InputLength: %i\n", InputLength);

    // Write the output file
    userinterface = out.mid(0, InputLength);
//    userinterface = out;
//     fwrite (OutFileBuffer, InputLength, 1, OutFile);

    //
    // GenFfs -t EFI_FV_FILETYPE_DRIVER -g GUID -o output -i pe32 -i userinterface
    //

    SectType = EFI_FV_FILETYPE_DRIVER;

    out.clear();
    Status = GenFFS(EFI_FV_FILETYPE_DRIVER, GUID, pe32, userinterface, out);

    output.clear();
    output.append(out);

    return STATUS_SUCCESS;
}

BOOLEAN KextConvert::getInfoFromPlist(QByteArray plist, QString & CFBundleName, QString & CFBundleShortVersionString)
{
    /* ToDo: Implement solid plist library - plist is bitchy */
}
