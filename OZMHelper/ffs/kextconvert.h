#include <QString>
#include <QFileInfo>
#include <QByteArray>

#ifndef KEXTCONVERT_H
#define KEXTCONVERT_H

#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <IndustryStandard/PeImage.h>

#include "CommonLib.h"
#include "Compress.h"
#include "Crc32.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"

/* Kext

FakeSMC.kext 1
Disabler.kext 2
Injector.kext 3
Other.kext 6 - infinite

BASE GUID "DADE100$2-1B31-4FE4-8557-26FCEFC78275"

-- EFI --

from 20 (0x14) ++
BASE GUID "DADE10$2-1B31-4FE4-8557-26FCEFC78275"

-- OZM PLIST --

STATIC GUID "99F2839C-57C3-411E-ABC3-ADE5267D960D"

*/

class KextConvert
{
public:
    KextConvert();
    ~KextConvert();

    UINT8 createFFS(QString name, QString GUID, QByteArray inputbinary, QByteArray & output);

private:
    UINT8 CalculateChecksum8 (UINT8 *Buffer,UINT32 Size);
    UINT8 CalculateSum8 (UINT8  *Buffer, UINT32  Size);
    VOID Ascii2UnicodeString (CHAR8 *String, CHAR16   *UniString);
    UINT8 StringToGuid (CHAR8 *AsciiGuidBuffer, EFI_GUID_  *GuidBuffer);
    EFI_STATUS GetSectionContents(QByteArray input[], UINT32 *InputFileAlign, UINT32 InputFileNum,
                                         UINT8 *FileBuffer, UINT32  *BufferLength, UINT32 *MaxAlignment, UINT8 *PESectionNum);
    UINT8 GenFFS(UINT8 type, QString GUID, QByteArray inputPE32, QByteArray userinterface, QByteArray & out);
    UINT8 GenSectionUserInterface(QString name, QByteArray &out);
    UINT8 GenSectionPE32 (QByteArray inputbinary, QByteArray & out);
};

#endif // KEXTCONVERT_H
