/* treemodel.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#include "treeitem.h"
#include "treemodel.h"
#include "ffs.h"
#include "descriptor.h"

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem(RootItem, 0, tr("Object"), tr("Type"), tr("Subtype"), tr("Text"));
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

bool TreeModel::hasEmptyHeader(const QModelIndex& index)
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyHeader();
}

bool TreeModel::hasEmptyBody(const QModelIndex& index)
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyBody();
}


int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::UserRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole)
        return item->data(index.column());
    else
        return item->info();

    return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}


int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

QModelIndex TreeModel::addItem(UINT8 type, UINT8 subtype, const QByteArray &header, const QByteArray &body, const QModelIndex &parent)
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
            info = tr("GUID: %1\nHeader size: %2\nFlags: %3\nImage size: %4")
                .arg(guidToQString(aptioCapsuleHeader->CapsuleHeader.CapsuleGuid))
                .arg(aptioCapsuleHeader->CapsuleHeader.Flags, 8, 16, QChar('0'))
                .arg(aptioCapsuleHeader->RomImageOffset, 4, 16, QChar('0'))
                .arg(aptioCapsuleHeader->CapsuleHeader.CapsuleImageSize - aptioCapsuleHeader->RomImageOffset, 8, 16, QChar('0'));
            //!TODO: more info about Aptio capsule
            break;
        case UefiCapsule:
            name = tr("UEFI capsule");
            capsuleHeader = (EFI_CAPSULE_HEADER*) header.constData();
            info = tr("GUID: %1\nHeader size: %2\nFlags: %3\nImage size: %4")
                .arg(guidToQString(capsuleHeader->CapsuleGuid))
                .arg(capsuleHeader->Flags, 8, 16, QChar('0'))
                .arg(capsuleHeader->HeaderSize, 8, 16, QChar('0'))
                .arg(capsuleHeader->CapsuleImageSize, 8, 16, QChar('0'));
            break;
        default:
            name = tr("Unknown capsule");
            info = tr("GUID: %1\n").arg(guidToQString(*(EFI_GUID*)header.constData()));
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
        info = tr("Size: %1").arg(body.size(), 8, 16, QChar('0'));
        //!TODO: more info about GbE and ME regions
        switch (subtype)
        {
        case GbeRegion:
            name = tr("GbE region");
            break;
        case MeRegion:
            name = tr("ME region");
            break;
        case BiosRegion:
            name = tr("BIOS region");
            break;
        case PdrRegion:
            name = tr("PDR region");
            break;
        default:
            name = tr("Unknown region");
            break;
        }
        break;
    case PaddingItem:
        name = tr("Padding");
        info = tr("Size: %1").arg(body.size(), 8, 16, QChar('0'));
        break;
    case VolumeItem:
        typeName = tr("Volume");
        // Parse volume header to determine its revision and file system
        volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) header.constData();
        name = guidToQString(volumeHeader->FileSystemGuid);
        subtypeName = tr("Revision %1").arg(volumeHeader->Revision);
        info = tr("Size: %1\nSignature: %2\nAttributes: %3\nHeader size: %4")
            .arg(volumeHeader->FvLength, 8, 16, QChar('0'))
            .arg(volumeHeader->Signature, 8, 16, QChar('0'))
            .arg(volumeHeader->Attributes, 8, 16, QChar('0'))
            .arg(volumeHeader->HeaderLength, 4, 16, QChar('0'));
        break;
    case FileItem:
        typeName = tr("File");
        // Parse file header to determine its GUID and type
        fileHeader = (EFI_FFS_FILE_HEADER*) header.constData();
        name = guidToQString(fileHeader->Name);
        subtypeName = fileTypeToQString(subtype);
        info = tr("Type: %1\nAttributes: %2\nSize: %3\nState: %4")
            .arg(fileHeader->Type, 2, 16, QChar('0'))
            .arg(fileHeader->Attributes, 2, 16, QChar('0'))
            .arg(uint24ToUint32(fileHeader->Size), 6, 16, QChar('0'))
            .arg(fileHeader->State, 2, 16, QChar('0'));
        break;
    case SectionItem:
        typeName = tr("Section");
        name = sectionTypeToQString(subtype) + tr(" section");
        info = tr("Size: %1").arg(body.size(), 8, 16, QChar('0'));
        //!TODO: add more specific info for all section types with uncommon headers
        // Set name of file
        if (subtype == UserInterfaceSection)
        {
            QString text = QString::fromUtf16((const ushort*)body.constData());
            setItemText(text, findParentOfType(FileItem, parent));
        }
        break;
    default:
        name = tr("Unknown"); 
        break;
    }

    emit layoutAboutToBeChanged();
    TreeItem *item = new TreeItem(type, subtype, name, typeName, subtypeName, "", info, header, body, parentItem);
    parentItem->appendChild(item);
    emit layoutChanged();
    return createIndex(parentItem->childCount() - 1, parentColumn, item);
}

bool TreeModel::setItemName(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;
    
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setName(data);
    emit dataChanged(index, index);
    return true;
}

bool TreeModel::setItemText(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->setText(data);
    emit dataChanged(index, index);
    return true;
}

bool TreeModel::removeItem(const QModelIndex &index)
{
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    item->parent()->removeChild(item);
    delete item;
    return true;
}

QModelIndex TreeModel::findParentOfType(UINT8 type, const QModelIndex& index)
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

QByteArray TreeModel::header(const QModelIndex& index)
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->header();
}

QByteArray TreeModel::body(const QModelIndex& index)
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->body();
}