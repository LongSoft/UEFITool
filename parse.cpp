/* parse.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "parse.h"

// Implementation of GNU memmem function using Boyer-Moore-Horspool algorithm 
UINT8* find_pattern(UINT8* string, UINT32 slen, CONST UINT8* pattern, UINT32 plen)
{
    UINT32 scan = 0;
    UINT32 bad_char_skip[256];
    UINT32 last;

    if (plen == 0 || !string || !pattern)
        return NULL;

    for (scan = 0; scan <= 255; scan++)
        bad_char_skip[scan] = plen;

    last = plen - 1;

    for (scan = 0; scan < last; scan++)
        bad_char_skip[pattern[scan]] = last - scan;

    while (slen >= plen)
    {
        for (scan = last; string[scan] == pattern[scan]; scan--)
            if (scan == 0)
                return string;

        slen     -= bad_char_skip[string[last]];
        string   += bad_char_skip[string[last]];
    }

    return NULL;
}

UINT8 write_to_file(UINT8* buffer, CONST size_t size, const char* name, const char* extension)
{
    /*char str[_MAX_PATH]; // Buffer for file name construction
    FILE* file;

    sprintf(str, "%s.%s", name, extension);     
    file = fopen((const char*) str, "wb");
    if (!file) {
        printf("Can't create %s file\n", str);
        return ERR_FILE_OPEN;
    }
    if (fwrite(buffer, sizeof(UINT8), size, file) != size) {
        printf("Can't write %s file\n", str);
        return ERR_FILE_WRITE;
    }
    fclose(file);*/
    return ERR_SUCCESS;
}

UINT8* parse_region(UINT8* buffer, size_t size, CONST BOOLEAN two_flash_chips, const char* region_name, CONST UINT16 region_base, CONST UINT16 region_limit)
{
    UINT8 result       = ERR_SUCCESS;
    UINT8* region      = NULL;
    size_t region_size = calculate_region_size(region_base, region_limit);

    if (!buffer || !size || !region_name || !region_size) 
        return NULL;

    // Check region base to be in buffer
    if (region_base >= size)
    {
        printf("Warning: %s region stored in descriptor is not found\n", region_name);
        if (two_flash_chips) 
            printf("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %s region\n", region_name);
        else
            printf("Error: one flash chip installed, so it is an error caused by damaged or incomplete dump\n");
        printf("Warning: absence of %s region assumed\n", region_name);
        return NULL;
    }
    // Check region to be fully present in buffer
    else if (region_base + region_size > size)
    {
        printf("Warning: %s region stored in descriptor overlaps the end of opened file\n", region_name);
        if (two_flash_chips) 
            printf("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %s region\n", region_name);
        else
            printf("Error: One flash chip installed, so it is an error caused by damaged or incomplete dump\n");
        printf("Warning: absence of %s region assumed\n", region_name);
        return NULL;
    }

    // Calculate region address
    region = calculate_address_16(buffer, region_base);

    // Display information about region
    printf("%s region found, size: %d (0x%08X)\n", region_name, region_size, region_size);

    // Write all regions that has non-zero size to files
    result = write_to_file(region, region_size, region_name, "region");
    if (result){
        printf("Error: Can't write %s region to file\n", region_name);
        return NULL;
    }
    printf("%s region written to file\n", region_name);

    return region;
}   

UINT8* find_next_volume(UINT8* buffer, size_t buffer_size)
{
    UINT8* pointer;
    if (!buffer || !buffer_size)
        return NULL;

    pointer = find_pattern(buffer, buffer_size, FV_SIGNATURE_STR, FV_SIGNATURE_LENGTH);
    if (pointer && pointer - FV_SIGNATURE_OFFSET >= buffer)
        return pointer - FV_SIGNATURE_OFFSET;

    return NULL;
}

UINT8 calculate_checksum_8(UINT8* buffer, size_t buffer_size)
{
    UINT8 counter = 0;
    while(buffer_size--)
        counter += buffer[buffer_size];
    return ~counter + 1;
}

UINT16 calculate_checksum_16(UINT8* buffer, size_t buffer_size)
{
    UINT16 counter = 0;
    while(buffer_size--)
        counter += buffer[buffer_size];
    return ~counter + 1;
}

// Format GUID to standard readable form
// Assumes str to have at least 37 bytes free
UINT8 format_guid(EFI_GUID* guid, char* str)
{
    UINT8*  guid1    = (UINT8*) guid;
    UINT8*  guid2    = guid1 + 4;
    UINT8*  guid3    = guid1 + 6;
    UINT32* data1    = (UINT32*) guid1;
    UINT16* data2    = (UINT16*) guid2;
    UINT16* data3    = (UINT16*) guid3;
    UINT8*  data41   = guid1 + 8;
    UINT8*  data42   = guid1 + 9;
    UINT8*  data51   = guid1 + 10;
    UINT8*  data52   = guid1 + 11;
    UINT8*  data53   = guid1 + 12;
    UINT8*  data54   = guid1 + 13;
    UINT8*  data55   = guid1 + 14;
    UINT8*  data56   = guid1 + 15;

    sprintf(str, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", *data1, *data2, *data3, *data41, *data42,
        *data51, *data52, *data53, *data54, *data55, *data56);

    return ERR_SUCCESS;
}

VOID size_to_filesize(size_t size, UINT8* module_size)
{
    module_size[2] = (UINT8) ((size) >> 16);
    module_size[1] = (UINT8) ((size) >>  8);
    module_size[0] = (UINT8) ((size)      );
}

size_t filesize_to_size(UINT8* module_size)
{
    return (module_size[2] << 16) + 
        (module_size[1] << 8)  + 
        module_size[0];
}

UINT8 parse_volume(UINT8* buffer, size_t buffer_size, const char* prefix)
{
    EFI_FIRMWARE_VOLUME_HEADER* volume_header;
    UINT8* current_file;
    char str[_MAX_PATH];
    char guid[37]; // FD44820B-F1AB-41C0-AE4E-0C55556EB9BD for example
    UINT8 empty_byte;
    UINT8 result;
    size_t header_size;
    size_t file_size;
    size_t files_counter = 0;
    
    volume_header = (EFI_FIRMWARE_VOLUME_HEADER*) buffer;
    if (volume_header->Signature != FV_SIGNATURE_INT)
        return ERR_INVALID_VOLUME;

    //!TODO: Check zero vector for code, so it could be a boot volume
    
    // Check filesystem GUID to be known
    // Check for FFS1
    if (find_pattern((UINT8*)&volume_header->FileSystemGuid, sizeof(EFI_GUID), EFI_FIRMWARE_FILE_SYSTEM_GUID, sizeof(EFI_GUID)) == (UINT8*)&volume_header->FileSystemGuid) {
        printf("File system: PI FFS v1\n");
    // Check for FFS2
    } else if (find_pattern((UINT8*)&volume_header->FileSystemGuid, sizeof(EFI_GUID), EFI_FIRMWARE_FILE_SYSTEM2_GUID, sizeof(EFI_GUID)) == (UINT8*)&volume_header->FileSystemGuid) {
        printf("File system: PI FFS v2\n");
    // Unknown FFS
    } else {
        printf("File system: unknown\nWarning: volume can't be parsed\n");
        return ERR_UNKNOWN_FFS;
    }

    // Check attributes
    // Determine erase polarity
    if (volume_header->Attributes & EFI_FVB_ERASE_POLARITY)
        empty_byte = 0xFF;
    else
        empty_byte = 0x00;
    printf("Empty byte: 0x%02X\n", empty_byte);

    // Check header checksum by recalculating it
    if (!calculate_checksum_16(buffer, volume_header->HeaderLength)) {
        printf("Warning: volume header checksum is invalid\nVolume can be inacessible for UEFI routines\n");
    }

    // Check for presence of extended header, only if header revision is not 1
    if (volume_header->Revision > 1 && volume_header->ExtHeaderOffset) {
        EFI_FIRMWARE_VOLUME_EXT_HEADER* extended_header;
        extended_header = (EFI_FIRMWARE_VOLUME_EXT_HEADER*) (buffer + volume_header->ExtHeaderOffset);
        result = format_guid(&extended_header->FvName, guid);
        if (result)
            return result;
        printf("Extended header: present\n"
               "Volume GUID: %s", guid);
        header_size = volume_header->ExtHeaderOffset + extended_header->ExtHeaderSize;
    } else {
        printf("Extended header: none\n");
        header_size = volume_header->HeaderLength;
    }

    //!TODO: Check block map to make enough space for the whole volume
    
    // Find and parse all files in current volume
    for(current_file = buffer + header_size; current_file && current_file < buffer + volume_header->FvLength - header_size;) {
        EFI_FFS_FILE_HEADER* file_header = (EFI_FFS_FILE_HEADER*) current_file;
        UINT8* byte;
        size_t empty_bytes_counter = 0;
        // Check if no more files left
        for (byte = current_file; *byte == empty_byte; byte++) {
            empty_bytes_counter++;
            if(empty_bytes_counter >= sizeof(EFI_FFS_FILE_HEADER))
            {
                printf("No more files left in current volume\n");
                return ERR_SUCCESS;
            }
        }
                
        result = format_guid(&file_header->Name, guid);
        if (result)
            return result;
        printf("FFS file %s found\n", guid);
        
        // Constructing file name
        sprintf(str, "%s.%d.%s", prefix, files_counter, guid);
        
        // Getting file size
        file_size = filesize_to_size(file_header->Size);
        
        // Store current volume to file
        result = write_to_file(current_file, file_size,
                str, "file");
        if (result){
            printf("Error: Can't write FFS file to file\n");
            return result;
        }
        printf("FFS file %s stored to file\n", guid);

        //!TODO: Parse current file

        // Parsed
        printf("FFS file %s parsed\n", guid);
        current_file += ALIGN_8(file_size);
        files_counter++;
    }

    return ERR_SUCCESS;
}

UINT8 parse_bios_space(UINT8* buffer, size_t buffer_size)
{
    UINT8* current_pointer                    = NULL;
    UINT8* previous_pointer                   = NULL;
    size_t previous_volume_size               = 0;
    EFI_FIRMWARE_VOLUME_HEADER* volume_header = NULL;
    UINT32 files_counter                      = 0;
    size_t rest_size                          = 0;
    UINT8  result                             = 0;
    char str[_MAX_PATH];

    // Search for all firmware volume headers
    for (current_pointer = buffer, previous_pointer = NULL; current_pointer && current_pointer - buffer <= buffer_size;) {
        rest_size = buffer_size - (current_pointer - buffer);
        // Search for next firmware volume
        //!TODO: add checks for incomplete file
        previous_pointer = current_pointer;
        current_pointer = find_next_volume(current_pointer, rest_size);
        if (current_pointer) {
            // Check for padding between previous and current volumes
            if (previous_pointer + previous_volume_size < current_pointer) {
                sprintf(str, "%d", files_counter); 
                result = write_to_file(previous_pointer + previous_volume_size, 
                    current_pointer - previous_pointer - previous_volume_size,
                    str, "padding");
                if (result){
                    printf("Error: Can't write padding to file\n");
                    return result;
                }
                printf("Padding between firmware volumes stored to file\n\n");
                files_counter++;
            }

            volume_header = (EFI_FIRMWARE_VOLUME_HEADER*) current_pointer;
            printf("Firmware volume found\n");

            // Check if volume overlaps the end if buffer
            if (current_pointer + volume_header->FvLength > buffer + buffer_size) {
                printf("Error: Volume overlaps the end of input buffer.\n");
                return ERR_INVALID_VOLUME;
            }

            // Constructing volume name
            sprintf(str, "%d", files_counter); 
            
            // Store current volume to file
            result = write_to_file(current_pointer, volume_header->FvLength,
                str, "volume");
            if (result){
                printf("Error: Can't write volume to file\n");
                return result;
            }
            printf("Volume stored to file\n");

            // Parse volume
            result = parse_volume(current_pointer, volume_header->FvLength, str);
            if (result)
                return result;
            printf("Firmware volume parsed\n\n");

            // Move to next volume
            current_pointer += volume_header->FvLength;
            files_counter++;
        }
        else  {// Previous_pointer was the last one (or there was none at all)
            // Check for padding between previous volume and end of buffer
            if (previous_pointer + previous_volume_size < buffer + buffer_size) {
                char str[256];
                sprintf(str, "%d", files_counter); 
                result = write_to_file(previous_pointer + previous_volume_size, 
                    buffer_size + buffer - previous_pointer,
                    str, "padding");
                if (result){
                    printf("Error: Can't write padding to file\n");
                    return result;
                }
                printf("Padding between last firmware volume and end of buffer stored to file\n\n");
                files_counter++;
            }
        }
    }

    return ERR_SUCCESS;
}

UINT8 parse_buffer(UINT8* buffer, size_t buffer_size)
{
    UINT8*                    original_buffer     = buffer;
    size_t                    original_size       = buffer_size;
    size_t                    capsule_header_size = 0;
    size_t                    flash_image_size    = 0;
    FLASH_DESCRIPTOR_HEADER*  descriptor_header   = NULL;
    UINT8                     result              = ERR_SUCCESS;

    // Check buffer to be valid
    if (!buffer || !buffer_size)
        return ERR_INVALID_PARAMETER;

    // Check buffer for being normal EFI capsule header
    if (find_pattern(buffer, sizeof(EFI_GUID), EFI_CAPSULE_GUID, sizeof(EFI_GUID)) == buffer) {
        EFI_CAPSULE_HEADER* capsule_header = (EFI_CAPSULE_HEADER*) buffer;
        printf("EFI Capsule header found\n"
            "Header size: %d (0x%08X)\n"
            "Header flags: %08X\n"
            "Image size: %d (0x%08X)\n\n", 
            capsule_header->HeaderSize,
            capsule_header->HeaderSize, 
            capsule_header->Flags,
            capsule_header->CapsuleImageSize,
            capsule_header->CapsuleImageSize); 
        capsule_header_size = capsule_header->HeaderSize;
    }

    // Check buffer for being extended Aptio capsule header
    else if (find_pattern(buffer, sizeof(EFI_GUID), APTIO_CAPSULE_GUID, sizeof(EFI_GUID)) == buffer) {
        APTIO_CAPSULE_HEADER* aptio_capsule_header = (APTIO_CAPSULE_HEADER*) buffer;
        printf("AMI Aptio extended capsule header found\n"
            "Header size: %d (0x%08X)\n"
            "Header flags: %08X\n"
            "Image size: %d (0x%08X)\n\n",
            aptio_capsule_header->RomImageOffset,
            aptio_capsule_header->RomImageOffset,
            aptio_capsule_header->CapsuleHeader.Flags,
            aptio_capsule_header->CapsuleHeader.CapsuleImageSize - aptio_capsule_header->RomImageOffset,
            aptio_capsule_header->CapsuleHeader.CapsuleImageSize - aptio_capsule_header->RomImageOffset);
        //!TODO: add more information about extended headed here
        capsule_header_size = aptio_capsule_header->RomImageOffset;
    }

    // Skip capsule header
    buffer += capsule_header_size;
    flash_image_size = buffer_size - capsule_header_size;

    // Consider buffer to be normal flash image without any headers now

    // Check buffer for being Intel flash descriptor
    descriptor_header = (FLASH_DESCRIPTOR_HEADER*) buffer;
    // Check descriptor signature
    if (descriptor_header->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        FLASH_DESCRIPTOR_MAP*               descriptor_map;
        FLASH_DESCRIPTOR_COMPONENT_SECTION* component_section;
        FLASH_DESCRIPTOR_REGION_SECTION*    region_section;
        FLASH_DESCRIPTOR_MASTER_SECTION*    master_section;
        // Vector of 16 0xFF for sanity check      
        CONST UINT8 FFVector[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; 
        // Store the beginning of descriptor as descriptor base address
        UINT8* descriptor  = buffer;
        UINT8* bios_region = NULL;

        printf("Intel flash descriptor found\n");
        // Check for buffer size to be greater or equal to descriptor region size
        if (flash_image_size < FLASH_DESCRIPTOR_SIZE) {
            printf("Error: Input file is smaller then mininum descriptor size of 4KB\nAborting\n");
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }

        //Write descriptor to desc.bin
        result = write_to_file(descriptor, FLASH_DESCRIPTOR_SIZE, "DESC", "region");
        if (result){
            printf("Error: Can't write descriptor region to file\nAborting\n");
            return result;
        }
        printf("Descriptor region written to file\n");

        // Check FFVector to consist 16 0xFF, as sanity check
        if (find_pattern(buffer, 16, FFVector, 16) != buffer)
            printf("Warning: Descriptor start vector has something but 0xFF bytes. That's strange\n");

        // Skip descriptor header
        buffer += sizeof(FLASH_DESCRIPTOR_HEADER);

        // Parse descriptor map
        descriptor_map = (FLASH_DESCRIPTOR_MAP*) buffer;
        component_section = (FLASH_DESCRIPTOR_COMPONENT_SECTION*) calculate_address_8(descriptor, descriptor_map->ComponentBase);
        region_section = (FLASH_DESCRIPTOR_REGION_SECTION*) calculate_address_8(descriptor, descriptor_map->RegionBase);
        master_section = (FLASH_DESCRIPTOR_MASTER_SECTION*) calculate_address_8(descriptor, descriptor_map->MasterBase);
        printf("\nParsing descriptor map\n"
            "Flash chips: %d\n"
            "Flash regions: %d\n"
            "Flash masters: %d\n"
            "PCH straps: %d\n"
            "PROC straps: %d\n"
            "ICC table entries: %d\n",
            descriptor_map->NumberOfFlashChips + 1,
            descriptor_map->NumberOfRegions + 1,
            descriptor_map->NumberOfMasters + 1,
            descriptor_map->NumberOfPchStraps,
            descriptor_map->NumberOfProcStraps,
            descriptor_map->NumberOfIccTableEntries);
        printf("Descriptor map parsed\n\n");

        // Parse component section
        //!TODO: add parsing code
        // Parse master section
        //!TODO: add parsing code

        // Parse region section
        printf("Parsing region section\n");
        parse_region(descriptor, flash_image_size, descriptor_map->NumberOfFlashChips, "GBE", region_section->GbeBase, region_section->GbeLimit);
        parse_region(descriptor, flash_image_size, descriptor_map->NumberOfFlashChips, "ME", region_section->MeBase, region_section->MeLimit);
        bios_region = parse_region(descriptor, flash_image_size, descriptor_map->NumberOfFlashChips, "BIOS", region_section->BiosBase, region_section->BiosLimit);
        parse_region(descriptor, flash_image_size, descriptor_map->NumberOfFlashChips, "PDR", region_section->PdrBase, region_section->PdrLimit);
        printf("Region section parsed\n\n");

        // Parsing complete
        printf("Descriptor parsing complete\n\n");

        // Exiting if no bios region found
        if (!bios_region) {
            printf("BIOS region not found\nAborting\n");
            return ERR_BIOS_REGION_NOT_FOUND;
        }

        // BIOS region is our buffer now
        buffer = bios_region;
        buffer_size = calculate_region_size(region_section->BiosBase, region_section->BiosLimit);
    }
    else {
        printf("Intel flash descriptor not found, assuming that input file is BIOS region image\n\n");
    }

    // We are in the beginning of BIOS space, where firmware volumes are
    // Parse BIOS space
    
    result = parse_bios_space(buffer, buffer_size);
    if (result)
        return result;

    return ERR_SUCCESS;
}
