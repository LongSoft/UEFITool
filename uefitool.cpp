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
    connect(ui->actionInsertInto, SIGNAL(triggered()), this, SLOT(insertInto()));
    connect(ui->actionInsertBefore, SIGNAL(triggered()), this, SLOT(insertBefore()));
    connect(ui->actionInsertAfter, SIGNAL(triggered()), this, SLOT(insertAfter()));
    connect(ui->actionReplace, SIGNAL(triggered()), this, SLOT(replaceAsIs()));
	connect(ui->actionReplaceBody, SIGNAL(triggered()), this, SLOT(replaceBody()));
    connect(ui->actionRemove, SIGNAL(triggered()), this, SLOT(remove()));
    connect(ui->actionRebuild, SIGNAL(triggered()), this, SLOT(rebuild()));
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
    ui->actionInsertInto->setEnabled(type == TreeItem::Volume || (type == TreeItem::File && subtype != EFI_FV_FILETYPE_ALL && subtype != EFI_FV_FILETYPE_RAW && subtype != EFI_FV_FILETYPE_PAD));
    ui->actionInsertBefore->setEnabled(type == TreeItem::File || type == TreeItem::Section);
    ui->actionInsertAfter->setEnabled(type == TreeItem::File || type == TreeItem::Section);
	ui->actionReplace->setEnabled(type == TreeItem::File || type == TreeItem::Section);
	ui->actionReplaceBody->setEnabled(type == TreeItem::File || type == TreeItem::Section);
}

void UEFITool::search()
{
	if (searchDialog->exec() != QDialog::Accepted)
		return;

	int index = searchDialog->ui->dataTypeComboBox->currentIndex();
	if (index == 0) { // Hex pattern
        QByteArray pattern = QByteArray::fromHex(searchDialog->ui->searchEdit->text().toLatin1());
		if (pattern.isEmpty())
			return;
		UINT8 mode;
		if (searchDialog->ui->headerOnlyRadioButton->isChecked())
			mode = SEARCH_MODE_HEADER;
		else if (searchDialog->ui->bodyOnlyRadioButton->isChecked())
			mode = SEARCH_MODE_BODY;
		else
			mode = SEARCH_MODE_ALL;
		ffsEngine->findHexPattern(pattern, mode);
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

    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
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

    UINT8 result = ffsEngine->insert(index, buffer, mode);
    if (result)
        ui->statusBar->showMessage(tr("File can't be inserted (%1)").arg(result));
    else
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::insertInto()
{
    insert(CREATE_MODE_PREPEND);
}

void UEFITool::insertBefore()
{
    insert(CREATE_MODE_BEFORE);
}

void UEFITool::insertAfter()
{
    insert(CREATE_MODE_AFTER);
}

void UEFITool::replaceAsIs()
{
    replace(REPLACE_MODE_AS_IS);
}

void UEFITool::replaceBody()
{
    replace(REPLACE_MODE_BODY);
}

void UEFITool::replace(const UINT8 mode)
{
	QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	QString path;
	if (item->type() == TreeItem::File) {
		if (mode == REPLACE_MODE_AS_IS) {
			path = QFileDialog::getOpenFileName(this, tr("Select FFS file to replace selected object"),".","FFS files (*.ffs *.bin);;All files (*.*)");
		}
		else if (mode == REPLACE_MODE_BODY) {
			if (item->subtype() == EFI_FV_FILETYPE_ALL || item->subtype() == EFI_FV_FILETYPE_RAW)
				path = QFileDialog::getOpenFileName(this, tr("Select raw file to replace body"),".","Raw files (*.raw *.bin);;All files (*.*)");
			else if (item->subtype() == EFI_FV_FILETYPE_PAD) // Pad file body can't be replaced
				return;
			else
				path = QFileDialog::getOpenFileName(this, tr("Select FFS file body to replace body"),".","FFS file body files (*.fbd *.bin);;All files (*.*)");
		}
		else
			return;
	}
	else if (item->type() == TreeItem::Section) {
		if (mode == REPLACE_MODE_AS_IS) {
			path = QFileDialog::getOpenFileName(this, tr("Select section file to replace selected object"),".","Section files (*.sec *.bin);;All files (*.*)");
		}
		else if (mode == REPLACE_MODE_BODY) {
			if (item->subtype() == EFI_SECTION_COMPRESSION || item->subtype() == EFI_SECTION_GUID_DEFINED || item->subtype() == EFI_SECTION_DISPOSABLE)
				path = QFileDialog::getOpenFileName(this, tr("Select FFS file body file to replace body"),".","FFS file body files (*.fbd *.bin);;All files (*.*)");
			else if (item->subtype() == EFI_SECTION_FIRMWARE_VOLUME_IMAGE)
				path = QFileDialog::getOpenFileName(this, tr("Select volume file to replace body"),".","Volume files (*.vol *.bin);;All files (*.*)");
			else if (item->subtype() == EFI_SECTION_RAW)
				path = QFileDialog::getOpenFileName(this, tr("Select raw file to replace body"),".","Raw files (*.raw *.bin);;All files (*.*)");
			else
				path = QFileDialog::getOpenFileName(this, tr("Select file to replace body"),".","Binary files (*.bin);;All files (*.*)");
		}
		else
			return;
	}
	else
		return;

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

    UINT8 result = ffsEngine->replace(index, buffer, mode);
    if (result)
        ui->statusBar->showMessage(tr("File can't be replaced (%1)").arg(result));
    else
        ui->actionSaveImageFile->setEnabled(true);
}

void UEFITool::extractAsIs()
{
    extract(EXTRACT_MODE_AS_IS);
}

void UEFITool::extractBody()
{
    extract(EXTRACT_MODE_BODY);
}

void UEFITool::extract(const UINT8 mode)
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    UINT8 type = item->type();

    QString path;
    if (mode == EXTRACT_MODE_AS_IS) {
        switch (type) {
        case TreeItem::Capsule:
            path = QFileDialog::getSaveFileName(this, tr("Save capsule to file"),".","Capsule files (*.cap *.bin);;All files (*.*)");
            break;
        case TreeItem::Image:
            path = QFileDialog::getSaveFileName(this, tr("Save image to file"),".","Image files (*.rom *.bin);;All files (*.*)");
            break;
        case TreeItem::Region:
            path = QFileDialog::getSaveFileName(this, tr("Save region to file"),".","Region files (*.rgn *.bin);;All files (*.*)");
            break;
        case TreeItem::Padding:
            path = QFileDialog::getSaveFileName(this, tr("Save padding to file"),".","Padding files (*.pad *.bin);;All files (*.*)");
            break;
        case TreeItem::Volume:
            path = QFileDialog::getSaveFileName(this, tr("Save volume to file"),".","Volume files (*.vol *.bin);;All files (*.*)");
            break;
        case TreeItem::File:
            path = QFileDialog::getSaveFileName(this, tr("Save FFS file to file"),".","FFS files (*.ffs *.bin);;All files (*.*)");
            break;
        case TreeItem::Section:
            path = QFileDialog::getSaveFileName(this, tr("Save section file to file"),".","Section files (*.sct *.bin);;All files (*.*)");
        break;
        default:
			path = QFileDialog::getSaveFileName(this, tr("Save object to file"),".","Binary files (*.bin);;All files (*.*)");
        }
    }
	else if (mode == EXTRACT_MODE_BODY) {
        switch (type) {
        case TreeItem::Capsule:
            path = QFileDialog::getSaveFileName(this, tr("Save capsule body to image file"),".","Image files (*.rom *.bin);;All files (*.*)");
            break;
        case TreeItem::File: {
				if (item->subtype() == EFI_FV_FILETYPE_ALL || item->subtype() == EFI_FV_FILETYPE_RAW)
					path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to raw file"),".","Raw files (*.raw *.bin);;All files (*.*)");
				else
					path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to file"),".","FFS file body files (*.fbd *.bin);;All files (*.*)");
			}
            break;
        case TreeItem::Section: {
			if (item->subtype() == EFI_SECTION_COMPRESSION || item->subtype() == EFI_SECTION_GUID_DEFINED || item->subtype() == EFI_SECTION_DISPOSABLE)
				path = QFileDialog::getSaveFileName(this, tr("Save encapsulated section body to FFS body file"),".","FFS file body files (*.fbd *.bin);;All files (*.*)");
			else if (item->subtype() == EFI_SECTION_FIRMWARE_VOLUME_IMAGE)
				path = QFileDialog::getSaveFileName(this, tr("Save section body to volume file"),".","Volume files (*.vol *.bin);;All files (*.*)");
			else if (item->subtype() == EFI_SECTION_RAW)
				path = QFileDialog::getSaveFileName(this, tr("Save section body to raw file"),".","Raw files (*.raw *.bin);;All files (*.*)");
			else
				path = QFileDialog::getSaveFileName(this, tr("Save section body to file"),".","Binary files (*.bin);;All files (*.*)");
			}
        break;
        default:
			path = QFileDialog::getSaveFileName(this, tr("Save object to file"),".","Binary files (*.bin);;All files (*.*)");
        }
    }
    else
        path = QFileDialog::getSaveFileName(this, tr("Save object to file"),".","Binary files (*.bin);;All files (*.*)");
    
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
    QString path = QFileDialog::getSaveFileName(this, tr("Save BIOS image file"),".","BIOS image files (*.rom *.bin *.cap *.bio *.fd *.wph *.efi);;All files (*.*)");

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

/*void UEFITool::resizeTreeViewColumns()
{
    int count = ffsEngine->model()->columnCount();
    for(int i = 0; i < count; i++)
        ui->structureTreeView->resizeColumnToContents(i);
}*/

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
