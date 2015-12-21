/* uefitool.cpp

  Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
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
ui(new Ui::UEFITool),
version(tr("0.21.5"))
{
    clipboard = QApplication::clipboard();

    // Create UI
    ui->setupUi(this);
    searchDialog = new SearchDialog(this);
    ffsEngine = NULL;

    // Set window title
    this->setWindowTitle(tr("UEFITool %1").arg(version));

    // Connect signals to slots
    connect(ui->actionOpenImageFile, SIGNAL(triggered()), this, SLOT(openImageFile()));
    connect(ui->actionOpenImageFileInNewWindow, SIGNAL(triggered()), this, SLOT(openImageFileInNewWindow()));
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
    connect(ui->actionMessagesCopy, SIGNAL(triggered()), this, SLOT(copyMessage()));
    connect(ui->actionMessagesCopyAll, SIGNAL(triggered()), this, SLOT(copyAllMessages()));
    connect(ui->actionMessagesClear, SIGNAL(triggered()), this, SLOT(clearMessages()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), this, SLOT(aboutQt()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(exit()));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(writeSettings()));

    // Enable Drag-and-Drop actions
    this->setAcceptDrops(true);

    // Set current directory
    currentDir = ".";

    // Set monospace font for some controls
    QFont font("Courier New", 10);
#if defined Q_OS_OSX
    font = QFont("Menlo", 10);
#elif defined Q_OS_WIN
    font = QFont("Consolas", 9);
#endif
    ui->infoEdit->setFont(font);
    ui->messageListWidget->setFont(font);
    ui->structureTreeView->setFont(font);
    searchDialog->ui->guidEdit->setFont(font);
    searchDialog->ui->hexEdit->setFont(font);

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

void UEFITool::setProgramPath(QString path)
{
    currentProgramPath = path;
};

void UEFITool::init()
{
    // Clear components
    ui->messageListWidget->clear();
    ui->infoEdit->clear();

    // Set window title
    this->setWindowTitle(tr("UEFITool %1").arg(version));

    // Disable menus
    ui->menuCapsuleActions->setDisabled(true);
    ui->menuImageActions->setDisabled(true);
    ui->menuRegionActions->setDisabled(true);
    ui->menuPaddingActions->setDisabled(true);
    ui->menuVolumeActions->setDisabled(true);
    ui->menuFileActions->setDisabled(true);
    ui->menuSectionActions->setDisabled(true);
    ui->actionMessagesCopy->setDisabled(true);
    ui->actionMessagesCopyAll->setDisabled(true);

    // Make new ffsEngine
    if (ffsEngine)
        delete ffsEngine;
    ffsEngine = new FfsEngine(this);
    ui->structureTreeView->setModel(ffsEngine->treeModel());

    // Connect
    connect(ui->structureTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(populateUi(const QModelIndex &)));
    connect(ui->messageListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scrollTreeView(QListWidgetItem*)));
    connect(ui->messageListWidget, SIGNAL(itemEntered(QListWidgetItem*)), this, SLOT(enableMessagesCopyActions(QListWidgetItem*)));
}

void UEFITool::populateUi(const QModelIndex &current)
{
    if (!current.isValid())
        return;

    TreeModel* model = ffsEngine->treeModel();
    UINT8 type = model->type(current);
    UINT8 subtype = model->subtype(current);

    // Set info text
    ui->infoEdit->setPlainText(model->info(current));

    // Enable menus
    ui->menuCapsuleActions->setEnabled(type == Types::Capsule);
    ui->menuImageActions->setEnabled(type == Types::Image);
    ui->menuRegionActions->setEnabled(type == Types::Region);
    ui->menuPaddingActions->setEnabled(type == Types::Padding);
    ui->menuVolumeActions->setEnabled(type == Types::Volume);
    ui->menuFileActions->setEnabled(type == Types::File);
    ui->menuSectionActions->setEnabled(type == Types::Section);

    // Enable actions
    ui->actionExtract->setDisabled(model->hasEmptyHeader(current) && model->hasEmptyBody(current));
    ui->actionRebuild->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionExtractBody->setDisabled(model->hasEmptyBody(current));
    ui->actionRemove->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionInsertInto->setEnabled((type == Types::Volume && subtype != Subtypes::UnknownVolume) ||
        (type == Types::File && subtype != EFI_FV_FILETYPE_ALL && subtype != EFI_FV_FILETYPE_RAW && subtype != EFI_FV_FILETYPE_PAD) ||
        (type == Types::Section && (subtype == EFI_SECTION_COMPRESSION || subtype == EFI_SECTION_GUID_DEFINED || subtype == EFI_SECTION_DISPOSABLE)));
    ui->actionInsertBefore->setEnabled(type == Types::File || type == Types::Section);
    ui->actionInsertAfter->setEnabled(type == Types::File || type == Types::Section);
    ui->actionReplace->setEnabled((type == Types::Region && subtype != Subtypes::DescriptorRegion) || type == Types::Padding || type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionReplaceBody->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionMessagesCopy->setEnabled(false);
}

void UEFITool::search()
{
    if (searchDialog->exec() != QDialog::Accepted)
        return;

    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    int index = searchDialog->ui->tabWidget->currentIndex();
    if (index == 0) { // Hex pattern
        searchDialog->ui->hexEdit->setFocus();
        QByteArray pattern = searchDialog->ui->hexEdit->text().toLatin1().replace(" ", "");
        if (pattern.isEmpty())
            return;
        UINT8 mode;
        if (searchDialog->ui->hexScopeHeaderRadioButton->isChecked())
            mode = SEARCH_MODE_HEADER;
        else if (searchDialog->ui->hexScopeBodyRadioButton->isChecked())
            mode = SEARCH_MODE_BODY;
        else
            mode = SEARCH_MODE_ALL;
        ffsEngine->findHexPattern(rootIndex, pattern, mode);
        showMessages();
    }
    else if (index == 1) { // GUID
        searchDialog->ui->guidEdit->setFocus();
        searchDialog->ui->guidEdit->setCursorPosition(0);
        QByteArray pattern = searchDialog->ui->guidEdit->text().toLatin1();
        if (pattern.isEmpty())
            return;
        UINT8 mode;
        if (searchDialog->ui->guidScopeHeaderRadioButton->isChecked())
            mode = SEARCH_MODE_HEADER;
        else if (searchDialog->ui->guidScopeBodyRadioButton->isChecked())
            mode = SEARCH_MODE_BODY;
        else
            mode = SEARCH_MODE_ALL;
        ffsEngine->findGuidPattern(rootIndex, pattern, mode);
        showMessages();
    }
    else if (index == 2) { // Text string
        searchDialog->ui->textEdit->setFocus();
        QString pattern = searchDialog->ui->textEdit->text();
        if (pattern.isEmpty())
            return;
        ffsEngine->findTextPattern(rootIndex, pattern, searchDialog->ui->textUnicodeCheckBox->isChecked(),
            (Qt::CaseSensitivity) searchDialog->ui->textCaseSensitiveCheckBox->isChecked());
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

    TreeModel* model = ffsEngine->treeModel();
    UINT8 type;

    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
        type = model->type(index.parent());
    else
        type = model->type(index);

    QString path;
    switch (type) {
    case Types::Volume:
        path = QFileDialog::getOpenFileName(this, tr("Select FFS file to insert"), currentDir, "FFS files (*.ffs *.bin);;All files (*)");
        break;
    case Types::File:
    case Types::Section:
        path = QFileDialog::getOpenFileName(this, tr("Select section file to insert"), currentDir, "Section files (*.sct *.bin);;All files (*)");
        break;
    default:
        return;
    }

    if (path.trimmed().isEmpty())
        return;

    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists()) {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, tr("Insertion failed"), tr("Can't open output file for reading"), QMessageBox::Ok);
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->insert(index, buffer, mode);
    if (result) {
        QMessageBox::critical(this, tr("Insertion failed"), errorMessage(result), QMessageBox::Ok);
        return;
    }
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

    TreeModel* model = ffsEngine->treeModel();
    QString path;
    if (model->type(index) == Types::Region) {
        if (mode == REPLACE_MODE_AS_IS) {
            path = QFileDialog::getOpenFileName(this, tr("Select region file to replace selected object"), currentDir, "Region files (*.rgn *.bin);;All files (*)");
        }
        else
            return;
    }
    else if (model->type(index) == Types::Padding) {
        if (mode == REPLACE_MODE_AS_IS) {
            path = QFileDialog::getOpenFileName(this, tr("Select padding file to replace selected object"), currentDir, "Padding files (*.pad *.bin);;All files (*)");
        }
        else
            return;
    }
    else if (model->type(index) == Types::Volume) {
        if (mode == REPLACE_MODE_AS_IS) {
            path = QFileDialog::getOpenFileName(this, tr("Select volume file to replace selected object"), currentDir, "Volume files (*.vol *.bin);;All files (*)");
        }
        else if (mode == REPLACE_MODE_BODY) {
            path = QFileDialog::getOpenFileName(this, tr("Select volume body file to replace body"), currentDir, "Volume body files (*.vbd *.bin);;All files (*)");
        }
        else
            return;
    }
    else if (model->type(index) == Types::File) {
        if (mode == REPLACE_MODE_AS_IS) {
            path = QFileDialog::getOpenFileName(this, tr("Select FFS file to replace selected object"), currentDir, "FFS files (*.ffs *.bin);;All files (*)");
        }
        else if (mode == REPLACE_MODE_BODY) {
            if (model->subtype(index) == EFI_FV_FILETYPE_ALL || model->subtype(index) == EFI_FV_FILETYPE_RAW)
                path = QFileDialog::getOpenFileName(this, tr("Select raw file to replace body"), currentDir, "Raw files (*.raw *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_FV_FILETYPE_PAD) // Pad file body can't be replaced
                //!TODO: handle non-empty pad files
                return;
            else
                path = QFileDialog::getOpenFileName(this, tr("Select FFS file body to replace body"), currentDir, "FFS file body files (*.fbd *.bin);;All files (*)");
        }
        else
            return;
    }
    else if (model->type(index) == Types::Section) {
        if (mode == REPLACE_MODE_AS_IS) {
            path = QFileDialog::getOpenFileName(this, tr("Select section file to replace selected object"), currentDir, "Section files (*.sct *.bin);;All files (*)");
        }
        else if (mode == REPLACE_MODE_BODY) {
            if (model->subtype(index) == EFI_SECTION_COMPRESSION || model->subtype(index) == EFI_SECTION_GUID_DEFINED || model->subtype(index) == EFI_SECTION_DISPOSABLE)
                path = QFileDialog::getOpenFileName(this, tr("Select FFS file body file to replace body"), currentDir, "FFS file body files (*.fbd *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_SECTION_FIRMWARE_VOLUME_IMAGE)
                path = QFileDialog::getOpenFileName(this, tr("Select volume file to replace body"), currentDir, "Volume files (*.vol *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_SECTION_RAW)
                path = QFileDialog::getOpenFileName(this, tr("Select raw file to replace body"), currentDir, "Raw files (*.raw *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_SECTION_PE32 || model->subtype(index) == EFI_SECTION_TE || model->subtype(index) == EFI_SECTION_PIC)
                path = QFileDialog::getOpenFileName(this, tr("Select EFI executable file to replace body"), currentDir, "EFI executable files (*.efi *.dxe *.pei *.bin);;All files (*)");
            else
                path = QFileDialog::getOpenFileName(this, tr("Select file to replace body"), currentDir, "Binary files (*.bin);;All files (*)");
        }
        else
            return;
    }
    else
        return;

    if (path.trimmed().isEmpty())
        return;

    QFileInfo fileInfo = QFileInfo(path);
    if (!fileInfo.exists()) {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, tr("Replacing failed"), tr("Can't open input file for reading"), QMessageBox::Ok);
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->replace(index, buffer, mode);
    if (result) {
        QMessageBox::critical(this, tr("Replacing failed"), errorMessage(result), QMessageBox::Ok);
        return;
    }
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

    TreeModel* model = ffsEngine->treeModel();
    UINT8 type = model->type(index);

    QString path;
    if (mode == EXTRACT_MODE_AS_IS) {
        switch (type) {
        case Types::Capsule:
            path = QFileDialog::getSaveFileName(this, tr("Save capsule to file"), currentDir, "Capsule files (*.cap *.bin);;All files (*)");
            break;
        case Types::Image:
            path = QFileDialog::getSaveFileName(this, tr("Save image to file"), currentDir, "Image files (*.rom *.bin);;All files (*)");
            break;
        case Types::Region:
            path = QFileDialog::getSaveFileName(this, tr("Save region to file"), currentDir, "Region files (*.rgn *.bin);;All files (*)");
            break;
        case Types::Padding:
            path = QFileDialog::getSaveFileName(this, tr("Save padding to file"), currentDir, "Padding files (*.pad *.bin);;All files (*)");
            break;
        case Types::Volume:
            path = QFileDialog::getSaveFileName(this, tr("Save volume to file"), currentDir, "Volume files (*.vol *.bin);;All files (*)");
            break;
        case Types::File:
            path = QFileDialog::getSaveFileName(this, tr("Save FFS file to file"), currentDir, "FFS files (*.ffs *.bin);;All files (*)");
            break;
        case Types::Section:
            path = QFileDialog::getSaveFileName(this, tr("Save section file to file"), currentDir, "Section files (*.sct *.bin);;All files (*)");
            break;
        default:
            path = QFileDialog::getSaveFileName(this, tr("Save object to file"), currentDir, "Binary files (*.bin);;All files (*)");
        }
    }
    else if (mode == EXTRACT_MODE_BODY) {
        switch (type) {
        case Types::Capsule:
            path = QFileDialog::getSaveFileName(this, tr("Save capsule body to image file"), currentDir, "Image files (*.rom *.bin);;All files (*)");
            break;
        case Types::Volume:
            path = QFileDialog::getSaveFileName(this, tr("Save volume body to file"), currentDir, "Volume body files (*.vbd *.bin);;All files (*)");
            break;
        case Types::File: {
            if (model->subtype(index) == EFI_FV_FILETYPE_ALL || model->subtype(index) == EFI_FV_FILETYPE_RAW)
                path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to raw file"), currentDir, "Raw files (*.raw *.bin);;All files (*)");
            else
                path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to file"), currentDir, "FFS file body files (*.fbd *.bin);;All files (*)");
        }
            break;
        case Types::Section: {
            if (model->subtype(index) == EFI_SECTION_COMPRESSION || model->subtype(index) == EFI_SECTION_GUID_DEFINED || model->subtype(index) == EFI_SECTION_DISPOSABLE)
                path = QFileDialog::getSaveFileName(this, tr("Save encapsulation section body to FFS body file"), currentDir, "FFS file body files (*.fbd *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_SECTION_FIRMWARE_VOLUME_IMAGE)
                path = QFileDialog::getSaveFileName(this, tr("Save section body to volume file"), currentDir, "Volume files (*.vol *.bin);;All files (*)");
            else if (model->subtype(index) == EFI_SECTION_RAW)
                path = QFileDialog::getSaveFileName(this, tr("Save section body to raw file"), currentDir, "Raw files (*.raw *.bin);;All files (*)");
            else
                path = QFileDialog::getSaveFileName(this, tr("Save section body to file"), currentDir, "Binary files (*.bin);;All files (*)");
        }
            break;
        default:
            path = QFileDialog::getSaveFileName(this, tr("Save object to file"), currentDir, "Binary files (*.bin);;All files (*)");
        }
    }
    else
        path = QFileDialog::getSaveFileName(this, tr("Save object to file"), currentDir, "Binary files (*.bin);;All files (*)");

    if (path.trimmed().isEmpty())
        return;

    QByteArray extracted;
    UINT8 result = ffsEngine->extract(index, extracted, mode);
    if (result) {
        QMessageBox::critical(this, tr("Extraction failed"), errorMessage(result), QMessageBox::Ok);
        return;
    }

    QFile outputFile;
    outputFile.setFileName(path);
    if (!outputFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, tr("Extraction failed"), tr("Can't open output file for rewriting"), QMessageBox::Ok);
        return;
    }
    outputFile.resize(0);
    outputFile.write(extracted);
    outputFile.close();
}

void UEFITool::about()
{
    QMessageBox::about(this, tr("About UEFITool"), tr(
        "Copyright (c) 2015, Nikolaj Schlej aka <b>CodeRush</b>.<br>"
        "Program icon made by <a href=https://www.behance.net/alzhidkov>Alexander Zhidkov</a>.<br><br>"
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
    QString path = QFileDialog::getSaveFileName(this, tr("Save BIOS image file"), currentDir, "BIOS image files (*.rom *.bin *.cap *.bio *.fd *.wph *.dec);;All files (*)");

    if (path.isEmpty())
        return;

    QByteArray reconstructed;
    UINT8 result = ffsEngine->reconstructImageFile(reconstructed);
    showMessages();
    if (result) {
        QMessageBox::critical(this, tr("Image reconstruction failed"), errorMessage(result), QMessageBox::Ok);
        return;
    }

    QFile outputFile;
    outputFile.setFileName(path);

    if (!outputFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, tr("Image reconstruction failed"), tr("Can't open output file for rewriting"), QMessageBox::Ok);
        return;
    }

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();
    if (QMessageBox::information(this, tr("Image reconstruction successful"), tr("Open reconstructed file?"), QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::Yes)
        openImageFile(path);
}

void UEFITool::openImageFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file"), currentDir, "BIOS image files (*.rom *.bin *.cap *.bio *.fd *.wph *.dec);;All files (*)");
    openImageFile(path);
}

void UEFITool::openImageFileInNewWindow()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file in new window"), currentDir, "BIOS image files (*.rom *.bin *.cap *.bio *.fd *.wph *.dec);;All files (*)");
    if (path.trimmed().isEmpty())
        return;
    QProcess::startDetached(currentProgramPath, QStringList(path));
}

void UEFITool::openImageFile(QString path)
{
    if (path.trimmed().isEmpty())
        return;

    QFileInfo fileInfo = QFileInfo(path);

    if (!fileInfo.exists()) {
        ui->statusBar->showMessage(tr("Please select existing file"));
        return;
    }

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, tr("Image parsing failed"), tr("Can't open input file for reading"), QMessageBox::Ok);
        return;
    }

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    init();
    this->setWindowTitle(tr("UEFITool %1 - %2").arg(version).arg(fileInfo.fileName()));

    UINT8 result = ffsEngine->parseImageFile(buffer);
    showMessages();
    if (result)
        QMessageBox::critical(this, tr("Image parsing failed"), errorMessage(result), QMessageBox::Ok);
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));

    // Enable search
    ui->actionSearch->setEnabled(true);

    // Set current directory
    currentDir = fileInfo.absolutePath();
}

void UEFITool::copyMessage()
{
    clipboard->clear();
    clipboard->setText(ui->messageListWidget->currentItem()->text());
}

void UEFITool::copyAllMessages()
{
    QString text;
    clipboard->clear();
    for(INT32 i = 0; i < ui->messageListWidget->count(); i++)
        text.append(ui->messageListWidget->item(i)->text()).append("\n");

    clipboard->clear();
    clipboard->setText(text);
}

void UEFITool::enableMessagesCopyActions(QListWidgetItem* item)
{
    ui->actionMessagesCopy->setEnabled(item != NULL);
    ui->actionMessagesCopyAll->setEnabled(item != NULL);
}

void UEFITool::clearMessages()
{
    ffsEngine->clearMessages();
    messageItems.clear();
    ui->messageListWidget->clear();
    ui->actionMessagesCopy->setEnabled(false);
    ui->actionMessagesCopyAll->setEnabled(false);
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

    ui->messageListWidget->scrollToBottom();
}

void UEFITool::scrollTreeView(QListWidgetItem* item)
{
    MessageListItem* messageItem = static_cast<MessageListItem*>(item);
    QModelIndex index = messageItem->index();
    if (index.isValid()) {
        ui->structureTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
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

    if (!ui->structureTreeView->underMouse())
        return;

    QPoint pt = event->pos();
    QModelIndex index = ui->structureTreeView->indexAt(ui->structureTreeView->viewport()->mapFrom(this, pt));
    if (!index.isValid())
        return;

    TreeModel* model = ffsEngine->treeModel();
    switch (model->type(index))
    {
    case Types::Capsule:
        ui->menuCapsuleActions->exec(event->globalPos());
        break;
    case Types::Image:
        ui->menuImageActions->exec(event->globalPos());
        break;
    case Types::Region:
        ui->menuRegionActions->exec(event->globalPos());
        break;
    case Types::Padding:
        ui->menuPaddingActions->exec(event->globalPos());
        break;
    case Types::Volume:
        ui->menuVolumeActions->exec(event->globalPos());
        break;
    case Types::File:
        ui->menuFileActions->exec(event->globalPos());
        break;
    case Types::Section:
        ui->menuSectionActions->exec(event->globalPos());
        break;
    }
}

void UEFITool::readSettings()
{
    QSettings settings(this);
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
}

void UEFITool::writeSettings()
{
    QSettings settings(this);
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
}
