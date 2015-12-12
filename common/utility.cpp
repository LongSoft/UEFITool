/* utility.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QObject>
#include "treemodel.h"
#include "utility.h"
#include "ffs.h"
#include "Tiano/EfiTianoCompress.h"
#include "Tiano/EfiTianoDecompress.h"
#include "LZMA/LzmaCompress.h"
#include "LZMA/LzmaDecompress.h"

// Returns either new parsing data instance or obtains it from index
PARSING_DATA parsingDataFromQModelIndex(const QModelIndex & index)
{
    if (index.isValid()) {
        TreeModel* model = (TreeModel*)index.model();
        return *(PARSING_DATA*)model->parsingData(index).data();
    }

    PARSING_DATA data;
    data.fixed = FALSE;     // Item is not fixed by default
    data.isOnFlash = TRUE;  // Data is on flash by default
    data.offset = 0;
    data.address = 0;
    data.ffsVersion = 0;    // Unknown by default

    // Type-specific parts remain unitialized
    return data;
}

// Converts parsing data to byte array
QByteArray parsingDataToQByteArray(const PARSING_DATA & pdata)
{
    return QByteArray((const char*)&pdata, sizeof(PARSING_DATA));
}

// Returns text representation of error code
QString errorCodeToQString(UINT8 errorCode)
{
    switch (errorCode) {
    case ERR_SUCCESS:                         return QObject::tr("Success");
    case ERR_NOT_IMPLEMENTED:                 return QObject::tr("Not implemented");
    case ERR_INVALID_PARAMETER:               return QObject::tr("Function called with invalid parameter");
    case ERR_BUFFER_TOO_SMALL:                return QObject::tr("Buffer too small");
    case ERR_OUT_OF_RESOURCES:                return QObject::tr("Out of resources");
    case ERR_OUT_OF_MEMORY:                   return QObject::tr("Out of memory");
    case ERR_FILE_OPEN:                       return QObject::tr("File can't be opened");
    case ERR_FILE_READ:                       return QObject::tr("File can't be read");
    case ERR_FILE_WRITE:                      return QObject::tr("File can't be written");
    case ERR_ITEM_NOT_FOUND:                  return QObject::tr("Item not found");
    case ERR_UNKNOWN_ITEM_TYPE:               return QObject::tr("Unknown item type");
    case ERR_INVALID_FLASH_DESCRIPTOR:        return QObject::tr("Invalid flash descriptor");
    case ERR_INVALID_REGION:                  return QObject::tr("Invalid region");
    case ERR_EMPTY_REGION:                    return QObject::tr("Empty region");
    case ERR_BIOS_REGION_NOT_FOUND:           return QObject::tr("BIOS region not found");
    case ERR_VOLUMES_NOT_FOUND:               return QObject::tr("UEFI volumes not found");
    case ERR_INVALID_VOLUME:                  return QObject::tr("Invalid UEFI volume");
    case ERR_VOLUME_REVISION_NOT_SUPPORTED:   return QObject::tr("Volume revision not supported");
    //case ERR_VOLUME_GROW_FAILED:              return QObject::tr("Volume grow failed");
    case ERR_UNKNOWN_FFS:                     return QObject::tr("Unknown file system");
    case ERR_INVALID_FILE:                    return QObject::tr("Invalid file");
    case ERR_INVALID_SECTION:                 return QObject::tr("Invalid section");
    case ERR_UNKNOWN_SECTION:                 return QObject::tr("Unknown section");
    case ERR_STANDARD_COMPRESSION_FAILED:     return QObject::tr("Standard compression failed");
    case ERR_CUSTOMIZED_COMPRESSION_FAILED:   return QObject::tr("Customized compression failed");
    case ERR_STANDARD_DECOMPRESSION_FAILED:   return QObject::tr("Standard decompression failed");
    case ERR_CUSTOMIZED_DECOMPRESSION_FAILED: return QObject::tr("Customized compression failed");
    case ERR_UNKNOWN_COMPRESSION_TYPE:        return QObject::tr("Unknown compression type");
    case ERR_UNKNOWN_EXTRACT_MODE:            return QObject::tr("Unknown extract mode");
    //case ERR_UNKNOWN_INSERT_MODE:             return QObject::tr("Unknown insert mode");
    case ERR_UNKNOWN_IMAGE_TYPE:              return QObject::tr("Unknown executable image type");
    case ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE: return QObject::tr("Unknown PE optional header type");
    case ERR_UNKNOWN_RELOCATION_TYPE:         return QObject::tr("Unknown relocation type");
    //case ERR_GENERIC_CALL_NOT_SUPPORTED:      return QObject::tr("Generic call not supported");
    //case ERR_VOLUME_BASE_NOT_FOUND:           return QObject::tr("Volume base address not found");
    //case ERR_PEI_CORE_ENTRY_POINT_NOT_FOUND:  return QObject::tr("PEI core entry point not found");
    case ERR_COMPLEX_BLOCK_MAP:               return QObject::tr("Block map structure too complex for correct analysis");
    case ERR_DIR_ALREADY_EXIST:               return QObject::tr("Directory already exists");
    case ERR_DIR_CREATE:                      return QObject::tr("Directory can't be created");
    //case ERR_UNKNOWN_PATCH_TYPE:              return QObject::tr("Unknown patch type");
    //case ERR_PATCH_OFFSET_OUT_OF_BOUNDS:      return QObject::tr("Patch offset out of bounds");
    //case ERR_INVALID_SYMBOL:                  return QObject::tr("Invalid symbol");
    //case ERR_NOTHING_TO_PATCH:                return QObject::tr("Nothing to patch");
    case ERR_DEPEX_PARSE_FAILED:              return QObject::tr("Dependency expression parsing failed");
    case ERR_TRUNCATED_IMAGE:                 return QObject::tr("Image is truncated");
    case ERR_INVALID_CAPSULE:                 return QObject::tr("Invalid capsule");
    default:                                  return QObject::tr("Unknown error %1").arg(errorCode);
    }
}

// CRC32 implementation
UINT32 crc32(UINT32 initial, const UINT8* buffer, UINT32 length)
{
    static const UINT32 crcTable[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535,
        0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD,
        0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D,
        0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
        0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC,
        0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB,
        0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
        0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB,
        0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA,
        0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE,
        0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
        0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409,
        0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739,
        0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268,
        0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
        0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8,
        0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
        0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703,
        0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7,
        0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE,
        0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6,
        0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D,
        0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
        0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
        0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };
    UINT32 crc32;
    UINT32 i;

    // Accumulate crc32 for buffer
    crc32 = initial ^ 0xFFFFFFFF;
    for (i = 0; i < length; i++) {
        crc32 = (crc32 >> 8) ^ crcTable[(crc32 ^ buffer[i]) & 0xFF];
    }

    return(crc32 ^ 0xFFFFFFFF);
}

// Compression routines
STATUS decompress(const QByteArray & compressedData, UINT8 & algorithm, QByteArray & decompressedData)
{
    const UINT8* data;
    UINT32 dataSize;
    UINT8* decompressed;
    UINT32 decompressedSize = 0;
    UINT8* scratch;
    UINT32 scratchSize = 0;
    const EFI_TIANO_HEADER* header;

    switch (algorithm)
    {
    case EFI_NOT_COMPRESSED:
        decompressedData = compressedData;
        algorithm = COMPRESSION_ALGORITHM_NONE;
        return ERR_SUCCESS;
    case EFI_STANDARD_COMPRESSION:
        // Set default algorithm to unknown
        algorithm = COMPRESSION_ALGORITHM_UNKNOWN;

        // Get buffer sizes
        data = (UINT8*)compressedData.data();
        dataSize = compressedData.size();

        // Check header to be valid
        header = (const EFI_TIANO_HEADER*)data;
        if (header->CompSize + sizeof(EFI_TIANO_HEADER) != dataSize)
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Get info function is the same for both algorithms
        if (ERR_SUCCESS != EfiTianoGetInfo(data, dataSize, &decompressedSize, &scratchSize))
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Allocate memory
        decompressed = new UINT8[decompressedSize];
        scratch = new UINT8[scratchSize];

        // Decompress section data

        //TODO: separate EFI1.1 from Tiano another way
        // Try Tiano decompression first
        if (ERR_SUCCESS != TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
            // Not Tiano, try EFI 1.1
            if (ERR_SUCCESS != EfiDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
                delete[] decompressed;
                delete[] scratch;
                return ERR_STANDARD_DECOMPRESSION_FAILED;
            }
            else algorithm = COMPRESSION_ALGORITHM_EFI11;
        }
        else algorithm = COMPRESSION_ALGORITHM_TIANO;

        decompressedData = QByteArray((const char*)decompressed, decompressedSize);

        delete[] decompressed;
        delete[] scratch;
        return ERR_SUCCESS;
    case EFI_CUSTOMIZED_COMPRESSION:
        // Set default algorithm to unknown
        algorithm = COMPRESSION_ALGORITHM_UNKNOWN;

        // Get buffer sizes
        data = (const UINT8*)compressedData.constData();
        dataSize = compressedData.size();

        // Get info
        if (ERR_SUCCESS != LzmaGetInfo(data, dataSize, &decompressedSize))
            return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;

        // Allocate memory
        decompressed = new UINT8[decompressedSize];

        // Decompress section data
        if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)) {
            // Intel modified LZMA workaround
            // Decompress section data once again
            data += sizeof(UINT32);

            // Get info again
            if (ERR_SUCCESS != LzmaGetInfo(data, dataSize, &decompressedSize)) {
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }

            // Decompress section data again
            if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)) {
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }
            else {
                algorithm = COMPRESSION_ALGORITHM_IMLZMA;
                decompressedData = QByteArray((const char*)decompressed, decompressedSize);
            }
        }
        else {
            algorithm = COMPRESSION_ALGORITHM_LZMA;
            decompressedData = QByteArray((const char*)decompressed, decompressedSize);
        }

        delete[] decompressed;
        return ERR_SUCCESS;
    default:
        algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
        return ERR_UNKNOWN_COMPRESSION_TYPE;
    }
}

// 8bit checksum calculation routine
UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize)
{
    if (!buffer)
        return 0;

    UINT8 counter = 0;

    while (bufferSize--)
        counter += buffer[bufferSize];

    return (UINT8)(0x100 - counter);
}

// 16bit checksum calculation routine
UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize)
{
    if (!buffer)
        return 0;

    UINT16 counter = 0;
    UINT32 index = 0;

    bufferSize /= sizeof(UINT16);

    for (; index < bufferSize; index++) {
        counter = (UINT16)(counter + buffer[index]);
    }

    return (UINT16)(0x10000 - counter);
}