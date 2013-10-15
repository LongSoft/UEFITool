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
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(saveAll()));
    connect(ui->actionExtractBody, SIGNAL(triggered()), this, SLOT(saveBody()));
    connect(ui->actionExtractUncompressed, SIGNAL(triggered()), this, SLOT(saveUncompressedFile()));
    // Enable Drag-and-Drop actions
    this->setAcceptDrops(true);

    // Initialise non-persistent data
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
    ui->actionDelete->setDisabled(true);
    ui->actionReplaceWithPadding->setDisabled(true);
    ui->actionInsertBefore->setDisabled(true);
    ui->actionInsertAfter->setDisabled(true);

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

void UEFITool::populateUi(const QModelIndex &current/*, const QModelIndex &previous*/)
{
    //!TODO: make widget
    currentIndex = current;
    ui->infoEdit->setPlainText(current.data(Qt::UserRole).toString());
    ui->actionExtract->setDisabled(ffsEngine->hasEmptyBody(current) && ffsEngine->hasEmptyHeader(current));
    ui->actionExtractBody->setDisabled(ffsEngine->hasEmptyHeader(current));
    ui->actionExtractUncompressed->setEnabled(ffsEngine->isCompressedFile(current));
}

void UEFITool::resizeTreeViewColums(/*const QModelIndex &index*/)
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
    UINT8 result = ffsEngine->parseInputFile(buffer);
    if (result)
        ui->statusBar->showMessage(tr("Opened file can't be parsed as UEFI image (%1)").arg(result));
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
    
    ui->debugEdit->appendPlainText(ffsEngine->message());
    resizeTreeViewColums();
}

void UEFITool::saveAll()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected item to binary file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }

    outputFile.write(ffsEngine->header(currentIndex) + ffsEngine->body(currentIndex));
    outputFile.close();
}

void UEFITool::saveBody()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected item without header to binary file"),".","Binary files (*.bin);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }

    outputFile.write(ffsEngine->body(currentIndex));
    outputFile.close();
}

void UEFITool::saveUncompressedFile()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save selected FFS file as uncompressed to binary file"),".","FFS files (*.ffs);;All files (*.*)");
    
    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly))
    {
        ui->statusBar->showMessage(tr("Can't open file for writing. Check file permissions."));
        return;
    }
    
    outputFile.write(ffsEngine->uncompressFile(currentIndex));
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

