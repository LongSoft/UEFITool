/* uefitool.cpp

  Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefitool.h"
#include "treeitemtypes.h"
#include "ui_uefitool.h"

UEFITool::UEFITool(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UEFITool)
{
    ui->setupUi(this);
    treeModel = NULL;

    // Signal-slot connections
    connect(ui->fromFileButton, SIGNAL(clicked()), this, SLOT(openImageFile()));
    connect(ui->exportAllButton, SIGNAL(clicked()), this, SLOT(saveAll()));
    connect(ui->exportBodyButton, SIGNAL(clicked()), this, SLOT(saveBody()));

    // Enable Drag-and-Drop actions
    this->setAcceptDrops(true);

    // Initialise non-persistent data
    init();
}

UEFITool::~UEFITool()
{
    delete ui;
    delete treeModel;
}

void UEFITool::init()
{
    // Clear UI components
    ui->debugEdit->clear();
    ui->infoEdit->clear();
    ui->exportAllButton->setDisabled(true);
    ui->exportBodyButton->setDisabled(true);
    // Make new tree model
    TreeModel * newModel = new TreeModel(this);
    ui->structureTreeView->setModel(newModel);
    if (treeModel)
        delete treeModel;
    treeModel = newModel;
    // Show info after selection the item in tree view
    connect(ui->structureTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(populateUi(const QModelIndex &)));
    //connect(ui->structureTreeView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    connect(ui->structureTreeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    resizeTreeViewColums();
}

void UEFITool::populateUi(const QModelIndex &current/*, const QModelIndex &previous*/)
{
    //!TODO: make widget
    currentIndex = current;
    ui->infoEdit->setPlainText(current.data(Qt::UserRole).toString());
    ui->exportAllButton->setDisabled(treeModel->hasEmptyBody(current) && treeModel->hasEmptyHeader(current));
    ui->exportBodyButton->setDisabled(treeModel->hasEmptyHeader(current));
}

void UEFITool::resizeTreeViewColums(/*const QModelIndex &index*/)
{
    int count = treeModel->columnCount();
    for(int i = 0; i < count; i++)
        ui->structureTreeView->resizeColumnToContents(i);
}

void UEFITool::openImageFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file"),".","BIOS image file (*.rom *.bin *.cap *.bio *.fd *.wph *.efi);;All files (*.*)");
    openImageFile(path);
}

void UEFITool::openImageFile(QString path)
{
    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists())
    {
        ui->statusBar->showMessage(tr("Please select existing BIOS image file."));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for reading. Check file permissions."));
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    init();
    UINT8 result = parseInputFile(buffer);
    if (result)
        debug(tr("Opened file can't be parsed as UEFI image (%1)").arg(result));
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
    resizeTreeViewColums();
}

void UEFITool::saveAll()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save header to binary file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }

    outputFile.write(treeModel->header(currentIndex) + treeModel->body(currentIndex));
    outputFile.close();
}

void UEFITool::saveBody()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save body to binary file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }

    outputFile.write(treeModel->body(currentIndex));
    outputFile.close();
}

/*void UEFITool::saveImageFile()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save BIOS image file"),".","BIOS image file (*.rom *.bin *.cap *.fd *.fwh);;All files (*.*)");

    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists())
    {
        ui->statusBar->showMessage(tr("Please select existing BIOS image file."));
        return;
    }

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::ReadWrite))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }
}*/

void UEFITool::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
             event->acceptProposedAction();
}

void UEFITool::dropEvent(QDropEvent* event)
{
    QString path = event->mimeData()->urls().at(0).toLocalFile();
    openImageFile(path);
}

void UEFITool::debug(const QString & text)
{
    //!TODO: log to separate debug window
    ui->debugEdit->appendPlainText(text);
}

QModelIndex UEFITool::addTreeItem(UINT8 type, UINT8 subtype, 
    const QByteArray & header, const QByteArray & body, const QModelIndex & parent)
{
    return treeModel->addItem(type, subtype, header, body, parent);
}

UINT8 UEFITool::parseInputFile(const QByteArray & buffer)
{
    UINT32                   capsuleHeaderSize = 0;
    FLASH_DESCRIPTOR_HEADER* descriptorHeader = NULL;
    QByteArray               flashImage;
    QByteArray               bios;
    QModelIndex              index;

    // Check buffer for being normal EFI capsule header
    if (buffer.startsWith(EFI_CAPSULE_GUID)) {
        EFI_CAPSULE_HEADER* capsuleHeader = (EFI_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        index = addTreeItem(CapsuleItem, UefiCapsule, header, body);
    }
    
    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_CAPSULE_GUID)) {
        APTIO_CAPSULE_HEADER* aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = aptioCapsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        index = addTreeItem(CapsuleItem, AptioCapsule, header, body);
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
            debug(tr("Input file is smaller then mininum descriptor size of 4KB"));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }

        // Parse descriptor map
        descriptorMap = (FLASH_DESCRIPTOR_MAP*) (flashImage.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
        regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*) calculateAddress8(descriptor, descriptorMap->RegionBase);
        
        // Add tree item
        QByteArray header = flashImage.left(sizeof(FLASH_DESCRIPTOR_HEADER));
        QByteArray body   = flashImage.mid(sizeof(FLASH_DESCRIPTOR_HEADER), FLASH_DESCRIPTOR_SIZE - sizeof(FLASH_DESCRIPTOR_HEADER));
        index = addTreeItem(DescriptorItem, 0, header, body, index);

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
        //!TODO: show some info about GbE, ME and PDR regions if found

        // Exit if no bios region found
        if (!biosRegion) {
            debug(tr("BIOS region not found"));
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

UINT8* UEFITool::parseRegion(const QByteArray & flashImage, UINT8 regionSubtype, const UINT16 regionBase, const UINT16 regionLimit, QModelIndex & index)
{
    // Check for empty region
    if (!regionLimit)
        return NULL;
    
    // Calculate region size
    UINT32 regionSize    = calculateRegionSize(regionBase, regionLimit);

    // Populate descriptor map 
    FLASH_DESCRIPTOR_MAP* descriptor_map = (FLASH_DESCRIPTOR_MAP*) flashImage.constData() + sizeof(FLASH_DESCRIPTOR_HEADER);
    
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
        debug(tr("Unknown region type"));
    };

    // Check region base to be in buffer
    if (regionBase * 0x1000 >= flashImage.size())
    {
        debug(tr("%1 region stored in descriptor not found").arg(regionName));
        if (twoChips) 
            debug(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            debug(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        debug(tr("Absence of %1 region assumed").arg(regionName));
        return NULL;
    }

    // Check region to be fully present in buffer
    else if (regionBase * 0x1000 + regionSize > flashImage.size())
    {
        debug(tr("%s region stored in descriptor overlaps the end of opened file").arg(regionName));
        if (twoChips) 
            debug(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            debug(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        debug(tr("Absence of %1 region assumed\n").arg(regionName));
        return NULL;
    }

    // Calculate region address
    UINT8* region = calculateAddress16((UINT8*) flashImage.constData(), regionBase);

    // Add tree item
    QByteArray body = flashImage.mid(regionBase * 0x1000, regionSize);
    index = addTreeItem(RegionItem, regionSubtype, QByteArray(), body, index);

    return region;
}   

UINT8 UEFITool::parseBios(const QByteArray & bios, const QModelIndex & parent)
{
    // Search for first volume
    INT32 prevVolumeIndex = getNextVolumeIndex(bios);
    
    // No volumes found
    if (prevVolumeIndex < 0) {
        return ERR_VOLUMES_NOT_FOUND;
    }

    // First volume is not at the beginning of bios space
    if (prevVolumeIndex > 0) {
        QByteArray padding = bios.left(prevVolumeIndex);
         addTreeItem(PaddingItem, 0, QByteArray(), padding, parent);
    }

    // Search for and parse all volumes
    INT32 volumeIndex;
    UINT32 prevVolumeSize;
    for (volumeIndex = prevVolumeIndex, prevVolumeSize = 0; 
         volumeIndex >= 0; 
         prevVolumeIndex = volumeIndex, prevVolumeSize = getVolumeSize(bios, volumeIndex), volumeIndex = getNextVolumeIndex(bios, volumeIndex + prevVolumeSize)) 
    {
        // Padding between volumes
        if (volumeIndex > prevVolumeIndex + prevVolumeSize) {
            UINT32 size = volumeIndex - prevVolumeIndex - prevVolumeSize;
            QByteArray padding = bios.mid(prevVolumeIndex + prevVolumeSize, size);
            addTreeItem(PaddingItem, 0, QByteArray(), padding, parent);
        } 
        
        // Populate volume header
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeIndex);
        
        //Check that volume is fully present in input
        if (volumeIndex + volumeHeader->FvLength > bios.size()) {
            debug(tr("Volume overlaps the end of input buffer"));
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
            debug("Incompatible revision 1 volume alignment setup");

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
            if (volumeIndex % alignment) {
                debug(tr("Unaligned revision 1 volume"));
            }
        }
        else if (volumeHeader->Revision == 2) {
            // Aquire alignment
            alignment = pow(2, (volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
            
            // Check alignment
            if (volumeIndex % alignment) {
                debug(tr("Unaligned revision 2 volume"));
            }
        }
        else
            debug(tr("Unknown volume revision (%1)").arg(volumeHeader->Revision));
        
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
            debug(tr("Unknown file system (%1)").arg(guidToQString(volumeHeader->FileSystemGuid)));
            parseCurrentVolume = false;
        }

        // Check attributes
        // Determine erase polarity
        bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;

        // Check header checksum by recalculating it
        if (!calculateChecksum16((UINT8*) volumeHeader, volumeHeader->HeaderLength)) {
            debug(tr("Volume header checksum is invalid"));
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
        QByteArray  header = bios.mid(volumeIndex, headerSize);
        QByteArray  body   = bios.mid(volumeIndex + headerSize, volumeHeader->FvLength - headerSize);
        QModelIndex index  = addTreeItem(VolumeItem, 0, header, body, parent);

        // Parse volume
        if (parseCurrentVolume) {
            UINT32 result = parseVolume(bios.mid(volumeIndex + headerSize, volumeHeader->FvLength - headerSize), headerSize, volumeHeader->Revision, erasePolarity, index);
            if (result)
                debug(tr("Volume parsing failed (%1)").arg(result));
        }
    }
    
    return ERR_SUCCESS;
}

INT32 UEFITool::getNextVolumeIndex(const QByteArray & bios, INT32 volumeIndex)
{
    if (volumeIndex < 0)
        return -1;

    INT32 nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeIndex);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET) {
        return -1;
    }

    return nextIndex - EFI_FV_SIGNATURE_OFFSET;
}

UINT32 UEFITool::getVolumeSize(const QByteArray & bios, INT32 volumeIndex)
{
    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeIndex);
    
    // Check volume signature
    if (QByteArray((const char*) &volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return 0;
    return volumeHeader->FvLength;
}

UINT8 UEFITool::parseVolume(const QByteArray & volume, UINT32 volumeBase, UINT8 revision, bool erasePolarity, const QModelIndex & parent)
{
    // Construct empty byte based on erasePolarity value
    // Native char type is used because QByteArray.count() takes it
    char empty = erasePolarity ? '\xFF' : '\x00';
    
    // Search for and parse all files
    INT32 fileIndex = 0;
    while (fileIndex >= 0) {
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) (volume.constData() + fileIndex);
        QByteArray file = volume.mid(fileIndex, uint24ToUint32(fileHeader->Size));
        QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
        
		// Check file size to at least sizeof(EFI_FFS_FILE_HEADER)
		if (file.size() < sizeof(EFI_FFS_FILE_HEADER))
		{
			debug(tr("File with invalid size"));
			return ERR_INVALID_FILE;
		}

        // We are at empty space in the end of volume
        if (header.count(empty) == header.size()) {
            QByteArray body = volume.right(volume.size() - fileIndex);
            addTreeItem(PaddingItem, 0, QByteArray(), body, parent);
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
            debug(tr("%1: stored header checksum %2 differs from calculated %3")
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
                debug(tr("%1: stored data checksum %2 differs from calculated %3")
                    .arg(guidToQString(fileHeader->Name))
                    .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0'))
                    .arg(calculated, 2, 16, QChar('0')));
            }
        }
        // Data checksum must be one of predefined values
        else {
            if (fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM &&fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2) {
                debug(tr("%1: stored data checksum %2 differs from standard value")
                    .arg(guidToQString(fileHeader->Name))
                    .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0')));
            }
        }

        // Check file alignment
        UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
        UINT32 alignment = pow(2, alignmentPower);
        if ((volumeBase + fileIndex + sizeof(EFI_FFS_FILE_HEADER)) % alignment) {
            debug(tr("%1: unaligned file").arg(guidToQString(fileHeader->Name)));
        }

        // Get file body
        QByteArray body = file.right(file.size() - sizeof(EFI_FFS_FILE_HEADER));
        // For files in Revision 1 volumes, check for file tail presence
        if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
        {
            //Check file tail;
            UINT16* tail = (UINT16*) body.right(sizeof(UINT16)).constData();
            if (!fileHeader->IntegrityCheck.TailReference == *tail)
                debug(tr("%1: file tail value %2 is not a bitwise not of %3 stored in file header")
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
            debug(tr("Unknown file type (%1)").arg(fileHeader->Type, 2, 16, QChar('0')));
        };

        // Check for empty file
        if (body.count(empty) == body.size())
        {
            // No need to parse empty files
            parseCurrentFile = false;
        }

        // Add tree item
        QModelIndex index = addTreeItem(FileItem, fileHeader->Type, header, body, parent);
        
        // Parse file
        if (parseCurrentFile) {
            if (parseAsBios) {
                UINT32 result = parseBios(body, index);
                if (result && result != ERR_VOLUMES_NOT_FOUND)
                    debug(tr("Parse file as BIOS failed (%1)").arg(result));
            }
            else {
                UINT32 result = parseFile(body, revision, erasePolarity, index);
                if (result)
                    debug(tr("Parse file as FFS failed (%1)").arg(result));
            }
        }

        // Move to next file
        fileIndex += file.size();
        fileIndex = ALIGN8(fileIndex);
        
        // Exit from loop if no files left
        if (fileIndex >= volume.size())
            fileIndex = -1;
    }
    
    return ERR_SUCCESS;
}

UINT8 UEFITool::parseFile(const QByteArray & file, UINT8 revision, bool erasePolarity, const QModelIndex & parent)
{
    // Search for and parse all sections
    INT32 sectionIndex = 0;
    while(sectionIndex >= 0)
    {
        EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) (file.constData() + sectionIndex);
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

        switch (sectionHeader->Type)
        {
        // Encapsulated sections
        case EFI_SECTION_COMPRESSION:
            compressedSectionHeader = (EFI_COMPRESSION_SECTION*) sectionHeader;
            header = file.mid(sectionIndex, sizeof(EFI_COMPRESSION_SECTION));

            // Try to decompress this section
            switch (compressedSectionHeader->CompressionType)
            {
            case EFI_NOT_COMPRESSED:
                body  = file.mid(sectionIndex + sizeof(EFI_COMPRESSION_SECTION), compressedSectionHeader->UncompressedLength);
                index = addTreeItem(SectionItem, CompressionSection, header, body, parent);
                // Parse stored file
                result = parseFile(body, revision, erasePolarity, index);
                if (result)
                    debug(tr("Stored section can not be parsed as file (%1)").arg(result));
                break;
            case EFI_STANDARD_COMPRESSION:
                //Must be Tiano for all revisions, needs checking
                body  = file.mid(sectionIndex + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);

                // Get buffer sizes
                data = (VOID*) (file.constData() + sectionIndex + sizeof(EFI_COMPRESSION_SECTION));
                dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION);
                if (TianoGetInfo(data, dataSize, &decompressedSize, &scratchSize) != ERR_SUCCESS
                    ||  decompressedSize != compressedSectionHeader->UncompressedLength)
                    debug(tr("TianoGetInfo failed"));
                else {
                    decompressed = new UINT8[decompressedSize];
                    scratch = new UINT8[scratchSize];
                    // Decompress section data
                    if (TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize) != ERR_SUCCESS)
                        debug(tr("TianoDecompress failed"));
                    else
                    {
                        body = QByteArray::fromRawData((const char*) decompressed, decompressedSize);
                        // Parse stored file
                        result = parseFile(body, revision, erasePolarity, index);
                        if (result)
                            debug(tr("Compressed section with Tiano compression can not be parsed as file (%1)").arg(result));
                    }

                    delete[] decompressed;
                    delete[] scratch;
                }
                break;
            case EFI_CUSTOMIZED_COMPRESSION:
                body  = file.mid(sectionIndex + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);

                // Get buffer sizes
                data = (VOID*) (file.constData() + sectionIndex + sizeof(EFI_COMPRESSION_SECTION));
                dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION);
                if (LzmaGetInfo(data, dataSize, &decompressedSize) != ERR_SUCCESS
                    ||  decompressedSize != compressedSectionHeader->UncompressedLength)
                {
                    // Shitty file with a section header between COMPRESSED_SECTION_HEADER and LZMA_HEADER
					data = (VOID*) (file.constData() + sectionIndex + sizeof(EFI_COMPRESSION_SECTION) + sizeof(EFI_COMMON_SECTION_HEADER));
					dataSize = uint24ToUint32(sectionHeader->Size) - sizeof(EFI_COMPRESSION_SECTION) - sizeof(EFI_COMMON_SECTION_HEADER);
					if (LzmaGetInfo(data, dataSize, &decompressedSize) != ERR_SUCCESS)
						debug(tr("LzmaGetInfo failed"));
                }   
				decompressed = new UINT8[decompressedSize];
				// Decompress section data
				if (LzmaDecompress(data, dataSize, decompressed) != ERR_SUCCESS)
					debug(tr("LzmaDecompress failed"));
				else
				{
					body = QByteArray::fromRawData((const char*) decompressed, decompressedSize);
					// Parse stored file
					result = parseFile(body, revision, erasePolarity, index);
					if (result)
						debug(tr("Compressed section with LZMA compression can not be parsed as file (%1)").arg(result));
				}

				delete[] decompressed;
                break;
            default:
                body  = file.mid(sectionIndex + sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
                index = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
                debug(tr("Compressed section with unknown compression type found (%1)").arg(compressedSectionHeader->CompressionType));
            }

            break;
        case EFI_SECTION_GUID_DEFINED:
            header = file.mid(sectionIndex, sizeof(EFI_GUID_DEFINED_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_GUID_DEFINED_SECTION), sectionSize - sizeof(EFI_GUID_DEFINED_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            // Parse section body as file
            guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*) (header.constData());
            body   = file.mid(sectionIndex + guidDefinedSectionHeader->DataOffset, sectionSize - guidDefinedSectionHeader->DataOffset); 
            result = parseFile(body, revision, erasePolarity, index);
            if (result)
                debug(tr("GUID defined section body can not be parsed as file (%1)").arg(result));
            break;
        case EFI_SECTION_DISPOSABLE:
            header = file.mid(sectionIndex, sizeof(EFI_DISPOSABLE_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_DISPOSABLE_SECTION), sectionSize - sizeof(EFI_DISPOSABLE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        // Leaf sections
        case EFI_SECTION_PE32:
            header = file.mid(sectionIndex, sizeof(EFI_PE32_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_PE32_SECTION), sectionSize - sizeof(EFI_PE32_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_PIC:
            header = file.mid(sectionIndex, sizeof(EFI_PIC_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_PIC_SECTION), sectionSize - sizeof(EFI_PIC_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_TE:
            header = file.mid(sectionIndex, sizeof(EFI_TE_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_TE_SECTION), sectionSize - sizeof(EFI_TE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_VERSION:
            header = file.mid(sectionIndex, sizeof(EFI_VERSION_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_VERSION_SECTION), sectionSize - sizeof(EFI_VERSION_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_USER_INTERFACE:
            header = file.mid(sectionIndex, sizeof(EFI_USER_INTERFACE_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_USER_INTERFACE_SECTION), sectionSize - sizeof(EFI_USER_INTERFACE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_COMPATIBILITY16:
            header = file.mid(sectionIndex, sizeof(EFI_COMPATIBILITY16_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_COMPATIBILITY16_SECTION), sectionSize - sizeof(EFI_COMPATIBILITY16_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
            header = file.mid(sectionIndex, sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION), sectionSize - sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            // Parse section body as BIOS space
            result = parseBios(body, index);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                debug(tr("Firmware volume image can not be parsed (%1)").arg(result));
            break;
        case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
            header = file.mid(sectionIndex, sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION), sectionSize - sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_RAW:
            header = file.mid(sectionIndex, sizeof(EFI_RAW_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_RAW_SECTION), sectionSize - sizeof(EFI_RAW_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            // Parse section body as BIOS space
            result = parseBios(body, index);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                debug(tr("Raw section can not be parsed as BIOS (%1)").arg(result));
            break;
            break;
        case EFI_SECTION_DXE_DEPEX:
            header = file.mid(sectionIndex, sizeof(EFI_DXE_DEPEX_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_DXE_DEPEX_SECTION), sectionSize - sizeof(EFI_DXE_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_PEI_DEPEX:
            header = file.mid(sectionIndex, sizeof(EFI_PEI_DEPEX_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_PEI_DEPEX_SECTION), sectionSize - sizeof(EFI_PEI_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        case EFI_SECTION_SMM_DEPEX:
            header = file.mid(sectionIndex, sizeof(EFI_SMM_DEPEX_SECTION));
            body   = file.mid(sectionIndex + sizeof(EFI_SMM_DEPEX_SECTION), sectionSize - sizeof(EFI_SMM_DEPEX_SECTION));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
            break;
        default:
            debug(tr("Section with unknown type (%1)").arg(sectionHeader->Type, 2, 16, QChar('0')));
            header = file.mid(sectionIndex, sizeof(EFI_COMMON_SECTION_HEADER));
            body   = file.mid(sectionIndex + sizeof(EFI_COMMON_SECTION_HEADER), sectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
            index  = addTreeItem(SectionItem, sectionHeader->Type, header, body, parent);
        }
        
        // Move to next section
        sectionIndex += uint24ToUint32(sectionHeader->Size);
        sectionIndex = ALIGN4(sectionIndex);

        // Exit from loop if no sections left
        if (sectionIndex >= file.size())
            sectionIndex = -1;
    }

    return ERR_SUCCESS;
}

