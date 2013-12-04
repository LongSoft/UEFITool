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
    // Create UI
    ui->setupUi(this);
    searchDialog = new SearchDialog(this);
	ffsEngine = NULL;
	
    // Connect signals to slots
    connect(ui->actionOpenImageFile, SIGNAL(triggered()), this, SLOT(openImageFile()));
	connect(ui->actionSaveImageFile, SIGNAL(triggered()), this, SLOT(saveImageFile()));
	connect(ui->actionSearch, SIGNAL(triggered()), this, SLOT(search()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(extractAsIs()));
    connect(ui->actionExtractBody, SIGNAL(triggered()), this, SLOT(extractBody()));
    connect(ui->actionExtractUncompressed, SIGNAL(triggered()), this, SLOT(extractUncompressed()));
    connect(ui->actionInsertInto, SIGNAL(triggered()), this, SLOT(insertInto()));
    connect(ui->actionInsertBefore, SIGNAL(triggered()), this, SLOT(insertBefore()));
    connect(ui->actionInsertAfter, SIGNAL(triggered()), this, SLOT(insertAfter()));
    connect(ui->actionReplace, SIGNAL(triggered()), this, SLOT(replace()));
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(remove()));
    connect(ui->actionRebuild, SIGNAL(triggered()), this, SLOT(rebuild()));
    connect(ui->actionChangeToNone, SIGNAL(triggered()), this, SLOT(changeToNone()));
    connect(ui->actionChangeToEfi11, SIGNAL(triggered()), this, SLOT(changeToEfi11()));
    connect(ui->actionChangeToTiano, SIGNAL(triggered()), this, SLOT(changeToTiano()));
    connect(ui->actionChangeToLzma, SIGNAL(triggered()), this, SLOT(changeToLzma()));
    connect(ui->actionMessagesClear, SIGNAL(triggered()), this, SLOT(clearMessages()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), this, SLOT(aboutQt()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(exit()));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(writeSettings()));

    // Enable Drag-and-Drop actions
    this->setAcceptDrops(true);

    // Initialize non-persistent data
    init();
	
	// Read stored settings
	readSettings();
}

UEFITool::~UEFITool()
{
    delete ui;
    delete ffsEngine;
	delete searchDialog;
}

void UEFITool::init()
{
	// Clear components
    ui->messageListWidget->clear();
    ui->infoEdit->clear();
    
    // Disable menus
    ui->menuCapsuleActions->setDisabled(true);
    ui->menuImageActions->setDisabled(true);
    ui->menuRegionActions->setDisabled(true);
    ui->menuPaddingActions->setDisabled(true);
    ui->menuVolumeActions->setDisabled(true);
    ui->menuFileActions->setDisabled(true);
    ui->menuSectionActions->setDisabled(true);

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
}

void UEFITool::populateUi(const QModelIndex &current)
{
    if (!current.isValid())
        return;

    TreeItem* item = static_cast<TreeItem*>(current.internalPointer());
    UINT8 type = item->type();
    UINT8 subtype = item->subtype();
    UINT8 algorithm = item->compression();
    
    // Set info text
    ui->infoEdit->setPlainText(item->info());

    // Enable menus
    ui->menuCapsuleActions->setEnabled(type == TreeItem::Capsule);
    ui->menuImageActions->setEnabled(type == TreeItem::Image);
    ui->menuRegionActions->setEnabled(type == TreeItem::Region);
    ui->menuPaddingActions->setEnabled(type == TreeItem::Padding);
    ui->menuVolumeActions->setEnabled(type == TreeItem::Volume);
    ui->menuFileActions->setEnabled(type == TreeItem::File);
    ui->menuSectionActions->setEnabled(type == TreeItem::Section);
    
    // Enable actions
    ui->actionExtract->setDisabled(item->hasEmptyHeader() && item->hasEmptyBody() && item->hasEmptyTail());
    ui->actionRebuild->setDisabled(item->hasEmptyHeader() && item->hasEmptyBody() && item->hasEmptyTail());
    ui->actionExtractBody->setDisabled(item->hasEmptyHeader());
    ui->actionRemove->setEnabled(type == TreeItem::Volume || type == TreeItem::File || type == TreeItem::Section);
    ui->actionInsertInto->setEnabled(type == TreeItem::Volume || (type == TreeItem::File && subtype != EFI_FV_FILETYPE_RAW && subtype != EFI_FV_FILETYPE_PAD)
        || (type == TreeItem::Section && (subtype == EFI_SECTION_COMPRESSION || subtype == EFI_SECTION_GUID_DEFINED || subtype == EFI_SECTION_DISPOSABLE)));
    ui->actionInsertBefore->setEnabled(type == TreeItem::File || type == TreeItem::Section);
    ui->actionInsertAfter->setEnabled(type == TreeItem::File || type == TreeItem::Section); 
    ui->menuChangeCompressionTo->setEnabled(type == TreeItem::Section && subtype == EFI_SECTION_COMPRESSION &&
        (algorithm == COMPRESSION_ALGORITHM_NONE || COMPRESSION_ALGORITHM_EFI11 || algorithm == COMPRESSION_ALGORITHM_TIANO || algorithm == COMPRESSION_ALGORITHM_LZMA));
}

void UEFITool::search()
{
	if (searchDialog->exec() != QDialog::Accepted)
		return;

	int index = searchDialog->ui->dataTypeComboBox->currentIndex();
	if (index == 0) { // Hex pattern
		QByteArray pattern = QByteArray::fromHex(searchDialog->ui->searchEdit->text().toAscii());
		if (pattern.isEmpty())
			return;
		ffsEngine->findHexPattern(pattern, searchDialog->ui->dataOnlyRadioButton->isChecked());
		showMessages();
	}
	else if (index == 1) { // Text string
		QString pattern = searchDialog->ui->searchEdit->text();
		if (pattern.isEmpty())
			return;
		ffsEngine->findTextPattern(pattern, searchDialog->ui->unicodeCheckBox->isChecked(), 
			(Qt::CaseSensitivity) searchDialog->ui->caseSensitiveCheckBox->isChecked());
		showMessages();
	}
}

void UEFITool::rebuild()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    UINT8 result = ffsEngine->rebuild(index);
    
    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
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
        path = QFileDialog::getOpenFileName(this, tr("Select FFS file to insert"),".","FFS files (*.ffs *.bin);;All files (*.*)");
        objectType = TreeItem::File;
    break;
    case TreeItem::File:
    case TreeItem::Section:
        path = QFileDialog::getOpenFileName(this, tr("Select section file to insert"),".","Section files (*.sct *.bin);;All files (*.*)");
        objectType = TreeItem::Section;
    break;
    default:
        return;
    }

    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists()) {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly)) {
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

void UEFITool::changeToEfi11()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    UINT8 result = ffsEngine->changeCompression(index, COMPRESSION_ALGORITHM_EFI11);
    
    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::changeToTiano()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    UINT8 result = ffsEngine->changeCompression(index, COMPRESSION_ALGORITHM_TIANO);
    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::changeToLzma()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    UINT8 result = ffsEngine->changeCompression(index, COMPRESSION_ALGORITHM_LZMA);
    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::changeToNone()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    UINT8 result = ffsEngine->changeCompression(index, COMPRESSION_ALGORITHM_NONE);
    if (result == ERR_SUCCESS)
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::about()
{
    QMessageBox::about(this, tr("About UEFITool"), tr(
        "Copyright (c) 2013, Nikolaj Schlej aka <b>CodeRush</b>.<br><br>"
        "The program is dedicated to <b>RevoGirl</b>. Rest in peace, young genius.<br><br>"
        "The program and the accompanying materials are licensed and made available under the terms and conditions of the BSD License.<br>"
        "The full text of the license may be found at <a href=http://opensource.org/licenses/bsd-license.php>OpenSource.org</a>.<br><br>"
        "<b>THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS, "
        "WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, "
        "EITHER EXPRESS OR IMPLIED.</b>"));
}

void UEFITool::aboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void UEFITool::exit()
{
	QCoreApplication::exit(0);
}

void UEFITool::saveImageFile()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save BIOS image file"),".","BIOS image files (*.rom *.bin *.cap *.fd *.wph *.efi);;All files (*.*)");

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly)) {
        ui->statusBar->showMessage(tr("Can't open file for writing"));
        return;
    }    
    
    QByteArray reconstructed;
    UINT8 result = ffsEngine->reconstructImage(reconstructed);
    showMessages();
    if (result) {
        ui->statusBar->showMessage(tr("Reconstruction failed (%1)").arg(result));
        return;
    }

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();
    ui->statusBar->showMessage(tr("Reconstructed image written"));
}

void UEFITool::resizeTreeViewColumns()
{
    int count = ffsEngine->model()->columnCount();
    for(int i = 0; i < count; i++)
        ui->structureTreeView->resizeColumnToContents(i);
}

void UEFITool::openImageFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file"),".","BIOS image files (*.rom *.bin *.cap *.bio *.fd *.wph *.efi);;All files (*.*)");
    openImageFile(path);
}

void UEFITool::openImageFile(QString path)
{
    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists()) {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly)) {
        ui->statusBar->showMessage(tr("Can't open file for reading"));
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    init();
    UINT8 result = ffsEngine->parseInputFile(buffer);
    showMessages();
    if (result)
        ui->statusBar->showMessage(tr("Opened file can't be parsed (%1)").arg(result));
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));

	// Enable search
	ui->actionSearch->setEnabled(true);
}

void UEFITool::clearMessages()
{
	ffsEngine->clearMessages();
	messageItems.clear();
	ui->messageListWidget->clear();
}

void UEFITool::extract(const UINT8 mode)
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    UINT8 type = item->type();

    QString path;
    if(mode == EXTRACT_MODE_AS_IS) {
        switch (type) {
        case TreeItem::Capsule:
            path = QFileDialog::getSaveFileName(this, tr("Save capsule to binary file"),".","Capsule files (*.cap *.bin);;All files (*.*)");
            break;
        case TreeItem::Image:
            path = QFileDialog::getSaveFileName(this, tr("Save image to binary file"),".","Image files (*.rom *.bin);;All files (*.*)");
            break;
        case TreeItem::Region:
            path = QFileDialog::getSaveFileName(this, tr("Save region to binary file"),".","Region files (*.rgn *.bin);;All files (*.*)");
            break;
        case TreeItem::Padding:
            path = QFileDialog::getSaveFileName(this, tr("Save padding to binary file"),".","Padding files (*.pad *.bin);;All files (*.*)");
            break;
        case TreeItem::Volume:
            path = QFileDialog::getSaveFileName(this, tr("Save volume to binary file"),".","Volume files (*.vol *.bin);;All files (*.*)");
            break;
        case TreeItem::File:
            path = QFileDialog::getSaveFileName(this, tr("Save FFS file to binary file"),".","FFS files (*.ffs *.bin);;All files (*.*)");
            break;
        case TreeItem::Section:
            path = QFileDialog::getSaveFileName(this, tr("Save section file to binary file"),".","Section files (*.sct *.bin);;All files (*.*)");
        break;
        default:
            return;
        }
    }
    else
        path = QFileDialog::getSaveFileName(this, tr("Save object to binary file"),".","Binary files (*.bin);;All files (*.*)");
    
    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly)) {
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

void UEFITool::showMessages()
{
    ui->messageListWidget->clear();
    if (!ffsEngine)
        return;

    messageItems = ffsEngine->messages();
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

void UEFITool::contextMenuEvent(QContextMenuEvent* event)
{
    if (ui->messageListWidget->underMouse()) {
		ui->menuMessages->exec(event->globalPos());
		return;
	}
	
	if(!ui->structureTreeView->underMouse())
        return;
    
	QPoint pt = event->pos();
    QModelIndex index = ui->structureTreeView->indexAt(ui->structureTreeView->viewport()->mapFrom(this, pt));
    if(!index.isValid())
        return;

    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    switch(item->type())
    {
    case TreeItem::Capsule:
        ui->menuCapsuleActions->exec(event->globalPos());
        break;
    case TreeItem::Image:
        ui->menuImageActions->exec(event->globalPos());
        break;
    case TreeItem::Region:
        ui->menuRegionActions->exec(event->globalPos());
        break;
    case TreeItem::Padding:
        ui->menuPaddingActions->exec(event->globalPos());
        break;
    case TreeItem::Volume:
        ui->menuVolumeActions->exec(event->globalPos());
        break;
    case TreeItem::File:
        ui->menuFileActions->exec(event->globalPos());
        break;
    case TreeItem::Section:
        ui->menuSectionActions->exec(event->globalPos());
        break;
    }
}

void UEFITool::readSettings()
{
	QSettings settings("UEFITool.ini", QSettings::IniFormat, this);
	resize(settings.value("mainWindow/size", QSize(800, 600)).toSize());
    move(settings.value("mainWindow/position", QPoint(0, 0)).toPoint());
	QList<int> horList, vertList;
	horList.append(settings.value("mainWindow/treeWidth", 600).toInt());
	horList.append(settings.value("mainWindow/infoWidth", 180).toInt());
	vertList.append(settings.value("mainWindow/treeHeight", 400).toInt());
	vertList.append(settings.value("mainWindow/messageHeight", 180).toInt());
	ui->infoSplitter->setSizes(horList);
	ui->messagesSplitter->setSizes(vertList);
	resizeTreeViewColumns();
	ui->structureTreeView->setColumnWidth(0, settings.value("tree/columnWidth0", ui->structureTreeView->columnWidth(0)).toInt());
	ui->structureTreeView->setColumnWidth(1, settings.value("tree/columnWidth1", ui->structureTreeView->columnWidth(1)).toInt());
	ui->structureTreeView->setColumnWidth(2, settings.value("tree/columnWidth2", ui->structureTreeView->columnWidth(2)).toInt());
	ui->structureTreeView->setColumnWidth(3, settings.value("tree/columnWidth3", ui->structureTreeView->columnWidth(3)).toInt());
	//ui->structureTreeView->setColumnWidth(4, settings.value("tree/columnWidth4", 10).toInt());
}

void UEFITool::writeSettings()
{
	QSettings settings("UEFITool.ini", QSettings::IniFormat, this);
	settings.setValue("mainWindow/size", size());
	settings.setValue("mainWindow/position", pos());
	settings.setValue("mainWindow/treeWidth", ui->structureGroupBox->width());
	settings.setValue("mainWindow/infoWidth", ui->infoGroupBox->width());
	settings.setValue("mainWindow/treeHeight", ui->structureGroupBox->height());
	settings.setValue("mainWindow/messageHeight", ui->messageGroupBox->height());
	settings.setValue("tree/columnWidth0", ui->structureTreeView->columnWidth(0));
	settings.setValue("tree/columnWidth1", ui->structureTreeView->columnWidth(1));
	settings.setValue("tree/columnWidth2", ui->structureTreeView->columnWidth(2));
	settings.setValue("tree/columnWidth3", ui->structureTreeView->columnWidth(3));
	//settings.setValue("tree/columnWidth4", ui->structureTreeView->columnWidth(4));
}