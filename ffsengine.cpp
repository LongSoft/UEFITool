/* ffsengine.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <math.h>

#include "ffsengine.h"
#include "treeitem.h"
#include "treemodel.h"
#include "treeitemtypes.h"
#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "Tiano/EfiTianoCompress.h"
#include "Tiano/EfiTianoDecompress.h"
#include "LZMA/LzmaCompress.h"
#include "LZMA/LzmaDecompress.h"

FfsEngine::FfsEngine(QObject *parent)
    : QObject(parent)
{
    rootItem = new TreeItem(RootItem, 0, 0, tr("Object"), tr("Type"), tr("Subtype"), tr("Text"));
    treeModel = new TreeModel(rootItem);
}

FfsEngine::~FfsEngine(void)
{
    delete treeModel;
    delete rootItem;
}

TreeModel* FfsEngine::model() const
{
    return treeModel;
}

QString FfsEngine::message() const
{
    return text;
}

void FfsEngine::msg(const QString & message)
{
    text.append(message).append("\n");
}

UINT8 FfsEngine::parseInputFile(const QByteArray & buffer)
{
    UINT32                   capsuleHeaderSize = 0;
    FLASH_DESCRIPTOR_HEADER* descriptorHeader = NULL;
    QByteArray               flashImage;
    QByteArray               bios;
    QModelIndex              index;

    // Check buffer size to be more or equal then sizeof(EFI_CAPSULE_HEADER)
    if (buffer.size() <= sizeof(EFI_CAPSULE_HEADER))
    {
        msg(tr("parseInputFile: Input file is smaller then mininum size of %1 bytes").arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    // Check buffer for being normal EFI capsule header
    if (buffer.startsWith(EFI_CAPSULE_GUID)) {
        EFI_CAPSULE_HEADER* capsuleHeader = (EFI_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        index = addTreeItem(CapsuleItem, UefiCapsule, 0, header, body);
    }
    
    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_CAPSULE_GUID)) {
        APTIO_CAPSULE_HEADER* aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = aptioCapsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        index = addTreeItem(CapsuleItem, AptioCapsule, 0, header, body);
    }

    // Skip capsule header to have flash chip image
    flashImage = buffer.right(buffer.size() - capsuleHeaderSize);
    
    // Check buffer for being Intel flash descriptor
    descriptorHeader = (FLASH_DESCRIPTOR_HEADER*) flashImage.constData();
    // Check descriptor signature
    if (descriptorHeader->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        FLASH_DESCRIPTOR_MAP*               descriptorMap;
        FLASH_DESCRIPTOR_REGION_SECTION*    regionSection;

        // Store the beginning of descriptor as descriptor base address
        UINT8* descriptor  = (UINT8*) flashImage.constData();
        UINT8* gbeRegion  = NULL;
        UINT8* meRegion   = NULL;
        UINT8* biosRegion = NULL;
        UINT8* pdrRegion  = NULL;

        // Check for buffer size to be greater or equal to descriptor region size
        if (flashImage.size() < FLASH_DESCRIPTOR_SIZE) {
            msg(tr("parseInputFile: Input file is smaller then mininum descriptor size of 4096 bytes"));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }

        // Parse descriptor map
        descriptorMap = (FLASH_DESCRIPTOR_MAP*) (flashImage.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
        regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*) calculateAddress8(descriptor, descriptorMap->RegionBase);
        
        // Add tree item
        QByteArray header = flashImage.left(sizeof(FLASH_DESCRIPTOR_HEADER));
        QByteArray body   = flashImage.mid(sizeof(FLASH_DESCRIPTOR_HEADER), FLASH_DESCRIPTOR_SIZE - sizeof(FLASH_DESCRIPTOR_HEADER));
        index = addTreeItem(DescriptorItem, 0, 0, header, body, index);

        // Parse region section
        QModelIndex gbeIndex(index);
        QModelIndex meIndex(index);
        QModelIndex biosIndex(index);
        QModelIndex pdrIndex(index);
        gbeRegion  = parseRegion(flashImage, GbeRegion,  regionSection->GbeBase,  regionSection->GbeLimit,  gbeIndex);
        meRegion   = parseRegion(flashImage, MeRegion,   regionSection->MeBase,   regionSection->MeLimit,   meIndex);
        biosRegion = parseRegion(flashImage, BiosRegion, regionSection->BiosBase, regionSection->BiosLimit, biosIndex);
        pdrRegion  = parseRegion(flashImage, PdrRegion,  regionSection->PdrBase,  regionSection->PdrLimit,  pdrIndex);
        
        // Parse complete
        // Exit if no bios region found
        if (!biosRegion) {
            msg(tr("parseInputFile: BIOS region not found"));
            return ERR_BIOS_REGION_NOT_FOUND;
        }

        index = biosIndex;
        bios = QByteArray::fromRawData((const char*) biosRegion, calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit));
    }
    else {
        bios = buffer;
    }

    // We are in the beginning of BIOS space, where firmware volumes are
    // Parse BIOS space
    
    return parseBios(bios, index);
}

UINT8* FfsEngine::parseRegion(const QByteArray & flashImage, UINT8 regionSubtype, const UINT16 regionBase, const UINT16 regionLimit, QModelIndex & index)
{
    // Check for empty region or flash image
    if (!regionLimit || flashImage.size() <= 0)
        return NULL;
    
    // Storing flash image size to unsigned variable, because it can't be negative now and all other values are unsigned  
    UINT32 flashImageSize = (UINT32) flashImage.size(); 

    // Calculate region offset and size
    UINT32 regionOffset = calculateRegionOffset(regionBase);
    UINT32 regionSize    = calculateRegionSize(regionBase, regionLimit);
    
    // Populate descriptor map 
    FLASH_DESCRIPTOR_MAP* descriptor_map = (FLASH_DESCRIPTOR_MAP*) (flashImage.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
    
    // Determine presence of 2 flash chips
    bool twoChips = descriptor_map->NumberOfFlashChips;
    
    // construct region name
    //!TODO: make this to regionTypeToQString(const UINT8 type) in descriptor.cpp
    QString regionName;
    switch (regionSubtype)
    {
    case GbeRegion:
        regionName = "GbE";
        break;
    case MeRegion:
        regionName = "ME";
        break;
    case BiosRegion:
        regionName = "Bios";
        break;
    case PdrRegion:
        regionName = "PDR";
        break;
    default:
        regionName = "Unknown";
        msg(tr("parseRegion: Unknown region type"));
    };

    // Check region base to be in buffer
    if (regionOffset >= flashImageSize)
    {
        msg(tr("parseRegion: %1 region stored in descriptor not found").arg(regionName));
        if (twoChips) 
            msg(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            msg(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        msg(tr("Absence of %1 region assumed").arg(regionName));
        return NULL;
    }

    // Check region to be fully present in buffer
    else if (regionOffset + regionSize > flashImageSize)
    {
        msg(tr("parseRegion: %1 region stored in descriptor overlaps the end of opened file").arg(regionName));
        if (twoChips) 
            msg(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            msg(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        msg(tr("Absence of %1 region assumed\n").arg(regionName));
        return NULL;
    }

    // Calculate region address
    UINT8* region = calculateAddress16((UINT8*) flashImage.constData(), regionBase);

    // Add tree item
    QByteArray body = flashImage.mid(regionOffset, regionSize);
    index = addTreeItem(RegionItem, regionSubtype, regionOffset, QByteArray(), body, index);

    return region;
}   

UINT8 FfsEngine::parseBios(const QByteArray & bios, const QModelIndex & parent)
{
    // Search for first volume
    INT32 prevVolumeOffset = findNextVolume(bios);
    
    // No volumes found
    if (prevVolumeOffset < 0) {
        return ERR_VOLUMES_NOT_FOUND;
    }

    // First volume is not at the beginning of bios space
    if (prevVolumeOffset > 0) {
        QByteArray padding = bios.left(prevVolumeOffset);
         addTreeItem(PaddingItem, 0, 0, QByteArray(), padding, parent);
    }

    // Search for and parse all volumes
    INT32 volumeOffset;
    UINT32 prevVolumeSize;
    for (volumeOffset = prevVolumeOffset, prevVolumeSize = 0; 
         volumeOffset >= 0; 
         prevVolumeOffset = volumeOffset, prevVolumeSize = getVolumeSize(bios, volumeOffset), volumeOffset = findNextVolume(bios, volumeOffset + prevVolumeSize)) 
    {
        // Padding between volumes
        if ((UINT32) volumeOffset > prevVolumeOffset + prevVolumeSize) { // Conversion to suppress warning, volumeOffset can't be negative here
            UINT32 size = volumeOffset - prevVolumeOffset - prevVolumeSize;
            QByteArray padding = bios.mid(prevVolumeOffset + prevVolumeSize, size);
            addTreeItem(PaddingItem, 0, prevVolumeOffset + prevVolumeSize, QByteArray(), padding, parent);
        } 
        
        // Populate volume header
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeOffset);
        
        //Check that volume is fully present in input
        if (volumeOffset + volumeHeader->FvLength > bios.size()) {
            msg(tr("parseBios: Volume overlaps the end of input buffer"));
            return ERR_INVALID_VOLUME;
        }
        
        // Check volume revision and alignment
        UINT32 alignment;
        if (volumeHeader->Revision == 1) {
            // Aquire alignment bits
            bool alignmentCap    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP;
            bool alignment2      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2;
            bool alignment4      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4;
            bool alignment8      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8;
            bool alignment16     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16;
            bool alignment32     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32;
            bool alignment64     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64;
            bool alignment128    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_128;
            bool alignment256    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_256;
            bool alignment512    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_512;
            bool alignment1k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_1K;
            bool alignment2k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2K;
            bool alignment4k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4K;
            bool alignment8k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8K;
            bool alignment16k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16K;
            bool alignment32k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32K;
            bool alignment64k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64K;

            // Check alignment setup
            if (!alignmentCap &&
            (  alignment2   || alignment4   || alignment8   || alignment16 
            || alignment32  || alignment64  || alignment128 || alignment256 
            || alignment512 || alignment1k  || alignment2k  || alignment4k
            || alignment8k  || alignment16k || alignment32k || alignment64k))
            msg("parseBios: Incompatible revision 1 volume alignment setup");

            // Assume that smaller alignment value consumes greater
            alignment = 0x01;
            if (alignment2)
                alignment = 0x02;
            else if (alignment4)
                alignment = 0x04;
            else if (alignment8)
                alignment = 0x08;
            else if (alignment16)
                alignment = 0x10;
            else if (alignment32)
                alignment = 0x20;
            else if (alignment64)
                alignment = 0x40;
            else if (alignment128)
                alignment = 0x80;
            else if (alignment256)
                alignment = 0x100;
            else if (alignment512)
                alignment = 0x200;
            else if (alignment1k)
                alignment = 0x400;
            else if (alignment2k)
                alignment = 0x800;
            else if (alignment4k)
                alignment = 0x1000;
            else if (alignment8k)
                alignment = 0x2000;
            else if (alignment16k)
                alignment = 0x4000;
            else if (alignment32k)
                alignment = 0x8000;
            else if (alignment64k)
                alignment = 0x10000;

            // Check alignment
            if (volumeOffset % alignment) {
                msg(tr("parseBios: Unaligned revision 1 volume"));
            }
        }
        else if (volumeHeader->Revision == 2) {
            // Aquire alignment
            alignment = pow(2, (volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
            
            // Check alignment
            if (volumeOffset % alignment) {
                msg(tr("parseBios: Unaligned revision 2 volume"));
            }
        }
        else
            msg(tr("parseBios: Unknown volume revision (%1)").arg(volumeHeader->Revision));
        
        // Check filesystem GUID to be known
        // Do not parse volume with unknown FFS, because parsing will fail
        bool parseCurrentVolume = true;
        // FFS GUID v1
        if (QByteArray((const char*) &volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM_GUID) {
            // Code can be added here
        }
        // FFS GUID v2
        else if (QByteArray((const char*) &volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM2_GUID) {
            // Code can be added here
        } 
        // Other GUID
        else {
            //info = info.append(tr("File system: unknown\n"));
            msg(tr("parseBios: Unknown file system (%1)").arg(guidToQString(volumeHeader->FileSystemGuid)));
            parseCurrentVolume = false;
        }

        // Check attributes
        // Determine erase polarity
        bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;

        // Check header checksum by recalculating it
        if (!calculateChecksum16((UINT8*) volumeHeader, volumeHeader->HeaderLength)) {
            msg(tr("parseBios: Volume header checksum is invalid"));
        }

        // Check for presence of extended header, only if header revision is not 1
        UINT32 headerSize;
        if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
            EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*) ((UINT8*) volumeHeader + volumeHeader->ExtHeaderOffset);
            headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
        } else {
            headerSize = volumeHeader->HeaderLength;
        }

        // Adding tree item
        QByteArray  header = bios.mid(volumeOffset, headerSize);
        QByteArray  body   = bios.mid(volumeOffset + headerSize, volumeHeader->FvLength - headerSize);
        QModelIndex index  = addTreeItem(VolumeItem, 0, volumeOffset, header, body, parent);

        // Parse volume
        if (parseCurrentVolume) {
            UINT32 result = parseVolume(bios.mid(volumeOffset + headerSize, volumeHeader->FvLength - headerSize), headerSize, volumeHeader->Revision, erasePolarity, index);
            if (result)
                msg(tr("parseBios: Volume parsing failed (%1)").arg(result));
        }
    }
    
    return ERR_SUCCESS;
}

INT32 FfsEngine::findNextVolume(const QByteArray & bios, INT32 volumeOffset)
{
    if (volumeOffset < 0)
        return -1;

    INT32 nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeOffset);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET) {
        return -1;
    }

    return nextIndex - EFI_FV_SIGNATURE_OFFSET;
}

UINT32 FfsEngine::getVolumeSize(const QByteArray & bios, INT32 volumeOffset)
{
    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeOffset);
    
    // Check volume signature
    if (QByteArray((const char*) &volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return 0;
    return volumeHeader->FvLength;
}

UINT8 FfsEngine::parseVolume(const QByteArray & volume, UINT32 volumeBase, UINT8 revision, bool erasePolarity, const QModelIndex & parent)
{
    // Construct empty byte based on erasePolarity value
    // Native char type is used because QByteArray.count() takes it
    char empty = erasePolarity ? '\xFF' : '\x00';
    
    // Search for and parse all files
    INT32 fileOffset = 0;
    while (fileOffset >= 0) {
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) (volume.constData() + fileOffset);
        QByteArray file = volume.mid(fileOffset, uint24ToUint32(fileHeader->Size));
        QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
        
		// Check file size to at least sizeof(EFI_FFS_FILE_HEADER)
		if (file.size() < sizeof(EFI_FFS_FILE_HEADER))
		{
			msg(tr("parseVolume: File with invalid size"));
			return ERR_INVALID_FILE;
		}

        // We are at empty space in the end of volume
        if (header.count(empty) == header.size()) {
            QByteArray body = volume.right(volume.size() - fileOffset);
            addTreeItem(PaddingItem, 0, fileOffset, QByteArray(), body, parent);
            break;
        }
                
        // Check header checksum
        QByteArray tempHeader = header;
        EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*) (tempHeader.data());
        tempFileHeader->IntegrityCheck.Checksum.Header = 0;
        tempFileHeader->IntegrityCheck.Checksum.File = 0;
        UINT8 calculated = calculateChecksum8((UINT8*) tempFileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);
        if (fileHeader->IntegrityCheck.Checksum.Header != calculated)
        {
            msg(tr("parseVolume: %1, stored header checksum %2 differs from calculated %3")
                    .arg(guidToQString(fileHeader->Name))
                    .arg(fileHeader->IntegrityCheck.Checksum.Header, 2, 16, QChar('0'))
                    .arg(calculated, 2, 16, QChar('0')));
        }

        // Check data checksum, if no tail was found
        // Data checksum must be calculated
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
            UINT32 bufferSize = file.size() - sizeof(EFI_FFS_FILE_HEADER);
            // Exclude file tail from data checksum calculation
            if(revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
                bufferSize -= sizeof(UINT16);
            calculated = calculateChecksum8((UINT8*)(file.constData() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
            if (fileHeader->IntegrityCheck.Checksum.File != calculated) {
                msg(tr("parseVolume: %1, stored data checksum %2 differs from calculated %3")
                    .arg(guidToQString(fileHeader->Name))
                    .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0'))
                    .arg(calculated, 2, 16, QChar('0')));
            }
        }
        // Data checksum must be one of predefined values
        else {
            if (fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM &&fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2) {
                msg(tr("parseVolume: %1, stored data checksum %2 differs from standard value")
                    .arg(guidToQString(fileHeader->Name))
                    .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0')));
            }
        }

        // Check file alignment
        UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
        UINT32 alignment = pow(2, alignmentPower);
        if ((volumeBase + fileOffset + sizeof(EFI_FFS_FILE_HEADER)) % alignment) {
            msg(tr("parseVolume: %1, unaligned file").arg(guidToQString(fileHeader->Name)));
        }

        // Get file body
        QByteArray body = file.right(file.size() - sizeof(EFI_FFS_FILE_HEADER));
        // For files in Revision 1 volumes, check for file tail presence
        if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
        {
            //Check file tail;
            UINT16* tail = (UINT16*) body.right(sizeof(UINT16)).constData();
            if (!fileHeader->IntegrityCheck.TailReference == *tail)
                msg(tr("parseVolume: %1, file tail value %2 is not a bitwise not of %3 stored in file header")
                .arg(guidToQString(fileHeader->Name))
                .arg(*tail, 4, 16, QChar('0'))
                .arg(fileHeader->IntegrityCheck.TailReference, 4, 16, QChar('0')));
            
            // Remove tail from file body
            body = body.left(body.size() - sizeof(UINT16));
        }

        // Parse current file by default
        bool parseCurrentFile = true;
        // Raw files can hide volumes inside them
        // So try to parse them as bios space
        bool parseAsBios = false;

        // Check file type
        //!TODO: add more file specific checks
        switch (fileHeader->Type)
        {
        case EFI_FV_FILETYPE_ALL:
            parseAsBios = true;
            break;
        case EFI_FV_FILETYPE_RAW:
            parseAsBios = true;
            break;
        case EFI_FV_FILETYPE_FREEFORM:
            break;
        case EFI_FV_FILETYPE_SECURITY_CORE:
            break;
        case EFI_FV_FILETYPE_PEI_CORE:
            break;
        case EFI_FV_FILETYPE_DXE_CORE:
            break;
        case EFI_FV_FILETYPE_PEIM:
            break;
        case EFI_FV_FILETYPE_DRIVER:
            break;
        case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
            break;
        case EFI_FV_FILETYPE_APPLICATION:
            break;
        case EFI_FV_FILETYPE_SMM:
            break;
        case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
            break;
        case EFI_FV_FILETYPE_COMBINED_SMM_DXE:
            break;
        case EFI_FV_FILETYPE_SMM_CORE:
            break;
        case EFI_FV_FILETYPE_PAD:
            parseCurrentFile = false;
            break;
        default:
            parseCurrentFile = false;
            msg(tr("parseVolume: Unknown file type (%1)").arg(fileHeader->Type, 2, 16, QChar('0')));
        };

        // Check for empty file
        if (body.count(empty) == body.size())
        {
            // No need to parse empty files
            parseCurrentFile = false;
        }

        // Add tree item
        QModelIndex index = addTreeItem(FileItem, fileHeader->Type, fileOffset, header, body, parent);
        
        // Parse file
        if (parseCurrentFile) {
            if (parseAsBios) {
                UINT32 result = parseBios(body, index);
                if (result && result != ERR_VOLUMES_NOT_FOUND)
                    msg(tr("parseVolume: Parse file as BIOS failed (%1)").arg(result));
            }
            else {
                UINT32 result = parseFile(body, revision, erasePolarity, index);
                if (result)
                    msg(tr("parseVolume: Parse file as FFS failed (%1)").arg(result));
            }
        }

        // Move to next file
        fileOffset += file.size();
        fileOffset = ALIGN8(fileOffset);
        
        // Exit from loop if no files left
        if (fileOffset >= volume.size())
            fileOffset = -1;
    }
    
    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseFile(const QByteArray & file, UINT8 revision, bool erasePolarity, const QModelIndex & parent)
{
    // Search for and parse all sections
    INT32 sectionOffset = 0;
    while(sectionOffset >= 0)
    {
        EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) (file.constData() + sectionOffset);
        UINT32 sectionSize = uint24ToUint32(sectionHeader->Size);
        
        // This declarations must be here because of the nature of switch statement 
        EFI_COMPRESSION_SECTION* compressedSectionHeader;
        EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader;
        QByteArray header;
        QByteArray body;
        UINT32 decompressedSize;
        UINT32 scratchSize;
        UINT8* decompressed;
        UINT8* scratch;
        VOID*  data;
        UINT32 dataSize;
        QModelIndex index;
        UINT32 result;
		UINT32 shittySectionSize;
		EFI_COMMON_SECTION_HEADER* shittySectionHeader;

        switch (sectionHeader->Type)
        {
        // Encapsulated sections
        case EFI_SECTION_COMPRESSION:
            compressedSectionHeader = (EFI_COMPRESSION_SECTION*) sectionHeader;
            header = file.mid(sectionOffset, sizeof(EFI_COMPRESSION_SECTION));

            // Try to decompress this section
            switch (compressedSectionHeader->CompressionType)
            {
            case EFI_NOT_COMPRESSED:
                body  = file.mid(sectionOffset + sizeof(EFI_COMPRESSION_SECTION), compressedSectionHeader->UncompressedLength);
                index = addTreeItem(SectionItem, CompressionSection, sectionOffset, header, body, parent);
                // Parse stored file
                result = parseFile(body, revision, erasePolarity, index);
                if (result)
                    msg(tr("parseFile: Stored section can not be parsed as file (%1)").arg(result));
                break;
            case EFI_STANDARD_COMPRESSION:
                //Must be Tiano for all revisions, needs checking
                body  = file.mid(sectionOffset + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);

                // Get buffer sizes
                data = (VOID*) (file.constData() + sectionOffset + sizeof(EFI_COMPRESSION_SECTION));
                dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION);
                if (TianoGetInfo(data, dataSize, &decompressedSize, &scratchSize) != ERR_SUCCESS
                    ||  decompressedSize != compressedSectionHeader->UncompressedLength)
                    msg(tr("parseFile: TianoGetInfo failed"));
                else {
                    decompressed = new UINT8[decompressedSize];
                    scratch = new UINT8[scratchSize];
                    // Decompress section data
                    if (TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize) != ERR_SUCCESS)
                        msg(tr("parseFile: TianoDecompress failed"));
                    else
                    {
                        body = QByteArray::fromRawData((const char*) decompressed, decompressedSize);
                        // Parse stored file
                        result = parseFile(body, revision, erasePolarity, index);
                        if (result)
                            msg(tr("parseFile: Compressed section with Tiano compression can not be parsed as file (%1)").arg(result));
                    }

                    delete[] decompressed;
                    delete[] scratch;
                }
                break;
            case EFI_CUSTOMIZED_COMPRESSION:
                body  = file.mid(sectionOffset + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);

                // Get buffer sizes
                data = (VOID*) (file.constData() + sectionOffset + sizeof(EFI_COMPRESSION_SECTION));
                dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION);
                if (LzmaGetInfo(data, dataSize, &decompressedSize) != ERR_SUCCESS
                    ||  decompressedSize != compressedSectionHeader->UncompressedLength)
                {
                    // Shitty file with a section header between COMPRESSED_SECTION_HEADER and LZMA_HEADER
					// We must determine section header size by checking it's type before we can unpack that non-standard compressed section
					shittySectionHeader = (EFI_COMMON_SECTION_HEADER*) data;
					shittySectionSize = sizeOfSectionHeaderOfType(shittySectionHeader->Type);
					data = (VOID*) (file.constData() + sectionOffset + sizeof(EFI_COMPRESSION_SECTION) + shittySectionSize);
					dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION) - shittySectionSize;
					if (LzmaGetInfo(data, dataSize, &decompressedSize) != ERR_SUCCESS)
						msg(tr("parseFile: LzmaGetInfo failed"));
                }   

				decompressed = new UINT8[decompressedSize];
				
				// Decompress section data
				if (LzmaDecompress(data, dataSize, decompressed) != ERR_SUCCESS)
					msg(tr("parseFile: LzmaDecompress failed"));
				else
				{
					body = QByteArray::fromRawData((const char*) decompressed, decompressedSize);
					// Parse stored file
					result = parseFile(body, revision, erasePolarity, index);
					if (result)
						msg(tr("parseFile: Compressed section with LZMA compression can not be parsed as file (%1)").arg(result));
				}

				delete[] decompressed;
                break;
            default:
                body  = file.mid(sectionOffset + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
                msg(tr("parseFile: Compressed section with unknown compression type found (%1)").arg(compressedSectionHeader->CompressionType));
            }

            break;
        case EFI_SECTION_GUID_DEFINED:
            header = file.mid(sectionOffset, sizeof(EFI_GUID_DEFINED_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_GUID_DEFINED_SECTION), sectionSize - sizeof(EFI_GUID_DEFINED_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            // Parse section body as file
            guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*) (header.constData());
            body   = file.mid(sectionOffset + guidDefinedSectionHeader->DataOffset, sectionSize - guidDefinedSectionHeader->DataOffset); 
            result = parseFile(body, revision, erasePolarity, index);
            if (result)
                msg(tr("parseFile: GUID defined section can not be parsed as file (%1)").arg(result));
            break;
        case EFI_SECTION_DISPOSABLE:
            header = file.mid(sectionOffset, sizeof(EFI_DISPOSABLE_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_DISPOSABLE_SECTION), sectionSize - sizeof(EFI_DISPOSABLE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        // Leaf sections
        case EFI_SECTION_PE32:
            header = file.mid(sectionOffset, sizeof(EFI_PE32_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_PE32_SECTION), sectionSize - sizeof(EFI_PE32_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_PIC:
            header = file.mid(sectionOffset, sizeof(EFI_PIC_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_PIC_SECTION), sectionSize - sizeof(EFI_PIC_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_TE:
            header = file.mid(sectionOffset, sizeof(EFI_TE_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_TE_SECTION), sectionSize - sizeof(EFI_TE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_VERSION:
            header = file.mid(sectionOffset, sizeof(EFI_VERSION_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_VERSION_SECTION), sectionSize - sizeof(EFI_VERSION_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_USER_INTERFACE:
            header = file.mid(sectionOffset, sizeof(EFI_USER_INTERFACE_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_USER_INTERFACE_SECTION), sectionSize - sizeof(EFI_USER_INTERFACE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_COMPATIBILITY16:
            header = file.mid(sectionOffset, sizeof(EFI_COMPATIBILITY16_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_COMPATIBILITY16_SECTION), sectionSize - sizeof(EFI_COMPATIBILITY16_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
            header = file.mid(sectionOffset, sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION), sectionSize - sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            // Parse section body as BIOS space
            result = parseBios(body, index);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                msg(tr("parseFile: Firmware volume image can not be parsed (%1)").arg(result));
            break;
        case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
            header = file.mid(sectionOffset, sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION), sectionSize - sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_RAW:
            header = file.mid(sectionOffset, sizeof(EFI_RAW_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_RAW_SECTION), sectionSize - sizeof(EFI_RAW_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            // Parse section body as BIOS space
            result = parseBios(body, index);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                msg(tr("parseFile: Raw section can not be parsed as BIOS (%1)").arg(result));
            break;
            break;
        case EFI_SECTION_DXE_DEPEX:
            header = file.mid(sectionOffset, sizeof(EFI_DXE_DEPEX_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_DXE_DEPEX_SECTION), sectionSize - sizeof(EFI_DXE_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_PEI_DEPEX:
            header = file.mid(sectionOffset, sizeof(EFI_PEI_DEPEX_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_PEI_DEPEX_SECTION), sectionSize - sizeof(EFI_PEI_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        case EFI_SECTION_SMM_DEPEX:
            header = file.mid(sectionOffset, sizeof(EFI_SMM_DEPEX_SECTION));
            body   = file.mid(sectionOffset + sizeof(EFI_SMM_DEPEX_SECTION), sectionSize - sizeof(EFI_SMM_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
            break;
        default:
            msg(tr("parseFile: Section with unknown type (%1)").arg(sectionHeader->Type, 2, 16, QChar('0')));
            header = file.mid(sectionOffset, sizeof(EFI_COMMON_SECTION_HEADER));
            body   = file.mid(sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER), sectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
            index  = addTreeItem(SectionItem, sectionHeader->Type, sectionOffset, header, body, parent);
        }
        
        // Move to next section
        sectionOffset += uint24ToUint32(sectionHeader->Size);
        sectionOffset = ALIGN4(sectionOffset);

        // Exit from loop if no sections left
        if (sectionOffset >= file.size())
            sectionOffset = -1;
    }

    return ERR_SUCCESS;
}

bool FfsEngine::hasEmptyHeader(const QModelIndex& index) const
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyHeader();
}

bool FfsEngine::hasEmptyBody(const QModelIndex& index) const
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyBody();
}

QModelIndex FfsEngine::addTreeItem(const UINT8 type, const UINT8 subtype, const UINT32 offset, const QByteArray &header, const QByteArray &body, const QModelIndex &parent)
{
    TreeItem *parentItem;
    int parentColumn = 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
    {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
        parentColumn = parent.column();
    }
    
    // All information extraction must be here
    QString name, typeName, subtypeName, info;
    EFI_CAPSULE_HEADER*                 capsuleHeader;
    APTIO_CAPSULE_HEADER*               aptioCapsuleHeader;
    FLASH_DESCRIPTOR_MAP*               descriptorMap;
    //FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection;
    //FLASH_DESCRIPTOR_REGION_SECTION*    regionSection;
    //FLASH_DESCRIPTOR_MASTER_SECTION*    masterSection;
    GBE_MAC* gbeMac;   
    GBE_VERSION* gbeVersion;
    ME_VERSION* meVersion;
    INT32 meVersionOffset;
    EFI_FIRMWARE_VOLUME_HEADER*         volumeHeader;
    EFI_FFS_FILE_HEADER*                fileHeader;
    //EFI_COMMON_SECTION_HEADER*          sectionHeader;
    
    switch (type)
    {
    case RootItem:
        // Do not allow to add another root item
        return QModelIndex();
        break;
    case CapsuleItem:
        //typeName = tr("Capsule");
        switch (subtype)
        {
        case AptioCapsule:
            name = tr("AMI Aptio capsule");
            aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*) header.constData();
            info = tr("Offset: %1\nHeader size: %2\nFlags: %3\nImage size: %4")
                .arg(offset, 8, 16, QChar('0'))
                .arg(aptioCapsuleHeader->RomImageOffset, 4, 16, QChar('0'))
                .arg(aptioCapsuleHeader->CapsuleHeader.Flags, 8, 16, QChar('0'))
                .arg(aptioCapsuleHeader->CapsuleHeader.CapsuleImageSize - aptioCapsuleHeader->RomImageOffset, 8, 16, QChar('0'));
            //!TODO: more info about Aptio capsule
            break;
        case UefiCapsule:
            name = tr("UEFI capsule");
            capsuleHeader = (EFI_CAPSULE_HEADER*) header.constData();
            info = tr("Offset: %1\nHeader size: %2\nFlags: %3\nImage size: %4")
                .arg(offset, 8, 16, QChar('0'))
                .arg(capsuleHeader->HeaderSize, 8, 16, QChar('0'))
                .arg(capsuleHeader->Flags, 8, 16, QChar('0'))
                .arg(capsuleHeader->CapsuleImageSize, 8, 16, QChar('0'));
            break;
        default:
            name = tr("Unknown capsule");
            info = tr("Offset: %1\nGUID: %2\n")
                .arg(offset, 8, 16, QChar('0'))
                .arg(guidToQString(*(EFI_GUID*)header.constData()));
            break;
        }
        break;
    case DescriptorItem:
        name = tr("Descriptor");
        descriptorMap = (FLASH_DESCRIPTOR_MAP*) body.constData();
        info = tr("Flash chips: %1\nRegions: %2\nMasters: %3\nPCH straps:%4\nPROC straps: %5\nICC table entries: %6")
            .arg(descriptorMap->NumberOfFlashChips + 1) //
            .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
            .arg(descriptorMap->NumberOfMasters + 1)    //
            .arg(descriptorMap->NumberOfPchStraps)
            .arg(descriptorMap->NumberOfProcStraps)
            .arg(descriptorMap->NumberOfIccTableEntries);
        //!TODO: more info about descriptor
        break;
    case RegionItem:
        typeName = tr("Region");
        info = tr("Offset: %1\nSize: %2")
            .arg(offset, 8, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));
        switch (subtype)
        {
        case GbeRegion:
            name = tr("GbE region");
            gbeMac = (GBE_MAC*) body.constData();
            gbeVersion = (GBE_VERSION*) (body.constData() + GBE_VERSION_OFFSET);
            info += tr("\nMAC: %1:%2:%3:%4:%5:%6\nVersion: %7.%8")
                .arg(gbeMac->vendor[0], 2, 16, QChar('0'))
                .arg(gbeMac->vendor[1], 2, 16, QChar('0'))
                .arg(gbeMac->vendor[2], 2, 16, QChar('0'))
                .arg(gbeMac->device[0], 2, 16, QChar('0'))
                .arg(gbeMac->device[1], 2, 16, QChar('0'))
                .arg(gbeMac->device[2], 2, 16, QChar('0'))
                .arg(gbeVersion->major)
                .arg(gbeVersion->minor);
            break;
        case MeRegion:
            name = tr("ME region");
            meVersionOffset = body.indexOf(ME_VERSION_SIGNATURE);
            if (meVersionOffset < 0){
                info += tr("\nVersion: unknown");
                msg(tr("addTreeItem: ME region version is unknown, it can be damaged"));
            }
            else {
                meVersion = (ME_VERSION*) (body.constData() + meVersionOffset);
                info += tr("\nVersion: %1.%2.%3.%4")
                    .arg(meVersion->major)
                    .arg(meVersion->minor)
                    .arg(meVersion->bugfix)
                    .arg(meVersion->build);
            }
            break;
        case BiosRegion:
            name = tr("BIOS region");
            break;
        case PdrRegion:
            name = tr("PDR region");
            break;
        default:
            name = tr("Unknown region");
            msg(tr("addTreeItem: Unknown region"));
            break;
        }
        break;
    case PaddingItem:
        name = tr("Padding");
        info = tr("Offset: %1\nSize: %2")
            .arg(offset, 8, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));
        break;
    case VolumeItem:
        typeName = tr("Volume");
        // Parse volume header to determine its revision and file system
        volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) header.constData();
        name = guidToQString(volumeHeader->FileSystemGuid);
        subtypeName = tr("Revision %1").arg(volumeHeader->Revision);
        info = tr("Offset: %1\nSize: %2\nAttributes: %3\nHeader size: %4")
            .arg(offset, 8, 16, QChar('0'))
            .arg(volumeHeader->FvLength, 8, 16, QChar('0'))
            .arg(volumeHeader->Attributes, 8, 16, QChar('0'))
            .arg(volumeHeader->HeaderLength, 4, 16, QChar('0'));
        break;
    case FileItem:
        typeName = tr("File");
        // Parse file header to determine its GUID and type
        fileHeader = (EFI_FFS_FILE_HEADER*) header.constData();
        name = guidToQString(fileHeader->Name);
        subtypeName = fileTypeToQString(subtype);
        info = tr("Offset: %1\nType: %2\nAttributes: %3\nSize: %4\nState: %5")
            .arg(offset, 8, 16, QChar('0'))
            .arg(fileHeader->Type, 2, 16, QChar('0'))
            .arg(fileHeader->Attributes, 2, 16, QChar('0'))
            .arg(uint24ToUint32(fileHeader->Size), 6, 16, QChar('0'))
            .arg(fileHeader->State, 2, 16, QChar('0'));
        break;
    case SectionItem:
        typeName = tr("Section");
        name = sectionTypeToQString(subtype) + tr(" section");
        info = tr("Offset: %1\nSize: %2")
            .arg(offset, 8, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));
        //!TODO: add more specific info for all section types with uncommon headers
        // Set name of file
        if (subtype == UserInterfaceSection)
        {
            QString text = QString::fromUtf16((const ushort*)body.constData());
            setTreeItemText(text, findParentOfType(FileItem, parent));
        }
        break;
    default:
        name = tr("Unknown");
        info = tr("Offset: %1").arg(offset, 8, 16, QChar('0'));
        break;
    }
        
    return treeModel->addItem(type, subtype, offset, name, typeName, subtypeName, "", info, header, body, parent);
}

bool FfsEngine::setTreeItemName(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;
    
    return treeModel->setItemName(data, index);
}

bool FfsEngine::setTreeItemText(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;

    return treeModel->setItemText(data, index);
}

bool FfsEngine::removeItem(const QModelIndex &index)
{
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->parent()->removeChild(item);
    delete item;
    return true;
}

QModelIndex FfsEngine::findParentOfType(UINT8 type, const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

    TreeItem *item;
    QModelIndex parent = index;

    for(item = static_cast<TreeItem*>(parent.internalPointer()); 
        item != NULL && item != rootItem && item->type() != type;
        item = static_cast<TreeItem*>(parent.internalPointer()))
            parent = parent.parent();
    if (item != NULL && item != rootItem)
        return parent;
    
    return QModelIndex();
}

QByteArray FfsEngine::header(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->header();
}

QByteArray FfsEngine::body(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->body();
}

QByteArray FfsEngine::uncompressFile(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    // Check index item to be FFS file
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if(item->type() != FileItem)
        return QByteArray();

    QByteArray file;
    UINT32 offset = 0;
    // Construct new item body
    for (int i = 0; i < item->childCount(); i++)
    {
        // If section is not compressed, add it to new body as is
        TreeItem* sectionItem = item->child(i);
        if (sectionItem->subtype() != CompressionSection)
        {
            QByteArray section = sectionItem->header().append(sectionItem->body());
            UINT32 align = ALIGN4(offset) - offset;
            file.append(QByteArray(align, '\x00')).append(section);
            offset += align + section.size();
        }
        else {
            // Construct new section body by adding all child sections to this new section
            QByteArray section;
            UINT32 subOffset = 0;
            for (int j = 0; j < sectionItem->childCount(); j++)
            {
                TreeItem* subSectionItem = sectionItem->child(j);
                QByteArray subSection = subSectionItem->header().append(subSectionItem->body());
                UINT32 align = ALIGN4(subOffset) - subOffset;
                section.append(QByteArray(align, '\x00')).append(subSection);
                subOffset += align + subSection.size();
            }
            // Add newly constructed section to file body
            
            EFI_COMPRESSION_SECTION sectionHeader;
            sectionHeader.Type = EFI_SECTION_COMPRESSION;
            sectionHeader.CompressionType = EFI_NOT_COMPRESSED;
            sectionHeader.UncompressedLength = section.size();
            uint32ToUint24(section.size() + sizeof(EFI_COMPRESSION_SECTION), sectionHeader.Size);
            UINT32 align = ALIGN4(offset) - offset;
            file.append(QByteArray(align, '\x00'))
                .append(QByteArray((const char*) &sectionHeader, sizeof(EFI_COMPRESSION_SECTION)))
                .append(section);
            offset += align + section.size();
        }
    }
    
    QByteArray header = item->header();
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) header.data();

    // Correct file data checksum, if needed
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM)
    {
        UINT32 bufferSize = file.size() - sizeof(EFI_FFS_FILE_HEADER);
        fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)(file.data() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
    }

    // Add file tail, if needed
    if(fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
        file.append(!fileHeader->IntegrityCheck.TailReference);

    return header.append(file);
}

bool FfsEngine::isCompressedFile(const QModelIndex& index) const
{
    if (!index.isValid())
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if(item->type() != FileItem)
        return false;

    for (int i = 0; i < item->childCount(); i++)
    {
        if (item->child(i)->subtype() == CompressionSection)
            return true;
    }

    return false;
}