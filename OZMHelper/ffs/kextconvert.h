#ifndef KEXTCONVERT_H
#define KEXTCONVERT_H

#include <QString>
#include <QByteArray>
#include <Common/UefiBaseTypes.h>

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
    EFI_STATUS StringToGuid (CHAR8 *AsciiGuidBuffer, EFI_GUID_  *GuidBuffer);
    EFI_STATUS GetSectionContents(QByteArray input[], UINT32 *InputFileAlign, UINT32 InputFileNum,
                                         UINT8 *FileBuffer, UINT32  *BufferLength, UINT32 *MaxAlignment, UINT8 *PESectionNum);
    UINT8 GenFFS(UINT8 type, QString GUID, QByteArray inputPE32, QByteArray userinterface, QByteArray & out);
    UINT8 GenSectionUserInterface(QString name, QByteArray &out);
    UINT8 GenSectionPE32 (QByteArray inputbinary, QByteArray & out);
};

#endif // KEXTCONVERT_H
