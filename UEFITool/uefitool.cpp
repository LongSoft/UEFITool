/* uefitool.cpp

  Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include "../version.h"
#include "uefitool.h"
#include "ui_uefitool.h"

UEFITool::UEFITool(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::UEFITool),
version(tr(PROGRAM_VERSION)),
markingEnabled(true)
{
    clipboard = QApplication::clipboard();

    // Create UI
    ui->setupUi(this);
    searchDialog = new SearchDialog(this);
    hexViewDialog = new HexViewDialog(this);
    goToAddressDialog = new GoToAddressDialog(this);
    goToBaseDialog = new GoToBaseDialog(this);
    model = NULL;
    ffsParser = NULL;
    ffsFinder = NULL;
    ffsOps = NULL;
    ffsBuilder = NULL;
    ffsReport = NULL;

    // Connect signals to slots
    connect(ui->actionOpenImageFile, SIGNAL(triggered()), this, SLOT(openImageFile()));
    connect(ui->actionOpenImageFileInNewWindow, SIGNAL(triggered()), this, SLOT(openImageFileInNewWindow()));
    connect(ui->actionSaveImageFile, SIGNAL(triggered()), this, SLOT(saveImageFile()));
    connect(ui->actionSearch, SIGNAL(triggered()), this, SLOT(search()));
    connect(ui->actionHexView, SIGNAL(triggered()), this, SLOT(hexView()));
    connect(ui->actionBodyHexView, SIGNAL(triggered()), this, SLOT(bodyHexView()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(extractAsIs()));
    connect(ui->actionExtractBody, SIGNAL(triggered()), this, SLOT(extractBody()));
    connect(ui->actionExtractBodyUncompressed, SIGNAL(triggered()), this, SLOT(extractBodyUncompressed()));
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
    connect(ui->actionGoToData, SIGNAL(triggered()), this, SLOT(goToData()));
    connect(ui->actionGoToBase, SIGNAL(triggered()), this, SLOT(goToBase()));
    connect(ui->actionGoToAddress, SIGNAL(triggered()), this, SLOT(goToAddress()));
    connect(ui->actionLoadGuidDatabase, SIGNAL(triggered()), this, SLOT(loadGuidDatabase()));
    connect(ui->actionUnloadGuidDatabase, SIGNAL(triggered()), this, SLOT(unloadGuidDatabase()));
    connect(ui->actionLoadDefaultGuidDatabase, SIGNAL(triggered()), this, SLOT(loadDefaultGuidDatabase()));
    connect(ui->actionExportDiscoveredGuids, SIGNAL(triggered()), this, SLOT(exportDiscoveredGuids()));
    connect(ui->actionGenerateReport, SIGNAL(triggered()), this, SLOT(generateReport()));
    connect(ui->actionToggleBootGuardMarking, SIGNAL(toggled(bool)), this, SLOT(toggleBootGuardMarking(bool)));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(writeSettings()));

    // Enable Drag-and-Drop actions
    setAcceptDrops(true);

    // Disable Builder tab, doesn't work right now
    ui->messagesTabWidget->setTabEnabled(TAB_BUILDER, false);

    // Set current directory
    currentDir = ".";

    // Load built-in GUID database
    initGuidDatabase(":/guids.csv");

    // Initialize non-persistent data
    init();

    // Read stored settings
    readSettings();
}

UEFITool::~UEFITool()
{
    delete ffsBuilder;
    delete ffsOps;
    delete ffsFinder;
    delete ffsParser;
    delete ffsReport;
    delete model;
    delete hexViewDialog;
    delete searchDialog;
    delete ui;
}

void UEFITool::init()
{
    // Clear components
    ui->parserMessagesListWidget->clear();
    ui->finderMessagesListWidget->clear();
    ui->fitTableWidget->clear();
    ui->fitTableWidget->setRowCount(0);
    ui->fitTableWidget->setColumnCount(0);
    ui->infoEdit->clear();
    ui->securityEdit->clear();
    ui->messagesTabWidget->setTabEnabled(TAB_FIT, false);
    ui->messagesTabWidget->setTabEnabled(TAB_SECURITY, false);
    ui->messagesTabWidget->setTabEnabled(TAB_SEARCH, false);
    ui->messagesTabWidget->setTabEnabled(TAB_BUILDER, false);

    // Set window title
    setWindowTitle(tr("UEFITool %1").arg(version));

    // Disable menus
    ui->actionSearch->setEnabled(false);
    ui->actionGoToBase->setEnabled(false);
    ui->actionGoToAddress->setEnabled(false);
    ui->menuCapsuleActions->setEnabled(false);
    ui->menuImageActions->setEnabled(false);
    ui->menuRegionActions->setEnabled(false);
    ui->menuPaddingActions->setEnabled(false);
    ui->menuVolumeActions->setEnabled(false);
    ui->menuFileActions->setEnabled(false);
    ui->menuSectionActions->setEnabled(false);
    ui->menuStoreActions->setEnabled(false);
    ui->menuEntryActions->setEnabled(false);
    ui->menuMessageActions->setEnabled(false);

    // Create new model ...
    delete model;
    model = new TreeModel();
    ui->structureTreeView->setModel(model);
    // ... and ffsParser
    delete ffsParser;
    ffsParser = new FfsParser(model);

    // Set proper marking state
    model->setMarkingEnabled(markingEnabled);
    ui->actionToggleBootGuardMarking->setChecked(markingEnabled);

    // Connect
    connect(ui->structureTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(populateUi(const QModelIndex &)));
    connect(ui->structureTreeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(populateUi(const QItemSelection &)));
    connect(ui->parserMessagesListWidget,  SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scrollTreeView(QListWidgetItem*)));
    connect(ui->parserMessagesListWidget,  SIGNAL(itemEntered(QListWidgetItem*)),       this, SLOT(enableMessagesCopyActions(QListWidgetItem*)));
    connect(ui->finderMessagesListWidget,  SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scrollTreeView(QListWidgetItem*)));
    connect(ui->finderMessagesListWidget,  SIGNAL(itemEntered(QListWidgetItem*)),       this, SLOT(enableMessagesCopyActions(QListWidgetItem*)));
    connect(ui->builderMessagesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scrollTreeView(QListWidgetItem*)));
    connect(ui->builderMessagesListWidget, SIGNAL(itemEntered(QListWidgetItem*)),       this, SLOT(enableMessagesCopyActions(QListWidgetItem*)));
    connect(ui->fitTableWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(scrollTreeView(QTableWidgetItem*)));
    connect(ui->messagesTabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));

    // allow enter/return pressing to scroll tree view
    ui->parserMessagesListWidget->installEventFilter(this);
    ui->finderMessagesListWidget->installEventFilter(this);
    ui->builderMessagesListWidget->installEventFilter(this);
}

void UEFITool::populateUi(const QItemSelection &selected)
{
    if (selected.isEmpty()) {
        return;
    }

    populateUi(selected.indexes().at(0));
}

void UEFITool::populateUi(const QModelIndex &current)
{
    // Check sanity
    if (!current.isValid()) {
        return;
    }

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
    ui->menuEntryActions->setEnabled(type == Types::Microcode
        || type == Types::SlicData
        || type == Types::NvarEntry
        || type == Types::VssEntry
        || type == Types::FsysEntry
        || type == Types::EvsaEntry
        || type == Types::FlashMapEntry
        || type == Types::IfwiHeader
        || type == Types::IfwiPartition
        || type == Types::FptPartition
        || type == Types::FptEntry
        || type == Types::BpdtPartition
        || type == Types::BpdtEntry
        || type == Types::CpdPartition
        || type == Types::CpdEntry
        || type == Types::CpdExtension
        || type == Types::CpdSpiEntry
        );
    ui->menuStoreActions->setEnabled(type == Types::VssStore
        || type == Types::Vss2Store
        || type == Types::FdcStore
        || type == Types::FsysStore
        || type == Types::EvsaStore
        || type == Types::FtwStore
        || type == Types::FlashMapStore
        || type == Types::CmdbStore
        || type == Types::FptStore
        || type == Types::BpdtStore
        || type == Types::CpdStore
        );

    // Enable actions
    ui->actionHexView->setDisabled(model->hasEmptyHeader(current) && model->hasEmptyBody(current) && model->hasEmptyTail(current));
    ui->actionBodyHexView->setDisabled(model->hasEmptyBody(current));
    ui->actionExtract->setDisabled(model->hasEmptyHeader(current) && model->hasEmptyBody(current) && model->hasEmptyTail(current));
    ui->actionGoToData->setEnabled(type == Types::NvarEntry && subtype == Subtypes::LinkNvarEntry);

    // Disable rebuild for now
    //ui->actionRebuild->setDisabled(type == Types::Region && subtype == Subtypes::DescriptorRegion);
    //ui->actionReplace->setDisabled(type == Types::Region && subtype == Subtypes::DescriptorRegion);

    //ui->actionRebuild->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionExtractBody->setDisabled(model->hasEmptyBody(current));
    ui->actionExtractBodyUncompressed->setEnabled(enableExtractBodyUncompressed(current));
    //ui->actionRemove->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    //ui->actionInsertInto->setEnabled((type == Types::Volume && subtype != Subtypes::UnknownVolume) ||
    //    (type == Types::File && subtype != EFI_FV_FILETYPE_ALL && subtype != EFI_FV_FILETYPE_RAW && subtype != EFI_FV_FILETYPE_PAD) ||
    //    (type == Types::Section && (subtype == EFI_SECTION_COMPRESSION || subtype == EFI_SECTION_GUID_DEFINED || subtype == EFI_SECTION_DISPOSABLE)));
    //ui->actionInsertBefore->setEnabled(type == Types::File || type == Types::Section);
    //ui->actionInsertAfter->setEnabled(type == Types::File || type == Types::Section);
    //ui->actionReplace->setEnabled((type == Types::Region && subtype != Subtypes::DescriptorRegion) || type == Types::Volume || type == Types::File || type == Types::Section);
    //ui->actionReplaceBody->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);

    ui->menuMessageActions->setEnabled(false);
}

bool UEFITool::enableExtractBodyUncompressed(const QModelIndex &current)
{
    // TODO: rewrite based on model->compressed()
    U_UNUSED_PARAMETER(current);
    return false;
}

void UEFITool::search()
{
    if (searchDialog->exec() != QDialog::Accepted)
        return;

    QModelIndex rootIndex = model->index(0, 0);

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
        ffsFinder->findHexPattern(rootIndex, pattern, mode);
        showFinderMessages();
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
        ffsFinder->findGuidPattern(rootIndex, pattern, mode);
        showFinderMessages();
    }
    else if (index == 2) { // Text string
        searchDialog->ui->textEdit->setFocus();
        QString pattern = searchDialog->ui->textEdit->text();
        if (pattern.isEmpty())
            return;
        UINT8 mode;
        if (searchDialog->ui->textScopeHeaderRadioButton->isChecked())
            mode = SEARCH_MODE_HEADER;
        else if (searchDialog->ui->textScopeBodyRadioButton->isChecked())
            mode = SEARCH_MODE_BODY;
        else
            mode = SEARCH_MODE_ALL;
        ffsFinder->findTextPattern(rootIndex, pattern, mode, searchDialog->ui->textUnicodeCheckBox->isChecked(),
            (Qt::CaseSensitivity) searchDialog->ui->textCaseSensitiveCheckBox->isChecked());
        showFinderMessages();
    }
}

void UEFITool::hexView()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    hexViewDialog->setItem(index, false);
    hexViewDialog->exec();
}

void UEFITool::bodyHexView()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    hexViewDialog->setItem(index, true);
    hexViewDialog->exec();
}

void UEFITool::goToBase()
{
    goToBaseDialog->ui->hexSpinBox->setFocus();
    goToBaseDialog->ui->hexSpinBox->selectAll();
    if (goToBaseDialog->exec() != QDialog::Accepted)
        return;

    UINT32 offset = (UINT32)goToBaseDialog->ui->hexSpinBox->value();
    QModelIndex index = model->findByBase(offset);
    if (index.isValid()) {
        ui->structureTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
        ui->structureTreeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void UEFITool::goToAddress()
{
    goToAddressDialog->ui->hexSpinBox->setFocus();
    goToAddressDialog->ui->hexSpinBox->selectAll();
    if (goToAddressDialog->exec() != QDialog::Accepted)
        return;

    UINT32 address = (UINT32)goToAddressDialog->ui->hexSpinBox->value();
    QModelIndex index = model->findByBase(address - (UINT32)ffsParser->getAddressDiff());
    if (index.isValid()) {
        ui->structureTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
        ui->structureTreeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void UEFITool::goToData()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid() || model->type(index) != Types::NvarEntry || model->subtype(index) != Subtypes::LinkNvarEntry)
        return;

    // Get parent
    QModelIndex parent = model->parent(index);

    for (int i = index.row(); i < model->rowCount(parent); i++) {
        if (model->hasEmptyParsingData(index))
            continue;

        UByteArray rdata = model->parsingData(index);
        const NVAR_ENTRY_PARSING_DATA* pdata = (const NVAR_ENTRY_PARSING_DATA*)rdata.constData();
        UINT32 lastVariableFlag = pdata->emptyByte ? 0xFFFFFF : 0;
        UINT32 offset = model->offset(index);
        if (pdata->next == lastVariableFlag) {
            ui->structureTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            ui->structureTreeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
        }

        for (int j = i + 1; j < model->rowCount(parent); j++) {
            QModelIndex currentIndex = parent.child(j, 0);
            if (model->hasEmptyParsingData(currentIndex))
                continue;

            if (model->offset(currentIndex) == offset + pdata->next) {
                index = currentIndex;
                break;
            }
        }
    }
}

void UEFITool::insert(const UINT8 mode)
{
    U_UNUSED_PARAMETER(mode);
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
    U_UNUSED_PARAMETER(mode);

}

void UEFITool::extractAsIs()
{
    extract(EXTRACT_MODE_AS_IS);
}

void UEFITool::extractBody()
{
    extract(EXTRACT_MODE_BODY);
}

void UEFITool::extractBodyUncompressed()
{
    extract(EXTRACT_MODE_BODY_UNCOMPRESSED);
}

void UEFITool::extract(const UINT8 mode)
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    QByteArray extracted;
    QString name;
    USTATUS result = ffsOps->extract(index, name, extracted, mode);
    if (result) {
        QMessageBox::critical(this, tr("Extraction failed"), errorCodeToUString(result), QMessageBox::Ok);
        return;
    }

    name = QDir::toNativeSeparators(currentDir + QDir::separator() + name);

    //ui->statusBar->showMessage(name);

    UINT8 type = model->type(index);
    UINT8 subtype = model->subtype(index);
    QString path;
    if (mode == EXTRACT_MODE_AS_IS) {
        switch (type) {
        case Types::Capsule:        path = QFileDialog::getSaveFileName(this, tr("Save capsule to file"),          name + ".cap",  tr("Capsule files (*.cap *.bin);;All files (*)"));          break;
        case Types::Image:          path = QFileDialog::getSaveFileName(this, tr("Save image to file"),            name + ".rom",  tr("Image files (*.rom *.bin);;All files (*)"));            break;
        case Types::Region:         path = QFileDialog::getSaveFileName(this, tr("Save region to file"),           name + ".rgn",  tr("Region files (*.rgn *.bin);;All files (*)"));           break;
        case Types::Padding:        path = QFileDialog::getSaveFileName(this, tr("Save padding to file"),          name + ".pad",  tr("Padding files (*.pad *.bin);;All files (*)"));          break;
        case Types::Volume:         path = QFileDialog::getSaveFileName(this, tr("Save volume to file"),           name + ".vol",  tr("Volume files (*.vol *.bin);;All files (*)"));           break;
        case Types::File:           path = QFileDialog::getSaveFileName(this, tr("Save FFS file to file"),         name + ".ffs",  tr("FFS files (*.ffs *.bin);;All files (*)"));              break;
        case Types::Section:        path = QFileDialog::getSaveFileName(this, tr("Save section to file"),          name + ".sct",  tr("Section files (*.sct *.bin);;All files (*)"));          break;
        case Types::NvarEntry:      path = QFileDialog::getSaveFileName(this, tr("Save NVAR entry to file"),       name + ".nvar", tr("NVAR entry files (*.nvar *.bin);;All files (*)"));      break;
        case Types::VssEntry:       path = QFileDialog::getSaveFileName(this, tr("Save VSS entry to file"),        name + ".vss",  tr("VSS entry files (*.vss *.bin);;All files (*)"));        break;
        case Types::FsysEntry:      path = QFileDialog::getSaveFileName(this, tr("Save Fsys entry to file"),       name + ".fse",  tr("Fsys entry files (*.fse *.bin);;All files (*)"));       break;
        case Types::EvsaEntry:      path = QFileDialog::getSaveFileName(this, tr("Save EVSA entry to file"),       name + ".evse", tr("EVSA entry files (*.evse *.bin);;All files (*)"));      break;
        case Types::FlashMapEntry:  path = QFileDialog::getSaveFileName(this, tr("Save FlashMap entry to file"),   name + ".fme",  tr("FlashMap entry files (*.fme *.bin);;All files (*)"));   break;
        case Types::VssStore:       path = QFileDialog::getSaveFileName(this, tr("Save VSS store to file"),        name + ".vss",  tr("VSS store files (*.vss *.bin);;All files (*)"));        break;
        case Types::Vss2Store:      path = QFileDialog::getSaveFileName(this, tr("Save VSS2 store to file"),       name + ".vss2", tr("VSS2 store files (*.vss2 *.bin);;All files (*)"));      break;
        case Types::FdcStore:       path = QFileDialog::getSaveFileName(this, tr("Save FDC store to file"),        name + ".fdc",  tr("FDC store files (*.fdc *.bin);;All files (*)"));        break;
        case Types::FsysStore:      path = QFileDialog::getSaveFileName(this, tr("Save Fsys store to file"),       name + ".fsys", tr("Fsys store files (*.fsys *.bin);;All files (*)"));      break;
        case Types::EvsaStore:      path = QFileDialog::getSaveFileName(this, tr("Save EVSA store to file"),       name + ".evsa", tr("EVSA store files (*.evsa *.bin);;All files (*)"));      break;
        case Types::FtwStore:       path = QFileDialog::getSaveFileName(this, tr("Save FTW store to file"),        name + ".ftw",  tr("FTW store files (*.ftw *.bin);;All files (*)"));        break;
        case Types::FlashMapStore:  path = QFileDialog::getSaveFileName(this, tr("Save FlashMap store to file"),   name + ".fmap", tr("FlashMap store files (*.fmap *.bin);;All files (*)"));  break;
        case Types::CmdbStore:      path = QFileDialog::getSaveFileName(this, tr("Save CMDB store to file"),       name + ".cmdb", tr("CMDB store files (*.cmdb *.bin);;All files (*)"));      break;
        case Types::Microcode:      path = QFileDialog::getSaveFileName(this, tr("Save microcode binary to file"), name + ".ucd",  tr("Microcode binary files (*.ucd *.bin);;All files (*)")); break;
        case Types::SlicData:
            if (subtype == Subtypes::PubkeySlicData) path = QFileDialog::getSaveFileName(this, tr("Save SLIC pubkey to file"), name + ".spk", tr("SLIC pubkey files (*.spk *.bin);;All files (*)"));
            else                                     path = QFileDialog::getSaveFileName(this, tr("Save SLIC marker to file"), name + ".smk", tr("SLIC marker files (*.smk *.bin);;All files (*)"));
            break;
        default:                    path = QFileDialog::getSaveFileName(this, tr("Save object to file"), name + ".bin", tr("Binary files (*.bin);;All files (*)"));
        }
    }
    else if (mode == EXTRACT_MODE_BODY || mode == EXTRACT_MODE_BODY_UNCOMPRESSED) {
        switch (type) {
        case Types::Capsule:                         path = QFileDialog::getSaveFileName(this, tr("Save capsule body to image file"), name + ".rom", tr("Image files (*.rom *.bin);;All files (*)"));       break;
        case Types::Volume:                          path = QFileDialog::getSaveFileName(this, tr("Save volume body to file"),        name + ".vbd", tr("Volume body files (*.vbd *.bin);;All files (*)")); break;
        case Types::File:
            if (subtype    == EFI_FV_FILETYPE_ALL
                || subtype == EFI_FV_FILETYPE_RAW)   path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to raw file"),  name + ".raw", tr("Raw files (*.raw *.bin);;All files (*)"));
            else                                     path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to file"),      name + ".fbd", tr("FFS file body files (*.fbd *.bin);;All files (*)"));
            break;
        case Types::Section:
            if (subtype    == EFI_SECTION_COMPRESSION
                || subtype == EFI_SECTION_GUID_DEFINED
                || subtype == EFI_SECTION_DISPOSABLE)              path = QFileDialog::getSaveFileName(this, tr("Save encapsulation section body to FFS body file"), name + ".fbd", tr("FFS file body files (*.fbd *.bin);;All files (*)"));
            else if (subtype == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) path = QFileDialog::getSaveFileName(this, tr("Save section body to volume file"),                 name + ".vol", tr("Volume files (*.vol *.bin);;All files (*)"));
            else if (subtype == EFI_SECTION_RAW)                   path = QFileDialog::getSaveFileName(this, tr("Save section body to raw file"),                    name + ".raw", tr("Raw files (*.raw *.bin);;All files (*)"));
            else if (subtype == EFI_SECTION_PE32
                || subtype == EFI_SECTION_TE
                || subtype == EFI_SECTION_PIC)                     path = QFileDialog::getSaveFileName(this, tr("Save section body to EFI executable file"),         name + ".efi", tr("EFI executable files (*.efi *.bin);;All files (*)"));
            else                                                   path = QFileDialog::getSaveFileName(this, tr("Save section body to file"),                        name + ".bin", tr("Binary files (*.bin);;All files (*)"));
           break;
        case Types::NvarEntry:
        case Types::VssEntry:
        case Types::EvsaEntry:
        case Types::FlashMapEntry:
        case Types::FsysEntry:                       path = QFileDialog::getSaveFileName(this, tr("Save entry body to file"),       name + ".bin", tr("Binary files (*.bin);;All files (*)")); break;
        case Types::VssStore:
        case Types::Vss2Store:
        case Types::FtwStore:
        case Types::FdcStore:
        case Types::FsysStore:
        case Types::FlashMapStore:
        case Types::CmdbStore:                       path = QFileDialog::getSaveFileName(this, tr("Save store body to file"),       name + ".bin", tr("Binary files (*.bin);;All files (*)")); break;
        case Types::Microcode:                       path = QFileDialog::getSaveFileName(this, tr("Save microcode body to file"),   name + ".ucb", tr("Microcode body files (*.ucb *.bin);;All files (*)")); break;
        case Types::SlicData:
            if (subtype == Subtypes::PubkeySlicData) path = QFileDialog::getSaveFileName(this, tr("Save SLIC pubkey body to file"), name + ".spb", tr("SLIC pubkey body files (*.spb *.bin);;All files (*)"));
            else                                     path = QFileDialog::getSaveFileName(this, tr("Save SLIC marker body to file"), name + ".smb", tr("SLIC marker body files (*.smb *.bin);;All files (*)"));
            break;
        default:                                     path = QFileDialog::getSaveFileName(this, tr("Save object to file"),           name + ".bin", tr("Binary files (*.bin);;All files (*)"));
        }
    }
    else                                             path = QFileDialog::getSaveFileName(this, tr("Save object to file"),           name + ".bin", tr("Binary files (*.bin);;All files (*)"));

    if (path.trimmed().isEmpty())
        return;

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

void UEFITool::rebuild()
{

}

void UEFITool::remove()
{

}

void UEFITool::about()
{
    QMessageBox::about(this, tr("About UEFITool"), tr(
        "Copyright (c) 2019, Nikolaj Schlej.<br>"
        "Program icon made by <a href=https://www.behance.net/alzhidkov>Alexander Zhidkov</a>.<br>"
        "The program uses QHexEdit2 library made by <a href=https://github.com/Simsys/>Simsys</a>.<br>"
        "Qt-less engine is using Bstrlib made by <a href=https://github.com/websnarf/>Paul Hsieh</a>.<br><br>"
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

}

void UEFITool::openImageFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file"), currentDir, tr("BIOS image files (*.rom *.bin *.cap *scap *.bio *.fd *.wph *.dec);;All files (*)"));
    openImageFile(path);
}

void UEFITool::openImageFileInNewWindow()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file in new window"), currentDir, tr("BIOS image files (*.rom *.bin *.cap *scap *.bio *.fd *.wph *.dec);;All files (*)"));
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
    setWindowTitle(tr("UEFITool %1 - %2").arg(version).arg(fileInfo.fileName()));

    // Parse the image
    USTATUS result = ffsParser->parse(buffer);
    showParserMessages();
    if (result) {
        QMessageBox::critical(this, tr("Image parsing failed"), errorCodeToUString(result), QMessageBox::Ok);
        return;
    }
    else
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));

    // Enable or disable FIT tab
    showFitTable();

    // Enable or disable Security tab
    showSecurityInfo();

    // Enable search ...
    delete ffsFinder;
    ffsFinder = new FfsFinder(model);
    ui->actionSearch->setEnabled(true);
    // ... and other operations
    delete ffsOps;
    ffsOps = new FfsOperations(model);
    // ... and reports
    delete ffsReport;
    ffsReport = new FfsReport(model);

    // Enable goToBase and goToAddress
    ui->actionGoToBase->setEnabled(true);
    if (ffsParser->getAddressDiff() <= 0xFFFFFFFFUL)
        ui->actionGoToAddress->setEnabled(true);

    // Enable generateReport
    ui->actionGenerateReport->setEnabled(true);

    // Enable saving GUIDs
    ui->actionExportDiscoveredGuids->setEnabled(true);

    // Set current directory
    currentDir = fileInfo.absolutePath();

    // Set current path
    currentPath = path;
}

void UEFITool::enableMessagesCopyActions(QListWidgetItem* item)
{
    ui->menuMessageActions->setEnabled(item != NULL);
    ui->actionMessagesCopy->setEnabled(item != NULL);
    ui->actionMessagesCopyAll->setEnabled(item != NULL);
    ui->actionMessagesClear->setEnabled(item != NULL);
}

void UEFITool::copyMessage()
{
    clipboard->clear();
    if (ui->messagesTabWidget->currentIndex() == TAB_PARSER) // Parser tab
        clipboard->setText(ui->parserMessagesListWidget->currentItem()->text());
    else if (ui->messagesTabWidget->currentIndex() == TAB_SEARCH) // Search tab
        clipboard->setText(ui->finderMessagesListWidget->currentItem()->text());
    else if (ui->messagesTabWidget->currentIndex() == TAB_BUILDER) // Builder tab
        clipboard->setText(ui->builderMessagesListWidget->currentItem()->text());
}

void UEFITool::copyAllMessages()
{
    QString text;
    clipboard->clear();
    if (ui->messagesTabWidget->currentIndex() == TAB_PARSER) { // Parser tab
        for (INT32 i = 0; i < ui->parserMessagesListWidget->count(); i++)
            text.append(ui->parserMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
    else if (ui->messagesTabWidget->currentIndex() == TAB_SEARCH) {  // Search tab
        for (INT32 i = 0; i < ui->finderMessagesListWidget->count(); i++)
            text.append(ui->finderMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
    else if (ui->messagesTabWidget->currentIndex() == TAB_BUILDER) {  // Builder tab
        for (INT32 i = 0; i < ui->builderMessagesListWidget->count(); i++)
            text.append(ui->builderMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
}

void UEFITool::clearMessages()
{
    if (ui->messagesTabWidget->currentIndex() == TAB_PARSER) { // Parser tab
        if (ffsParser) ffsParser->clearMessages();
        ui->parserMessagesListWidget->clear();
    }
    else if (ui->messagesTabWidget->currentIndex() == TAB_SEARCH) {  // Search tab
        if (ffsFinder) ffsFinder->clearMessages();
        ui->finderMessagesListWidget->clear();
    }
    else if (ui->messagesTabWidget->currentIndex() == TAB_BUILDER) {  // Builder tab
        if (ffsBuilder) ffsBuilder->clearMessages();
        ui->builderMessagesListWidget->clear();
    }

    ui->menuMessageActions->setEnabled(false);
    ui->actionMessagesCopy->setEnabled(false);
    ui->actionMessagesCopyAll->setEnabled(false);
    ui->actionMessagesClear->setEnabled(false);
}

void UEFITool::toggleBootGuardMarking(bool enabled)
{
    model->setMarkingEnabled(enabled);
    markingEnabled = enabled;
}

/* emit double click signal of QListWidget on enter/return key pressed */
bool UEFITool::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);

        if (key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return) {
            QListWidget* list = qobject_cast<QListWidget*>(obj);

            if (list != NULL && list->currentItem() != NULL)
                emit list->itemDoubleClicked(list->currentItem());
        }
    }

    return QObject::eventFilter(obj, event);
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

void UEFITool::showParserMessages()
{
    ui->parserMessagesListWidget->clear();
    if (!ffsParser)
        return;

    std::vector<std::pair<QString, QModelIndex> > messages = ffsParser->getMessages();
    std::pair<QString, QModelIndex> msg;
    foreach (msg, messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));
        ui->parserMessagesListWidget->addItem(item);
    }

    ui->messagesTabWidget->setCurrentIndex(TAB_PARSER);
    ui->parserMessagesListWidget->scrollToBottom();
}

void UEFITool::showFinderMessages()
{
    ui->finderMessagesListWidget->clear();
    if (!ffsParser)
        return;

    std::vector<std::pair<QString, QModelIndex> > messages = ffsFinder->getMessages();
    std::pair<QString, QModelIndex> msg;
    foreach (msg, messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));;
        ui->finderMessagesListWidget->addItem(item);
    }

    ui->messagesTabWidget->setTabEnabled(TAB_SEARCH, true);
    ui->messagesTabWidget->setCurrentIndex(TAB_SEARCH);
    ui->finderMessagesListWidget->scrollToBottom();
}

void UEFITool::showBuilderMessages()
{
    ui->builderMessagesListWidget->clear();
    if (!ffsBuilder)
        return;

    std::vector<std::pair<QString, QModelIndex> > messages = ffsBuilder->getMessages();
    std::pair<QString, QModelIndex> msg;
    foreach (msg, messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));
        ui->builderMessagesListWidget->addItem(item);
    }

    ui->messagesTabWidget->setTabEnabled(TAB_BUILDER, true);
    ui->messagesTabWidget->setCurrentIndex(TAB_BUILDER);
    ui->builderMessagesListWidget->scrollToBottom();
}

void UEFITool::scrollTreeView(QListWidgetItem* item)
{
    QByteArray second = item->data(Qt::UserRole).toByteArray();
    QModelIndex *index = (QModelIndex *)second.data();
    if (index && index->isValid()) {
        ui->structureTreeView->scrollTo(*index, QAbstractItemView::PositionAtCenter);
        ui->structureTreeView->selectionModel()->select(*index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void UEFITool::scrollTreeView(QTableWidgetItem* item)
{
    QByteArray second = item->data(Qt::UserRole).toByteArray();
    QModelIndex *index = (QModelIndex *)second.data();
    if (index && index->isValid()) {
        ui->structureTreeView->scrollTo(*index, QAbstractItemView::PositionAtCenter);
        ui->structureTreeView->selectionModel()->select(*index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void UEFITool::contextMenuEvent(QContextMenuEvent* event)
{
    // The checks involving underMouse do not work well enough on macOS, and result in right-click sometimes
    // not showing any context menu at all. Most likely it is a bug in Qt, which does not affect other systems.
    // For this reason we reimplement this manually.
    if (ui->parserMessagesListWidget->rect().contains(ui->parserMessagesListWidget->mapFromGlobal(event->globalPos())) ||
        ui->finderMessagesListWidget->rect().contains(ui->finderMessagesListWidget->mapFromGlobal(event->globalPos())) ||
        ui->builderMessagesListWidget->rect().contains(ui->builderMessagesListWidget->mapFromGlobal(event->globalPos()))) {
        ui->menuMessageActions->exec(event->globalPos());
        return;
    }


    if (!ui->structureTreeView->rect().contains(ui->structureTreeView->mapFromGlobal(event->globalPos())))
        return;

    QPoint pt = event->pos();
    QModelIndex index = ui->structureTreeView->indexAt(ui->structureTreeView->viewport()->mapFrom(this, pt));
    if (!index.isValid()) {
        return;
    }

    switch (model->type(index))
    {
    case Types::Capsule:        ui->menuCapsuleActions->exec(event->globalPos());      break;
    case Types::Image:          ui->menuImageActions->exec(event->globalPos());        break;
    case Types::Region:         ui->menuRegionActions->exec(event->globalPos());       break;
    case Types::Padding:        ui->menuPaddingActions->exec(event->globalPos());      break;
    case Types::Volume:         ui->menuVolumeActions->exec(event->globalPos());       break;
    case Types::File:           ui->menuFileActions->exec(event->globalPos());         break;
    case Types::Section:        ui->menuSectionActions->exec(event->globalPos());      break;
    case Types::VssStore:
    case Types::Vss2Store:
    case Types::FdcStore:
    case Types::FsysStore:
    case Types::EvsaStore:
    case Types::FtwStore:
    case Types::FlashMapStore:
    case Types::CmdbStore:
    case Types::FptStore:
    case Types::CpdStore:
    case Types::BpdtStore:      ui->menuStoreActions->exec(event->globalPos());        break;
    case Types::FreeSpace:      break; // No menu needed for FreeSpace item
    default:                    ui->menuEntryActions->exec(event->globalPos());        break;
    }
}

void UEFITool::readSettings()
{
    QSettings settings(this);
    restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    restoreState(settings.value("mainWindow/windowState").toByteArray());
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
    markingEnabled = settings.value("tree/markingEnabled", true).toBool();
    ui->actionToggleBootGuardMarking->setChecked(markingEnabled);

    // Set monospace font for some controls
    QString fontName;
    int fontSize;
#if defined Q_OS_OSX
    fontName = settings.value("mainWindow/fontName", QString("Menlo")).toString();
    fontSize = settings.value("mainWindow/fontSize", 10).toInt();
#elif defined Q_OS_WIN
    fontName = settings.value("mainWindow/fontName", QString("Consolas")).toString();
    fontSize = settings.value("mainWindow/fontSize", 9).toInt();
#else
    fontName = settings.value("mainWindow/fontName", QString("Courier New")).toString();
    fontSize = settings.value("mainWindow/fontSize", 10).toInt();
#endif
    currentFont = QFont(fontName, fontSize);
    ui->infoEdit->setFont(currentFont);
    ui->parserMessagesListWidget->setFont(currentFont);
    ui->finderMessagesListWidget->setFont(currentFont);
    ui->builderMessagesListWidget->setFont(currentFont);
    ui->fitTableWidget->setFont(currentFont);
    ui->securityEdit->setFont(currentFont);
    ui->structureTreeView->setFont(currentFont);
    searchDialog->ui->guidEdit->setFont(currentFont);
    searchDialog->ui->hexEdit->setFont(currentFont);
    hexViewDialog->setFont(currentFont);
    goToAddressDialog->ui->hexSpinBox->setFont(currentFont);
    goToBaseDialog->ui->hexSpinBox->setFont(currentFont);
}

void UEFITool::writeSettings()
{
    QSettings settings(this);
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());
    settings.setValue("mainWindow/treeWidth", ui->structureGroupBox->width());
    settings.setValue("mainWindow/infoWidth", ui->infoGroupBox->width());
    settings.setValue("mainWindow/treeHeight", ui->structureGroupBox->height());
    settings.setValue("mainWindow/messageHeight", ui->messagesTabWidget->height());
    settings.setValue("tree/columnWidth0", ui->structureTreeView->columnWidth(0));
    settings.setValue("tree/columnWidth1", ui->structureTreeView->columnWidth(1));
    settings.setValue("tree/columnWidth2", ui->structureTreeView->columnWidth(2));
    settings.setValue("tree/columnWidth3", ui->structureTreeView->columnWidth(3));
    settings.setValue("tree/markingEnabled", markingEnabled);
    settings.setValue("mainWindow/fontName", currentFont.family());
    settings.setValue("mainWindow/fontSize", currentFont.pointSize());
}

void UEFITool::showFitTable()
{
    std::vector<std::pair<std::vector<UString>, UModelIndex> > fitTable = ffsParser->getFitTable();
    if (fitTable.empty()) {
        // Disable FIT tab
        ui->messagesTabWidget->setTabEnabled(TAB_FIT, false);
        return;
    }

    // Enable FIT tab
    ui->messagesTabWidget->setTabEnabled(TAB_FIT, true);

    // Set up the FIT table
    ui->fitTableWidget->clear();
    ui->fitTableWidget->setRowCount((int)fitTable.size());
    ui->fitTableWidget->setColumnCount(6);
    ui->fitTableWidget->setHorizontalHeaderLabels(QStringList() << tr("Address") << tr("Size") << tr("Version") << tr("Checksum") << tr("Type") << tr("Information"));
    ui->fitTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->fitTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fitTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fitTableWidget->horizontalHeader()->setStretchLastSection(true);

    // Add all data to the table widget
    for (size_t i = 0; i < fitTable.size(); i++) {
        for (UINT8 j = 0; j < 6; j++) {
            QTableWidgetItem* item = new QTableWidgetItem(fitTable[i].first[j]);
            item->setData(Qt::UserRole, QByteArray((const char*)&fitTable[i].second, sizeof(fitTable[i].second)));
            ui->fitTableWidget->setItem((int)i, j, item);
        }
    }

    ui->fitTableWidget->resizeColumnsToContents();
    ui->fitTableWidget->resizeRowsToContents();
    ui->messagesTabWidget->setCurrentIndex(TAB_FIT);
}

void UEFITool::showSecurityInfo()
{
    // Get security info
    UString secInfo = ffsParser->getSecurityInfo();
    if (secInfo.isEmpty()) {
        ui->messagesTabWidget->setTabEnabled(TAB_SECURITY, false);
        return;
    }

    ui->messagesTabWidget->setTabEnabled(TAB_SECURITY, true);
    ui->securityEdit->setPlainText(secInfo);
    ui->messagesTabWidget->setCurrentIndex(TAB_SECURITY);
}

void UEFITool::currentTabChanged(int index)
{
    U_UNUSED_PARAMETER(index);

    ui->menuMessageActions->setEnabled(false);
    ui->actionMessagesCopy->setEnabled(false);
    ui->actionMessagesCopyAll->setEnabled(false);
    ui->actionMessagesClear->setEnabled(false);
}

void UEFITool::loadGuidDatabase()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select GUID database file to load"), currentDir, tr("Comma-separated values files (*.csv);;All files (*)"));
    if (!path.isEmpty()) {
        initGuidDatabase(path);
        if (!currentPath.isEmpty() && QMessageBox::Yes == QMessageBox::information(this, tr("New GUID database loaded"), tr("Apply new GUID database on the opened file?\nUnsaved changes and tree position will be lost."), QMessageBox::Yes, QMessageBox::No))
            openImageFile(currentPath);
    }
}

void UEFITool::unloadGuidDatabase()
{
    initGuidDatabase();
    if (!currentPath.isEmpty() && QMessageBox::Yes == QMessageBox::information(this, tr("GUID database unloaded"), tr("Apply changes on the opened file?\nUnsaved changes and tree position will be lost."), QMessageBox::Yes, QMessageBox::No))
       openImageFile(currentPath);
}

void UEFITool::loadDefaultGuidDatabase()
{
    initGuidDatabase(":/guids.csv");
    if (!currentPath.isEmpty() && QMessageBox::Yes == QMessageBox::information(this, tr("Default GUID database loaded"), tr("Apply default GUID database on the opened file?\nUnsaved changes and tree position will be lost."), QMessageBox::Yes, QMessageBox::No))
        openImageFile(currentPath);
}

void UEFITool::exportDiscoveredGuids()
{
    GuidDatabase db = guidDatabaseFromTreeRecursive(model, model->index(0, 0));
    if (!db.empty()) {
        QString path = QFileDialog::getSaveFileName(this, tr("Save parsed GUIDs to database"), currentPath + ".guids.csv", tr("Comma-separated values files (*.csv);;All files (*)"));
        if (!path.isEmpty())
            guidDatabaseExportToFile(path, db);
    }
}

void UEFITool::generateReport()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save report to text file"), currentPath + ".report.txt", tr("Text files (*.txt);;All files (*)"));
    if (!path.isEmpty()) {
        std::vector<QString> report = ffsReport->generate();
        if (report.size()) {
            QFile file;
            file.setFileName(path);
            if (file.open(QFile::Text | QFile::WriteOnly)) {
                for (size_t i = 0; i < report.size(); i++) {
                    file.write(report[i].toLatin1().append('\n'));
                }
                file.close();
            }
        }
    }
}
