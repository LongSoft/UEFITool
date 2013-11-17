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
#include "ui_uefitool.h"

UEFITool::UEFITool(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UEFITool)
{
    ui->setupUi(this);
    ffsEngine = NULL;

    //Connect
    connect(ui->actionOpenImageFile, SIGNAL(triggered()), this, SLOT(openImageFile()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(extractAsIs()));
    connect(ui->actionExtractBody, SIGNAL(triggered()), this, SLOT(extractBody()));
    connect(ui->actionExtractUncompressed, SIGNAL(triggered()), this, SLOT(extractUncompressed()));
    connect(ui->actionInsertInto, SIGNAL(triggered()), this, SLOT(insertInto()));
    connect(ui->actionInsertBefore, SIGNAL(triggered()), this, SLOT(insertBefore()));
    connect(ui->actionInsertAfter, SIGNAL(triggered()), this, SLOT(insertAfter()));
    connect(ui->actionReplace, SIGNAL(triggered()), this, SLOT(replace()));
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(remove()));
    connect(ui->actionSaveImageFile, SIGNAL(triggered()), this, SLOT(saveImageFile()));
    
    // Enable Drag-and-Drop actions
    this->setAcceptDrops(true);

    // Initialize non-persistent data
    init();
}

UEFITool::~UEFITool()
{
    delete ui;
    delete ffsEngine;
}

void UEFITool::init()
{
    // Clear components
    ui->messageListWidget->clear();
    ui->infoEdit->clear();
    
    // Disable all actions except openImageFile
    ui->actionExtract->setDisabled(true);
    ui->actionExtractBody->setDisabled(true);
    ui->actionExtractUncompressed->setDisabled(true);
    ui->actionReplace->setDisabled(true);
    ui->actionRemove->setDisabled(true);
    ui->actionInsertInto->setDisabled(true);
    ui->actionInsertBefore->setDisabled(true);
    ui->actionInsertAfter->setDisabled(true);
    ui->actionSaveImageFile->setDisabled(true);

    // Make new ffsEngine
	if (ffsEngine)
		delete ffsEngine;
    ffsEngine = new FfsEngine(this);
    ui->structureTreeView->setModel(ffsEngine->model());
    
    // Connect
    connect(ui->structureTreeView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    connect(ui->structureTreeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    connect(ui->structureTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(populateUi(const QModelIndex &)));
    connect(ui->messageListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scrollTreeView(QListWidgetItem*)));

    resizeTreeViewColums();
}

void UEFITool::populateUi(const QModelIndex &current)
{
    if (!current.isValid())
        return;

    TreeItem* item = static_cast<TreeItem*>(current.internalPointer());
    UINT8 type = item->type();
    UINT8 subtype = item->subtype();

    ui->infoEdit->setPlainText(item->info());
    ui->actionExtract->setDisabled(item->hasEmptyHeader() && item->hasEmptyBody() && item->hasEmptyTail());
    ui->actionExtractBody->setDisabled(item->hasEmptyHeader());
    //ui->actionExtractUncompressed->setEnabled(ffsEngine->isCompressedFile(current));
    ui->actionRemove->setEnabled(type == TreeItem::Volume || type == TreeItem::File || type == TreeItem::Section);
    ui->actionInsertInto->setEnabled(type == TreeItem::Volume || type == TreeItem::File 
        || (type == TreeItem::Section && (subtype == EFI_SECTION_COMPRESSION || subtype == EFI_SECTION_GUID_DEFINED || subtype == EFI_SECTION_DISPOSABLE)));
    ui->actionInsertBefore->setEnabled(type == TreeItem::File || type == TreeItem::Section);
    ui->actionInsertAfter->setEnabled(type == TreeItem::File || type == TreeItem::Section); 
    //ui->actionReplace->setEnabled(ffsEngine->isOfType(TreeItem::File, current));
}

void UEFITool::remove()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    UINT8 result = ffsEngine->remove(index);

    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::insert(const UINT8 mode)
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    UINT8 type;
	UINT8 objectType;

    if (mode == INSERT_MODE_BEFORE || mode == INSERT_MODE_AFTER)
        type = item->parent()->type();
    else
        type = item->type();

    QString path;
    switch (type) {
    case TreeItem::Volume:
        path = QFileDialog::getOpenFileName(this, tr("Select FFS file to insert"),".","FFS file (*.ffs *.bin);;All files (*.*)");
		objectType = TreeItem::File;
    break;
    case TreeItem::File:
    case TreeItem::Section:
        path = QFileDialog::getOpenFileName(this, tr("Select section file to insert"),".","Section file (*.sct *.bin);;All files (*.*)");
		objectType = TreeItem::Section;
    break;
    default:
        return;
    }

    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists())
    {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for reading"));
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->insert(index, buffer, objectType, mode);
    if (result)
        ui->statusBar->showMessage(tr("File can't be inserted (%1)").arg(result));
    else
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::insertInto()
{
    insert(INSERT_MODE_PREPEND);
}

void UEFITool::insertBefore()
{
    insert(INSERT_MODE_BEFORE);
}

void UEFITool::insertAfter()
{
    insert(INSERT_MODE_AFTER);
}

void UEFITool::replace()
{
    
}

void UEFITool::saveImageFile()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save BIOS image file"),".","BIOS image file (*.rom *.bin *.cap *.fd *.wph *.efi);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing"));
        return;
    }    
    
    QByteArray reconstructed;
    UINT8 result = ffsEngine->reconstructImage(reconstructed);
	showMessage();
    if (result)
    {
        ui->statusBar->showMessage(tr("Reconstruction failed (%1)").arg(result));

        return;
    }

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();
    ui->statusBar->showMessage(tr("Reconstructed image written"));
}

void UEFITool::resizeTreeViewColums()
{
    int count = ffsEngine->model()->columnCount();
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
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for reading"));
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    init();
    UINT8 result = ffsEngine->parseInputFile(buffer);
    showMessage();
	if (result)
        ui->statusBar->showMessage(tr("Opened file can't be parsed (%1)").arg(result));
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));

    resizeTreeViewColums();
}

void UEFITool::extract(const UINT8 mode)
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    UINT8 type = item->type();

    QString path;
    switch (type) {
    case TreeItem::Capsule:
        path = QFileDialog::getSaveFileName(this, tr("Save capsule to binary file"),".","Capsule file (*.cap *.bin);;All files (*.*)");
        break;
    case TreeItem::Image:
        path = QFileDialog::getSaveFileName(this, tr("Save image to binary file"),".","Image file (*.rom *.bin);;All files (*.*)");
        break;
    case TreeItem::Region:
        path = QFileDialog::getSaveFileName(this, tr("Save region to binary file"),".","Region file (*.rgn *.bin);;All files (*.*)");
        break;
    case TreeItem::Padding:
        path = QFileDialog::getSaveFileName(this, tr("Save padding to binary file"),".","Padding file (*.pad *.bin);;All files (*.*)");
        break;
    case TreeItem::Volume:
        path = QFileDialog::getSaveFileName(this, tr("Save volume to binary file"),".","Volume file (*.vol *.bin);;All files (*.*)");
        break;
    case TreeItem::File:
        path = QFileDialog::getSaveFileName(this, tr("Save FFS file to binary file"),".","FFS file (*.ffs *.bin);;All files (*.*)");
        break;
    case TreeItem::Section:
        path = QFileDialog::getSaveFileName(this, tr("Select section file to insert"),".","Section file (*.sct *.bin);;All files (*.*)");
    break;
    default:
        return;
    }
    
    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for rewriting"));
        return;
    }

    QByteArray extracted;
    UINT8 result = ffsEngine->extract(index, extracted, mode);
    if (result) 
        ui->statusBar->showMessage(tr("File can't be extracted (%1)").arg(result));
    else {
        outputFile.resize(0);
        outputFile.write(extracted);
        outputFile.close();
    }
}

void UEFITool::extractAsIs()
{
    extract(EXTRACT_MODE_AS_IS);
}

void UEFITool::extractBody()
{
    extract(EXTRACT_MODE_BODY_ONLY);
}

void UEFITool::extractUncompressed()
{
    extract(EXTRACT_MODE_UNCOMPRESSED);
}

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

void UEFITool::showMessage()
{
    ui->messageListWidget->clear();
	if (!ffsEngine)
		return;

	messageItems = ffsEngine->message();
    for (int i = 0; i < messageItems.count(); i++) {
        ui->messageListWidget->addItem(new MessageListItem(messageItems.at(i)));
    }
}

void UEFITool::scrollTreeView(QListWidgetItem* item)
{
    MessageListItem* messageItem = (MessageListItem*) item;
    QModelIndex index = messageItem->index();
    if (index.isValid()) {
        ui->structureTreeView->scrollTo(index);
        ui->structureTreeView->selectionModel()->clearSelection();
        ui->structureTreeView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
}