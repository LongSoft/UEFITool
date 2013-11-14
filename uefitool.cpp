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
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(extract()));
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
    // Clear UI components
    ui->debugEdit->clear();
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
    ffsEngine = new FfsEngine(this);
    ui->structureTreeView->setModel(ffsEngine->model());
    
    // Connect
    connect(ui->structureTreeView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    connect(ui->structureTreeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(resizeTreeViewColums(void)));
    connect(ui->structureTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(populateUi(const QModelIndex &)));

    resizeTreeViewColums();
}

void UEFITool::populateUi(const QModelIndex &current)
{
    currentIndex = current;
    ui->infoEdit->setPlainText(current.data(Qt::UserRole).toString());
    ui->actionExtract->setDisabled(ffsEngine->hasEmptyBody(current) && ffsEngine->hasEmptyHeader(current));
    ui->actionExtractBody->setDisabled(ffsEngine->hasEmptyHeader(current));
    ui->actionExtractUncompressed->setEnabled(ffsEngine->isCompressedFile(current));
    ui->actionRemove->setEnabled(ffsEngine->isOfType(TreeItem::Volume, current) 
        || ffsEngine->isOfType(TreeItem::File, current) 
        || ffsEngine->isOfType(TreeItem::Section, current));
    ui->actionInsertInto->setEnabled(ffsEngine->isOfType(TreeItem::Volume, current) 
        || ffsEngine->isOfType(TreeItem::File, current)
        || ffsEngine->isOfType(TreeItem::Section, current));
    ui->actionInsertBefore->setEnabled(ffsEngine->isOfType(TreeItem::File, current) 
        || ffsEngine->isOfType(TreeItem::Section, current));
    ui->actionInsertAfter->setEnabled(ffsEngine->isOfType(TreeItem::File, current)
        || ffsEngine->isOfType(TreeItem::Section, current));
    //ui->actionReplace->setEnabled(ffsEngine->isOfType(TreeItem::File, current));
}

void UEFITool::remove()
{
    UINT8 result = ffsEngine->remove(currentIndex);
    if (result) {
        ui->debugEdit->setPlainText(ffsEngine->message());
    }
    else
        ui->actionSaveImageFile->setEnabled(true);
    
    resizeTreeViewColums();
}

void UEFITool::insert(const UINT8 mode)
{
    QString path;
    TreeItem* item = static_cast<TreeItem*>(currentIndex.internalPointer());
    
    UINT8 type;
	UINT8 objectType;
    if (mode == INSERT_MODE_BEFORE || mode == INSERT_MODE_BEFORE)
        type = item->parent()->type();
    else
        type = item->type();

    switch (type) {
    case TreeItem::Volume:
        path = QFileDialog::getOpenFileName(this, tr("Select FFS file to insert"),".","FFS file (*.ffs *.bin);;All files (*.*)");
		objectType = TreeItem::File;
    break;
    case TreeItem::File:
    case TreeItem::Section:
        path = QFileDialog::getOpenFileName(this, tr("Select section file to insert"),".","Section file (*.sec *.bin);;All files (*.*)");
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

    UINT8 result = ffsEngine->insert(currentIndex, buffer, objectType, mode);
    if (result)
        ui->statusBar->showMessage(tr("File can't be inserted (%1)").arg(result));
    else
        ui->actionSaveImageFile->setEnabled(true);
    
    resizeTreeViewColums();
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
    if (result)
    {
        ui->statusBar->showMessage(tr("Reconstruction failed (%1)").arg(result));
        ui->debugEdit->setPlainText(ffsEngine->message());
        return;
    }

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();
    ui->statusBar->showMessage(tr("Reconstructed image written"));
    ui->debugEdit->setPlainText(ffsEngine->message());
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
    if (result)
        ui->statusBar->showMessage(tr("Opened file can't be parsed (%1)").arg(result));
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
    
    ui->debugEdit->appendPlainText(ffsEngine->message());
    
    resizeTreeViewColums();
}

void UEFITool::extract()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected item to binary file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for rewriting"));
        return;
    }

    outputFile.resize(0);
    outputFile.write(ffsEngine->header(currentIndex) + ffsEngine->body(currentIndex));
    outputFile.close();
}

void UEFITool::extractBody()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected item without header to file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for rewriting"));
        return;
    }

    outputFile.resize(0);
    outputFile.write(ffsEngine->body(currentIndex));
    outputFile.close();
}

void UEFITool::extractUncompressed()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected FFS file as uncompressed to file"),".","FFS files (*.ffs);;All files (*.*)");
    
    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for rewriting"));
        return;
    }

    outputFile.resize(0);
    outputFile.write(ffsEngine->decompressFile(currentIndex));
    outputFile.close();
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

