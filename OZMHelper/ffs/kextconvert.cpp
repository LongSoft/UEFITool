#include <Common/PiFirmwareFile.h>
#include <IndustryStandard/PeImage.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

#include "kextconvert.h"

#define EFI_GUIDED_SECTION_NONE 0x80

#define DEBUG 0

#if DEBUG
    #define dprintf(...) \
            do { printf(__VA_ARGS__); } while (0)
#else
    #define dprintf(...)
#endif

STATIC UINT32 mFfsValidAlign[] = {0, 8, 16, 128, 512, 1024, 4096, 32768, 65536};

KextConvert::KextConvert()
{
}

KextConvert::~KextConvert()
{
}


UINT8 KextConvert::CalculateChecksum8 (UINT8 *Buffer,UINT32 Size)
{
  return (UINT8) (0x100 - CalculateSum8 (Buffer, Size));
}


UINT8 KextConvert::CalculateSum8 (UINT8  *Buffer, UINT32  Size)
{
  UINT32 Index;
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


VOID KextConvert::Ascii2UnicodeString (CHAR8 *String, CHAR16   *UniString)
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

  Converts a string to an EFI_GUID.  The string must be in the
  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format.

--*/
EFI_STATUS KextConvert::StringToGuid (CHAR8 *AsciiGuidBuffer, EFI_GUID_  *GuidBuffer)
{
  INT32 Index;
  unsigned Data1;
  unsigned Data2;
  unsigned Data3;
  unsigned Data4[8];

  if (AsciiGuidBuffer == NULL || GuidBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check Guid Format strictly xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  //
  for (Index = 0; AsciiGuidBuffer[Index] != '\0' && Index < 37; Index ++) {
    if (Index == 8 || Index == 13 || Index == 18 || Index == 23) {
      if (AsciiGuidBuffer[Index] != '-') {
        break;
      }
    } else {
      if (((AsciiGuidBuffer[Index] >= '0') && (AsciiGuidBuffer[Index] <= '9')) ||
         ((AsciiGuidBuffer[Index] >= 'a') && (AsciiGuidBuffer[Index] <= 'f')) ||
         ((AsciiGuidBuffer[Index] >= 'A') && (AsciiGuidBuffer[Index] <= 'F'))) {
        continue;
      } else {
        break;
      }
    }
  }

  if (Index < 36 || AsciiGuidBuffer[36] != '\0') {
    printf("Invalid option value: Incorrect GUID \"%s\"\n  Correct Format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\"\n", AsciiGuidBuffer);
    return EFI_ABORTED;
  }

  //
  // Scan the guid string into the buffer
  //
  Index = sscanf (
            AsciiGuidBuffer,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            &Data1,
            &Data2,
            &Data3,
            &Data4[0],
            &Data4[1],
            &Data4[2],
            &Data4[3],
            &Data4[4],
            &Data4[5],
            &Data4[6],
            &Data4[7]
            );

  //
  // Verify the correct number of items were scanned.
  //
  if (Index != 11) {
    printf("Invalid option value: Incorrect GUID \"%s\"\n  Correct Format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\"", AsciiGuidBuffer);
    return EFI_ABORTED;
  }
  //
  // Copy the data into our GUID.
  //
  GuidBuffer->Data1     = (UINT32) Data1;
  GuidBuffer->Data2     = (UINT16) Data2;
  GuidBuffer->Data3     = (UINT16) Data3;
  GuidBuffer->Data4[0]  = (UINT8) Data4[0];
  GuidBuffer->Data4[1]  = (UINT8) Data4[1];
  GuidBuffer->Data4[2]  = (UINT8) Data4[2];
  GuidBuffer->Data4[3]  = (UINT8) Data4[3];
  GuidBuffer->Data4[4]  = (UINT8) Data4[4];
  GuidBuffer->Data4[5]  = (UINT8) Data4[5];
  GuidBuffer->Data4[6]  = (UINT8) Data4[6];
  GuidBuffer->Data4[7]  = (UINT8) Data4[7];

  return EFI_SUCCESS;
}

EFI_STATUS KextConvert::GetSectionContents(QByteArray input[], UINT32 *InputFileAlign, UINT32 InputFileNum,
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

      FileSize = input[Index].size();
      dprintf("the input section is size %u bytes\n", (unsigned) FileSize);

    //
    // Check this section is Te/Pe section, and Calculate the numbers of Te/Pe section.
    //
    TeOffset = 0;
    if (FileSize >= MAX_FFS_SIZE) {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER2);
    } else {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER);
    }
      memcpy(&TempSectHeader,input[Index].data(), HeaderSize);
    if (TempSectHeader.Type == EFI_SECTION_TE) {
      (*PESectionNum) ++;
        memcpy(&TeHeader, &input[Index].data()[HeaderSize], sizeof(TeHeader));
      if (TeHeader.Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
        TeOffset = TeHeader.StrippedSize - sizeof (TeHeader);
      }
    } else if (TempSectHeader.Type == EFI_SECTION_PE32) {
      (*PESectionNum) ++;
    } else if (TempSectHeader.Type == EFI_SECTION_GUID_DEFINED) {
      if (FileSize >= MAX_SECTION_SIZE) {
          memcpy(&GuidSectHeader2, input[Index].data(), sizeof (GuidSectHeader2));
        if ((GuidSectHeader2.Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
          HeaderSize = GuidSectHeader2.DataOffset;
        }
      } else {
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
      dprintf("Pad raw section for section data alignment Pad Raw section size is %u\n", (unsigned) Offset);

      Size = Size + Offset;
    }

    //
    // Now read the contents of the file into the buffer
    // Buffer must be enough to contain the file content.
    //
    if ((FileSize > 0) && (FileBuffer != NULL) && ((Size + FileSize) <= *BufferLength)) {
        memcpy(&FileBuffer[Size], input[Index].data(), (size_t) FileSize);
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
UINT8 KextConvert::GenFFS(UINT8 type, QString GUID, QByteArray inputPE32, QByteArray userinterface, QByteArray & out)
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
    EFI_GUID_ FileGuid = {0, 0, 0 , 0};

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
    dprintf("FFS File Guid is %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
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
      dprintf("FFS File has the fixed file attribute\n");
    }
    if ((FfsAttrib & FFS_ATTRIB_CHECKSUM) != 0) {
      dprintf("FFS File requires the checksum of the whole file\n");
    }

    for (Index = 0; Index < InputFileNum; Index ++) {
      if (InputFileAlign[Index] == 0) {
        //
        // Minimum alignment is 1 byte.
        //
        InputFileAlign[Index] = 1;
      }
      dprintf("the %dth input section alignment is %u\n", Index, (unsigned) InputFileAlign[Index]);
    }

    //
    // Calculate the size of all input section files.
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
        printf("Resource memory cannot be allocated!\n");
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
    memcpy (&FfsFileHeader.Name, &FileGuid, sizeof (EFI_GUID_));
    FfsFileHeader.Type       = FfsFiletype;
    //
    // Update FFS Alignment based on the max alignment required by input section files
    //
    dprintf("the max alignment of all input sections is %u\n", (unsigned) MaxAlignment);
    for (Index = 0; Index < sizeof (mFfsValidAlign) / sizeof (UINT32) - 1; Index ++) {
      if ((MaxAlignment > mFfsValidAlign [Index]) && (MaxAlignment <= mFfsValidAlign [Index + 1])) {
        break;
      }
    }
    if (FfsAlign < Index) {
      FfsAlign = Index;
    }
    dprintf("the alignment of the generated FFS file is %u\n", (unsigned) mFfsValidAlign [FfsAlign + 1]);

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
    dprintf("the size of the generated FFS file is %u bytes\n", (unsigned) FileSize);

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
    // write header
    //
    out.clear();
    out.reserve(FileSize);
    out.append((const char*)&FfsFileHeader, HeaderSize);
    //
    // write data
    //
    out.append((const char*)FileBuffer, FileSize-HeaderSize);

    if (InputFileAlign != NULL) {
      free (InputFileAlign);
    }
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }

    return STATUS_SUCCESS;
}

UINT8 KextConvert::GenSectionUserInterface(QString name, QByteArray &out)
{
    UINT8 *Buffer;
    UINT32 Index;
    EFI_USER_INTERFACE_SECTION *UiSect;


    Index = sizeof (EFI_COMMON_SECTION_HEADER);    

    // StringBuffer is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
    Index += (strlen(name.toLocal8Bit().constData()) * 2) +2;

    Buffer = (UINT8 *) malloc (Index);
    if (Buffer == NULL) {
      printf("Resource memory cannot be allcoated\n");
      return STATUS_ERROR;
    }

    UiSect = (EFI_USER_INTERFACE_SECTION *) Buffer;
    UiSect->CommonHeader.Type     = EFI_SECTION_USER_INTERFACE;
    UiSect->CommonHeader.Size[0]  = (UINT8) (Index & 0xff);
    UiSect->CommonHeader.Size[1]  = (UINT8) ((Index & 0xff00) >> 8);
    UiSect->CommonHeader.Size[2]  = (UINT8) ((Index & 0xff0000) >> 16);
    Ascii2UnicodeString (name.toLocal8Bit().data(), UiSect->FileNameString);

    dprintf("the size of the created section file is %u bytes\n", (unsigned) Index);

    out.clear();
    out.reserve(Index);
    out.append((const char*)Buffer, Index);

    return STATUS_SUCCESS;
}


UINT8 KextConvert::GenSectionPE32 (QByteArray inputbinary, QByteArray & out)
{
  UINT32                    InputFileLength;
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
  dprintf("the size of the created section file is %u bytes\n", (unsigned) TotalLength);
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
  out.append((const char*) Buffer, TotalLength);

  return STATUS_SUCCESS;
}

UINT8 KextConvert::createFFS(QString name, QString GUID, QByteArray inputbinary, QByteArray & output)
{
    QByteArray out;
    QByteArray pe32;
    QByteArray userinterface;

    UINT32 InputLength;
    EFI_STATUS Status;
    EFI_COMMON_SECTION_HEADER *SectionHeader;

    //
    // GenSec -s EFI_SECTION_PE32 -o pe32 inputbinary
    //
    out.clear();
    Status = GenSectionPE32(inputbinary, out);

    if (Status != STATUS_SUCCESS) {
        printf("Status is not successful: Status value is 0x%X\n", (int) Status);
        return STATUS_ERROR;
    }

    // Get output file length
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)out.data();
    InputLength = *(UINT32 *)SectionHeader->Size & 0x00ffffff;
    if (InputLength == 0xffffff) {
        InputLength = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
    }

    // Write the output file
    pe32 = out.left(InputLength);

    //
    // GenSec -s EFI_SECTION_USER_INTERFACE -n basename+version -o userinterface
    //
    out.clear();
    Status = GenSectionUserInterface(name, out);

    if (Status != STATUS_SUCCESS) {
        printf("Status is not successful: Status value is 0x%X\n", (int) Status);
        return STATUS_ERROR;
    }

    // Get output file length
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)out.data();
    InputLength = *(UINT32 *)SectionHeader->Size & 0x00ffffff;
    if (InputLength == 0xffffff) {
        InputLength = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
    }

    dprintf("Userinterface InputLength: %i\n", InputLength);

    // Write the output file
    userinterface = out.left(InputLength);

    //
    // GenFfs -t EFI_FV_FILETYPE_DRIVER -g GUID -o output -i pe32 -i userinterface
    //
    out.clear();

    //Status = GenFFS(EFI_FV_FILETYPE_DRIVER, GUID, pe32, userinterface, out);
    Status = GenFFS(EFI_FV_FILETYPE_FREEFORM, GUID, pe32, userinterface, out);
    if (Status != STATUS_SUCCESS) {
        printf("Status is not successful: Status value is 0x%X\n", (int) Status);
        return STATUS_ERROR;
    }

    output.clear();
    output.append(out);

    return STATUS_SUCCESS;
}
