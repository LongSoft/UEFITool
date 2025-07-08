/* uefitool.cpp
 
 Copyright (c) 2022, Nikolaj Schlej. All rights reserved.
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

#include "../common/zlib/zlib.h"
#include "../common/digest/sha1.h"
#include "../common/digest/sha2.h"
#include "../common/digest/sm3.h"

#if QT_VERSION_MAJOR >= 6
#include <QStyleHints>
#endif
#include <QProxyStyle>

class DockProxyStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
        QPainter* painter, const QWidget* widget) const override
    {
        if (widget && (element == QStyle::PE_IndicatorDockWidgetResizeHandle
            || element == QStyle::PE_FrameDockWidget))
        {
            // "drawing" invisible elements
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

UEFITool::UEFITool(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::UEFITool),
version(tr(PROGRAM_VERSION)),
markingEnabled(true)
{
    clipboard = QApplication::clipboard();

    // Create UI
    ui->setupUi(this);
    setStyle(new DockProxyStyle(style()));
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    ui->hexViewWidgetContents->layout()->addWidget(&selectedHexView);
    dockTimer.setSingleShot(true);
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
    connect(ui->actionUncompressedHexView, SIGNAL(triggered()), this, SLOT(uncompressedHexView()));
    connect(ui->actionExtract, SIGNAL(triggered()), this, SLOT(extractAsIs()));
    connect(ui->actionExtractBody, SIGNAL(triggered()), this, SLOT(extractBody()));
    connect(ui->actionExtractUncompressed, SIGNAL(triggered()), this, SLOT(extractUncompressed()));
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
    connect(ui->actionCopyItemName, SIGNAL(triggered()), this, SLOT(copyItemName()));
    connect(ui->actionExpandItemRecursively, SIGNAL(triggered()), this, SLOT(expandItemRecursively()));
    connect(ui->actionCollapseItemRecursively, SIGNAL(triggered()), this, SLOT(collapseItemRecursively()));
    connect(ui->actionClearRecentlyOpenedFilesList, SIGNAL(triggered()), this, SLOT(clearRecentlyOpenedFilesList()));
    connect(ui->actionHashCrc32, SIGNAL(triggered()), this, SLOT(hashCrc32()));
    connect(ui->actionHashSha1, SIGNAL(triggered()), this, SLOT(hashSha1()));
    connect(ui->actionHashSha256, SIGNAL(triggered()), this, SLOT(hashSha256()));
    connect(ui->actionHashSha384, SIGNAL(triggered()), this, SLOT(hashSha384()));
    connect(ui->actionHashSha512, SIGNAL(triggered()), this, SLOT(hashSha512()));
    connect(ui->actionHashSm3, SIGNAL(triggered()), this, SLOT(hashSm3()));
    connect(ui->actionBodyHashCrc32, SIGNAL(triggered()), this, SLOT(hashBodyCrc32()));
    connect(ui->actionBodyHashSha1, SIGNAL(triggered()), this, SLOT(hashBodySha1()));
    connect(ui->actionBodyHashSha256, SIGNAL(triggered()), this, SLOT(hashBodySha256()));
    connect(ui->actionBodyHashSha384, SIGNAL(triggered()), this, SLOT(hashBodySha384()));
    connect(ui->actionBodyHashSha512, SIGNAL(triggered()), this, SLOT(hashBodySha512()));
    connect(ui->actionBodyHashSm3, SIGNAL(triggered()), this, SLOT(hashBodySm3()));
    connect(ui->actionUncompressedHashCrc32, SIGNAL(triggered()), this, SLOT(hashUncompressedCrc32()));
    connect(ui->actionUncompressedHashSha1, SIGNAL(triggered()), this, SLOT(hashUncompressedSha1()));
    connect(ui->actionUncompressedHashSha256, SIGNAL(triggered()), this, SLOT(hashUncompressedSha256()));
    connect(ui->actionUncompressedHashSha384, SIGNAL(triggered()), this, SLOT(hashUncompressedSha384()));
    connect(ui->actionUncompressedHashSha512, SIGNAL(triggered()), this, SLOT(hashUncompressedSha512()));
    connect(ui->actionUncompressedHashSm3, SIGNAL(triggered()), this, SLOT(hashUncompressedSm3()));
    for (auto dock : findChildren<QDockWidget*>()) {
        connect(dock, SIGNAL(topLevelChanged(bool)), this, SLOT(onDockStateChange(bool)));
        connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(onDockStateChange(bool)));
    }
    connect(&dockTimer, SIGNAL(timeout()), this, SLOT(checkAndUpdateDocks()));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(writeSettings()));
    
    // Enable Drag-and-Drop actions
    setAcceptDrops(true);
    
    // Disable Builder tab, doesn't work right now
    enableDock(ui->builderMessagesDock, false);
    
    // Set current directory
    currentDir = ".";
    
    // Load built-in GUID database
    initGuidDatabase(":/guids.csv");
    
    // Initialize non-persistent data
    init();
    
    // Read stored settings
    readSettings();

    // Update recent files list in menu
    updateRecentFilesMenu();
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
    delete goToAddressDialog;
    delete goToBaseDialog;
    setStatusBar(nullptr);          // workaround for a bug related to the use of addDockWidget() in GUI application (or in Qt internally)
    ui->statusBar->deleteLater();   // with QDockWidget on macOS, causing exception EXC_BAD_ACCESS right after (or somewhere in) QMainWindow destructor
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
    contextEventWidget = nullptr;
    bool wayland = QGuiApplication::platformName().contains("wayland", Qt::CaseInsensitive);
    for (auto dock : findChildren<QDockWidget*>()) {
        enableDock(dock, false);
        // floating QDockWidgets are defective in Wayland
        if (wayland)
            dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    }
    
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
    ui->menuHashActions->setEnabled(false);
    ui->menuHashBodyActions->setEnabled(false);
    ui->menuHashUncompressedActions->setEnabled(false);
    
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
    
    // Connect signals to slots
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
    
    // Allow enter/return pressing to scroll tree view
    ui->parserMessagesListWidget->installEventFilter(this);
    ui->finderMessagesListWidget->installEventFilter(this);
    ui->builderMessagesListWidget->installEventFilter(this);

    // Detect and set UI light or dark mode
#if QT_VERSION_MAJOR >= 6
#if QT_VERSION_MINOR < 5
#if defined Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    if (settings.value("AppsUseLightTheme", 1).toInt() == 0) {
        model->setMarkingDarkMode(true);
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        QApplication::setPalette(QApplication::style()->standardPalette());
    }
#else
    const QPalette palette = QApplication::palette();
    const QColor& color = palette.color(QPalette::Active, QPalette::Base);
    if (color.lightness() < 127) { // TreeView has dark background
        model->setMarkingDarkMode(true);
    }
#endif // defined Q_OS_WIN
#else // QT_VERSION_MINOR >= 5
    // Qt 6.5.0 added proper support for dark UI mode, including detection and notification on mode change
    // It also supposed to work in all OSes, but still requires changing the default style on Windows from Vista to Fusion
    auto styleHints = QGuiApplication::styleHints();
    model->setMarkingDarkMode(styleHints->colorScheme() == Qt::ColorScheme::Dark);
    connect(styleHints, SIGNAL(colorSchemeChanged(Qt::ColorScheme)), this, SLOT(updateUiForNewColorScheme(Qt::ColorScheme)));

#if defined Q_OS_WIN
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setPalette(QApplication::style()->standardPalette());
#endif
#endif // QT_VERSION_MINOR
#endif // QT_VERSION_MAJOR
}

#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 5
void UEFITool::updateUiForNewColorScheme(Qt::ColorScheme scheme)
{
    model->setMarkingDarkMode(scheme == Qt::ColorScheme::Dark);
    QApplication::setPalette(QApplication::style()->standardPalette());

    QModelIndex current = ui->structureTreeView->selectionModel()->currentIndex();
    selectedHexView.setBackground(0, model->header(current).size(),
        model->markingDarkMode() ? Qt::darkGreen : Qt::green);
}
#endif

void UEFITool::updateRecentFilesMenu(const QString& fileName)
{
    // Update list
    if (!fileName.isEmpty()) {
        recentFiles.removeAll(fileName);
        recentFiles.removeAll(QDir::toNativeSeparators(fileName));
        recentFiles.prepend(fileName);
        while (recentFiles.size() > 21) {
            recentFiles.removeLast();
        }
    }

    // Delete old actions
    for (QAction* action : recentFileActions) {
        ui->menuFile->removeAction(action);
        delete action;
    }
    recentFileActions.clear();

    if (!recentFiles.isEmpty()) {
        int key = 0;

        // Enable "Clear recently opened files list" action
        ui->actionClearRecentlyOpenedFilesList->setEnabled(true);
        
        // Insert new actions before "Clear recently opened files list"
        for (const QString& path : recentFiles) {
            QAction* action = new QAction(QDir::toNativeSeparators(path), this);
            if (++key < 10)
                action->setShortcut(QKeySequence(Qt::ALT | (Qt::Key_0 + key)));
            else if (key == 10)
                action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_0));

            connect(action, SIGNAL(triggered()), this, SLOT(openRecentImageFile()));
            action->setData(path);
            ui->menuFile->insertAction(ui->actionClearRecentlyOpenedFilesList, action);
            recentFileActions.append(action);
        }
        // Finally, insert a separator after the list and before "Clear recently opened files list" action
        recentFileActions.append(ui->menuFile->insertSeparator(ui->actionClearRecentlyOpenedFilesList));
    }
    else {
        // Disable "Clear recently opened files list" action
        ui->actionClearRecentlyOpenedFilesList->setEnabled(false);
    }
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
    enableDock(ui->infoDock, true);

    // Set Hex view
    selectedHexView.clearMetadata();
    selectedHexView.setBackground(0, model->header(current).size(),
        model->markingDarkMode() ? Qt::darkGreen : Qt::green);
    selectedHexView.setData(model->header(current) + model->body(current) + model->tail(current));
    enableDock(ui->hexViewDock, true);
    
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
                                     || type == Types::SysFEntry
                                     || type == Types::EvsaEntry
                                     || type == Types::PhoenixFlashMapEntry
                                     || type == Types::InsydeFlashDeviceMapEntry
                                     || type == Types::DellDvarEntry
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
                                     || type == Types::StartupApDataEntry
                                     );
    ui->menuStoreActions->setEnabled(type == Types::VssStore
                                     || type == Types::Vss2Store
                                     || type == Types::FdcStore
                                     || type == Types::SysFStore
                                     || type == Types::EvsaStore
                                     || type == Types::FtwStore
                                     || type == Types::PhoenixFlashMapStore
                                     || type == Types::InsydeFlashDeviceMapStore
                                     || type == Types::DellDvarStore
                                     || type == Types::NvarGuidStore
                                     || type == Types::CmdbStore
                                     || type == Types::FptStore
                                     || type == Types::BpdtStore
                                     || type == Types::CpdStore
                                     );
    
    bool empty = model->hasEmptyHeader(current) && model->hasEmptyBody(current) && model->hasEmptyTail(current);
    ui->menuHashActions->setDisabled(empty);
    ui->menuHashBodyActions->setDisabled(model->hasEmptyBody(current));
    ui->menuHashUncompressedActions->setDisabled(model->hasEmptyUncompressedData(current));
    
    // Enable actions
    ui->actionHexView->setDisabled(empty);
    ui->actionBodyHexView->setDisabled(model->hasEmptyBody(current));
    ui->actionUncompressedHexView->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionExtract->setDisabled(empty);
    ui->actionGoToData->setEnabled(type == Types::NvarEntry && subtype == Subtypes::LinkNvarEntry);
    ui->actionCopyItemName->setDisabled(model->name(current).isEmpty());
    ui->actionExpandItemRecursively->setEnabled(model->rowCount(current) > 0);
    ui->actionCollapseItemRecursively->setEnabled(model->rowCount(current) > 0);
    ui->actionHashCrc32->setDisabled(empty);
    ui->actionHashSha1->setDisabled(empty);
    ui->actionHashSha256->setDisabled(empty);
    ui->actionHashSha384->setDisabled(empty);
    ui->actionHashSha512->setDisabled(empty);
    ui->actionHashSm3->setDisabled(empty);
    ui->actionBodyHashCrc32->setDisabled(model->hasEmptyBody(current));
    ui->actionBodyHashSha1->setDisabled(model->hasEmptyBody(current));
    ui->actionBodyHashSha256->setDisabled(model->hasEmptyBody(current));
    ui->actionBodyHashSha384->setDisabled(model->hasEmptyBody(current));
    ui->actionBodyHashSha512->setDisabled(model->hasEmptyBody(current));
    ui->actionBodyHashSm3->setDisabled(model->hasEmptyBody(current));
    ui->actionUncompressedHashCrc32->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionUncompressedHashSha1->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionUncompressedHashSha256->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionUncompressedHashSha384->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionUncompressedHashSha512->setDisabled(model->hasEmptyUncompressedData(current));
    ui->actionUncompressedHashSm3->setDisabled(model->hasEmptyUncompressedData(current));
    
    // Disable rebuild for now
    //ui->actionRebuild->setDisabled(type == Types::Region && subtype == Subtypes::DescriptorRegion);
    //ui->actionReplace->setDisabled(type == Types::Region && subtype == Subtypes::DescriptorRegion);
    
    //ui->actionRebuild->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
    ui->actionExtractBody->setDisabled(model->hasEmptyBody(current));
    ui->actionExtractUncompressed->setDisabled(model->hasEmptyUncompressedData(current));
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

void UEFITool::search()
{
    QSettings settings(this);
    searchDialog->restoreGeometry(settings.value("searchDialog/geometry").toByteArray());
    searchDialog->ui->tabWidget->setCurrentIndex(settings.value("searchDialog/currentScopeMode").toInt());
    UINT8 mode = settings.value("searchDialog/hexScopeMode", SEARCH_MODE_ALL).toUInt();
    searchDialog->ui->hexScopeHeaderRadioButton->setChecked(mode <= SEARCH_MODE_HEADER);
    searchDialog->ui->hexScopeBodyRadioButton->setChecked(mode == SEARCH_MODE_BODY);
    searchDialog->ui->hexScopeFullRadioButton->setChecked(mode >= SEARCH_MODE_ALL);
    mode = settings.value("searchDialog/guidScopeMode", SEARCH_MODE_HEADER).toUInt();
    searchDialog->ui->guidScopeHeaderRadioButton->setChecked(mode <= SEARCH_MODE_HEADER);
    searchDialog->ui->guidScopeBodyRadioButton->setChecked(mode == SEARCH_MODE_BODY);
    searchDialog->ui->guidScopeFullRadioButton->setChecked(mode >= SEARCH_MODE_ALL);
    mode = settings.value("searchDialog/textScopeMode", SEARCH_MODE_ALL).toUInt();
    searchDialog->ui->textScopeHeaderRadioButton->setChecked(mode <= SEARCH_MODE_HEADER);
    searchDialog->ui->textScopeBodyRadioButton->setChecked(mode == SEARCH_MODE_BODY);
    searchDialog->ui->textScopeFullRadioButton->setChecked(mode >= SEARCH_MODE_ALL);
    searchDialog->ui->textUnicodeCheckBox->setChecked(settings.value("searchDialog/textUnicode", true).toBool());
    searchDialog->ui->textCaseSensitiveCheckBox->setChecked(settings.value("searchDialog/textCaseSensitive", false).toBool());

    if (searchDialog->exec() != QDialog::Accepted)
        return;
    
    settings.setValue("searchDialog/geometry", searchDialog->saveGeometry());
    settings.setValue("searchDialog/currentScopeMode", searchDialog->ui->tabWidget->currentIndex());
    if (searchDialog->ui->hexScopeHeaderRadioButton->isChecked())
        mode = SEARCH_MODE_HEADER;
    else if (searchDialog->ui->hexScopeBodyRadioButton->isChecked())
        mode = SEARCH_MODE_BODY;
    else
        mode = SEARCH_MODE_ALL;
    settings.setValue("searchDialog/hexScopeMode", mode);
    if (searchDialog->ui->guidScopeHeaderRadioButton->isChecked())
        mode = SEARCH_MODE_HEADER;
    else if (searchDialog->ui->guidScopeBodyRadioButton->isChecked())
        mode = SEARCH_MODE_BODY;
    else
        mode = SEARCH_MODE_ALL;
    settings.setValue("searchDialog/guidScopeMode", mode);
    if (searchDialog->ui->textScopeHeaderRadioButton->isChecked())
        mode = SEARCH_MODE_HEADER;
    else if (searchDialog->ui->textScopeBodyRadioButton->isChecked())
        mode = SEARCH_MODE_BODY;
    else
        mode = SEARCH_MODE_ALL;
    settings.setValue("searchDialog/textScopeMode", mode);
    settings.setValue("searchDialog/textUnicode", searchDialog->ui->textUnicodeCheckBox->isChecked());
    settings.setValue("searchDialog/textCaseSensitive", searchDialog->ui->textCaseSensitiveCheckBox->isChecked());

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
        ffsFinder->findHexPattern(pattern, mode);
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
        ffsFinder->findGuidPattern(pattern, mode);
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
        ffsFinder->findTextPattern(pattern, mode, searchDialog->ui->textUnicodeCheckBox->isChecked(),
                                   (Qt::CaseSensitivity) searchDialog->ui->textCaseSensitiveCheckBox->isChecked());
        showFinderMessages();
    }
}

void UEFITool::hexView()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    hexViewDialog->setItem(index, HexViewDialog::HexViewType::fullHexView);
    hexViewDialog->exec();
}

void UEFITool::bodyHexView()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    hexViewDialog->setItem(index, HexViewDialog::HexViewType::bodyHexView);
    hexViewDialog->exec();
}

void UEFITool::uncompressedHexView()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    hexViewDialog->setItem(index, HexViewDialog::HexViewType::uncompressedHexView);
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
        UINT32 offset = model->offset(index);
        if (pdata->next == 0xFFFFFF) {
            ui->structureTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            ui->structureTreeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
        }
        
        for (int j = i + 1; j < model->rowCount(parent); j++) {
            QModelIndex currentIndex = parent.model()->index(j, 0, parent);
            
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

void UEFITool::extractUncompressed()
{
    extract(EXTRACT_MODE_UNCOMPRESSED);
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
    
    name = QDir::toNativeSeparators(extractDir + QDir::separator() + name);
    
    //ui->statusBar->showMessage(name);
    
    UINT8 type = model->type(index);
    UINT8 subtype = model->subtype(index);
    QString path;
    if (mode == EXTRACT_MODE_AS_IS) {
        switch (type) {
            case Types::Capsule: path = QFileDialog::getSaveFileName(this, tr("Save capsule to file"), name + ".cap",  tr("Capsule files (*.cap *.bin);;All files (*)")); break;
            case Types::Image:   path = QFileDialog::getSaveFileName(this, tr("Save image to file"), name + ".rom",  tr("Image files (*.rom *.bin);;All files (*)")); break;
            case Types::Region:  path = QFileDialog::getSaveFileName(this, tr("Save region to file"), name + ".rgn",  tr("Region files (*.rgn *.bin);;All files (*)")); break;
            case Types::Padding: path = QFileDialog::getSaveFileName(this, tr("Save padding to file"), name + ".pad",  tr("Padding files (*.pad *.bin);;All files (*)")); break;
            case Types::Volume:  path = QFileDialog::getSaveFileName(this, tr("Save volume to file"), name + ".vol",  tr("Volume files (*.vol *.bin);;All files (*)")); break;
            case Types::File:    path = QFileDialog::getSaveFileName(this, tr("Save FFS file to file"), name + ".ffs",  tr("FFS files (*.ffs *.bin);;All files (*)")); break;
            case Types::Section: path = QFileDialog::getSaveFileName(this, tr("Save section to file"), name + ".sct",  tr("Section files (*.sct *.bin);;All files (*)")); break;
            default:             path = QFileDialog::getSaveFileName(this, tr("Save object to file"), name + ".bin", tr("Binary files (*.bin);;All files (*)"));
        }
    }
    else if (mode == EXTRACT_MODE_BODY) {
        switch (type) {
            case Types::Capsule: path = QFileDialog::getSaveFileName(this, tr("Save capsule body to image file"), name + ".rom", tr("Image files (*.rom *.bin);;All files (*)")); break;
            case Types::Volume:  path = QFileDialog::getSaveFileName(this, tr("Save volume body to file"), name + ".vbd", tr("Volume body files (*.vbd *.bin);;All files (*)")); break;
            case Types::File:    path = QFileDialog::getSaveFileName(this, tr("Save FFS file body to file"), name + ".fbd", tr("FFS file body files (*.fbd *.bin);;All files (*)")); break;
            case Types::Section:
                if (subtype == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
                    path = QFileDialog::getSaveFileName(this, tr("Save section body to volume file"), name + ".vol", tr("Volume files (*.vol *.bin);;All files (*)")); break;
                }
                else if (subtype == EFI_SECTION_PE32
                         || subtype == EFI_SECTION_TE
                         || subtype == EFI_SECTION_PIC) {
                    path = QFileDialog::getSaveFileName(this, tr("Save section body to EFI executable file"), name + ".efi", tr("EFI executable files (*.efi *.bin);;All files (*)")); break;
                }
            default: path = QFileDialog::getSaveFileName(this, tr("Save object body to file"), name + ".bin", tr("Binary files (*.bin);;All files (*)"));
        }
    }
    else path = QFileDialog::getSaveFileName(this, tr("Save object to file"), name + ".bin", tr("Binary files (*.bin);;All files (*)"));
    
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

    extractDir = QFileInfo(path).absolutePath();
}

void UEFITool::rebuild()
{
    
}

void UEFITool::remove()
{
    
}

void UEFITool::about()
{
    QMessageBox::about(this,
                       tr("About UEFITool"),
                       tr("<b>UEFITool %1.</b><br><br>"
                          "Copyright (c) 2013-2025, Nikolaj (<b>CodeRush</b>) Schlej, Vitaly (<b>vit9696</b>) Cheptsov, <a href=https://github.com/LongSoft/UEFITool/graphs/contributors>et al</a>.<br><br>"
                          "Program icon made by <a href=https://www.behance.net/alzhidkov>Alexander Zhidkov</a>.<br><br>"
                          "GUI uses QHexView made by <a href=https://github.com/Dax89>Antonio Davide</a>.<br>"
                          "Qt-less engine uses Bstrlib made by <a href=https://github.com/websnarf>Paul Hsieh</a>.<br>"
                          "Engine uses Tiano compression code made by <a href=https://github.com/tianocore>TianoCore developers</a>.<br>"
                          "Engine uses LZMA compression code made by <a href=https://www.7-zip.org/sdk.html>Igor Pavlov</a>.<br>"
                          "Engine uses zlib compression code made by <a href=https://github.com/madler>Mark Adler</a>.<br>"
                          "Engine uses LibTomCrypt hashing code made by <a href=https://github.com/libtom>LibTom developers</a>.<br>"
                          "Engine uses KaitaiStruct runtime made by <a href=https://github.com/kaitai-io>Kaitai team</a>.<br><br>"
                          "The program is dedicated to <b>RevoGirl</b>. Rest in peace, young genius.<br><br>"
                          "The program and the accompanying materials are licensed and made available under the terms and conditions of the BSD-2-Clause License.<br>"
                          "The full text of the license may be found at <a href=https://opensource.org/licenses/BSD-2-Clause>OpenSource.org</a>.<br><br>"
                          "<b>THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS, "
                          "WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, "
                          "EITHER EXPRESS OR IMPLIED.</b>"
                          "").arg(version)
                       );
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

void UEFITool::onDockStateChange(const bool topLevel)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (dock)
        updateDock(dock);
}

void UEFITool::updateDock(QDockWidget* const dock)
{
    if (!dock || dock->isHidden())
        return;
    if (!dock->widget() || !dock->widget()->layout())
        return;

    QWidget *widget = dock->titleBarWidget();
    QMargins margins = dock->widget()->layout()->contentsMargins();
    int ref = margins.left();
    margins.setTop(ref);

    if (widget) {
        dock->setTitleBarWidget(nullptr);
        delete widget;
    }

    // Floating? Using built-in title
    if (dock->isFloating()) {
        dock->widget()->layout()->setContentsMargins(margins);
        return;
    }

    widget = new QWidget();
    auto layout = new QHBoxLayout(widget);
    dock->setTitleBarWidget(widget);
    QString titleText = dock->windowTitle();

    // Tabified? Using blank title
    if (!tabifiedDockWidgets(dock).isEmpty()) {
        for (auto tabBar : findChildren<QTabBar*>()) {
            for (int i = tabBar->count() - 1; i >= 0; i--) {
                // Hope all docks have different titles
                if (titleText == tabBar->tabText(i)) {
                    layout->setContentsMargins(0, ref, 0, 0);
                    dock->widget()->layout()->setContentsMargins(ref, 0, ref, ref);
                    QPalette palette = QApplication::palette();
                    tabBar->setTabTextColor(i, !dock->isEnabled()
                        ? palette.color(QPalette::Disabled, QPalette::WindowText)
                            : dock->isVisible()
                                ? palette.color(QPalette::Active, QPalette::WindowText)
                                    : palette.color(QPalette::Inactive, QPalette::WindowText));
                    return;
                }
            }
        }
    }

    // Docked? Setup own title with text
    layout->setContentsMargins(ref, ref / 2, ref, 0);
    auto titleLabel = new QLabel(titleText);
    layout->addWidget(titleLabel);
    dock->widget()->layout()->setContentsMargins(ref, ref / 2, ref, ref);

}

bool UEFITool::checkDock(QDockWidget* const dock)
{
    if (!dock || dock->isHidden())
        return true;
    if (!dock->widget() || !dock->widget()->layout())
        return true;

    QWidget* widget = dock->titleBarWidget();
    // floating dock - no title widget
    if (dock->isFloating())
        return widget ? false : true;
    // tabified dock - title widget with blank layout
    if (!widget || !widget->layout())
        return false;
    if (!tabifiedDockWidgets(dock).isEmpty())
        return true;
    // sticked dock - title widget with layout with text widget(s)
    return widget->layout()->findChildren<QLabel*>().isEmpty() ? false : true;
}

void UEFITool::enableDock(QDockWidget* const dock, const bool enable)
{
    if (!dock)
        return;

    dock->setEnabled(enable);
    updateDock(dock);
}

void UEFITool::checkAndUpdateDocks()
{
    for (auto dock : findChildren<QDockWidget*>()) {
        if (!checkDock(dock))
            updateDock(dock);
    }
}

void UEFITool::resetDocks()
{
    selectedHexView.setReadOnly(true);

    addDockWidget(Qt::LeftDockWidgetArea, ui->structureTreeDock);
    addDockWidget(Qt::RightDockWidgetArea, ui->infoDock);
    addDockWidget(Qt::BottomDockWidgetArea, ui->parserMessagesDock);
    tabifyDockWidget(ui->parserMessagesDock, ui->fitDock);
    tabifyDockWidget(ui->fitDock, ui->securityDock);
    tabifyDockWidget(ui->securityDock, ui->finderMessagesDock);
    tabifyDockWidget(ui->finderMessagesDock, ui->builderMessagesDock);
    ui->parserMessagesDock->raise();
    tabifyDockWidget(ui->infoDock, ui->hexViewDock);
    ui->infoDock->raise();

    QSize mainSize = size();
    int totalWidth = mainSize.width();
    int leftWidth = totalWidth * 2 / 3;
    resizeDocks({ ui->structureTreeDock, ui->infoDock },
        { leftWidth, totalWidth - leftWidth }, Qt::Horizontal);

    int totalHeight = mainSize.height();
    int topHeight = totalHeight * 4 / 5;
    resizeDocks({ ui->structureTreeDock, ui->parserMessagesDock },
        { topHeight, totalHeight - topHeight }, Qt::Vertical);

    QMargins margins = ui->structureTreeWidgetContents->layout()->contentsMargins();

    for (auto dock : findChildren<QDockWidget*>()) {
        dock->setContentsMargins(0, 0, 0, 0);
        dock->layout()->setContentsMargins(0, 0, 0, 0);
        dock->widget()->setContentsMargins(0, 0, 0, 0);
        dock->widget()->layout()->setContentsMargins(margins);
        dock->setWindowFlags(dock->windowFlags() | Qt::WindowTitleHint);
        updateDock(dock);
    }
}

void UEFITool::openImageFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file"), openImageDir, tr("BIOS image files (*.rom *.bin *.cap *.scap *.bio *.fd *.wph *.dec);;All files (*)"));
    openImageFile(path);
}

void UEFITool::openImageFileInNewWindow()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open BIOS image file in new window"), openImageDir, tr("BIOS image files (*.rom *.bin *.cap *.scap *.bio *.fd *.wph *.dec);;All files (*)"));
    if (path.trimmed().isEmpty())
        return;
    QProcess::startDetached(currentProgramPath, QStringList(path));
}

void UEFITool::openRecentImageFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString fileName = action->data().toString();
        if (!fileName.isEmpty()) {
            openImageFile(fileName);
        }
    }
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
    else {
        ui->statusBar->showMessage(tr("Opened: %1").arg(fileInfo.fileName()));
    }
    ffsParser->outputInfo();
    enableDock(ui->structureTreeDock, true);
    
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
    openImageDir = currentDir;

    // Set current path
    currentPath = path;

    // Update menu
    updateRecentFilesMenu(currentPath);

    QModelIndex root = model->index(0, 0, QModelIndex());
    ui->structureTreeView->selectionModel()->select(root, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
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

    if (contextEventWidget == ui->parserMessagesListWidget) // Parser tab
        clipboard->setText(ui->parserMessagesListWidget->currentItem()->text());
    else if (contextEventWidget == ui->finderMessagesListWidget) // Search tab
        clipboard->setText(ui->finderMessagesListWidget->currentItem()->text());
    else if (contextEventWidget == ui->builderMessagesListWidget) // Builder tab
        clipboard->setText(ui->builderMessagesListWidget->currentItem()->text());
}

void UEFITool::copyAllMessages()
{
    QString text;
    clipboard->clear();

    if (contextEventWidget == ui->parserMessagesListWidget) { // Parser tab
        for (INT32 i = 0; i < ui->parserMessagesListWidget->count(); i++)
            text.append(ui->parserMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
    else if (contextEventWidget == ui->finderMessagesListWidget) {  // Search tab
        for (INT32 i = 0; i < ui->finderMessagesListWidget->count(); i++)
            text.append(ui->finderMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
    else if (contextEventWidget == ui->builderMessagesListWidget) {  // Builder tab
        for (INT32 i = 0; i < ui->builderMessagesListWidget->count(); i++)
            text.append(ui->builderMessagesListWidget->item(i)->text()).append("\n");
        clipboard->setText(text);
    }
}

void UEFITool::clearMessages()
{
    if (contextEventWidget == ui->parserMessagesListWidget) { // Parser tab
        if (ffsParser) ffsParser->clearMessages();
        ui->parserMessagesListWidget->clear();
    }
    else if (contextEventWidget == ui->finderMessagesListWidget) {  // Search tab
        if (ffsFinder) ffsFinder->clearMessages();
        ui->finderMessagesListWidget->clear();
    }
    else if (contextEventWidget == ui->builderMessagesListWidget) {  // Builder tab
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

// Emit double click signal of QListWidget on enter/return key pressed
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
    
    for (const auto &msg : messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));
        ui->parserMessagesListWidget->addItem(item);
    }
        
    enableDock(ui->parserMessagesDock, true);
    ui->parserMessagesDock->raise();
    ui->parserMessagesListWidget->scrollToBottom();
}

void UEFITool::showFinderMessages()
{
    ui->finderMessagesListWidget->clear();
    if (!ffsParser)
        return;
    
    std::vector<std::pair<QString, QModelIndex> > messages = ffsFinder->getMessages();
    
    for (const auto &msg : messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));;
        ui->finderMessagesListWidget->addItem(item);
    }
    
    enableDock(ui->finderMessagesDock, true);
    ui->finderMessagesDock->raise();
    ui->finderMessagesListWidget->scrollToBottom();
}

void UEFITool::showBuilderMessages()
{
    ui->builderMessagesListWidget->clear();
    if (!ffsBuilder)
        return;
    
    std::vector<std::pair<QString, QModelIndex> > messages = ffsBuilder->getMessages();
    
    for (const auto &msg : messages) {
        QListWidgetItem* item = new QListWidgetItem(msg.first, NULL, 0);
        item->setData(Qt::UserRole, QByteArray((const char*)&msg.second, sizeof(msg.second)));
        ui->builderMessagesListWidget->addItem(item);
    }
    
    enableDock(ui->builderMessagesDock, true);
    ui->builderMessagesDock->raise();
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
    if (!item)
        return;

    QByteArray second = item->data(Qt::UserRole).toByteArray();
    QModelIndex *index = (QModelIndex *)second.data();
    if (index && index->isValid()) {
        ui->structureTreeView->scrollTo(*index, QAbstractItemView::PositionAtCenter);
        ui->structureTreeView->selectionModel()->select(*index, QItemSelectionModel::Select | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
    }
}

void UEFITool::contextMenuEvent(QContextMenuEvent* event)
{
    if (!event)
        return;

    QPoint gp = event->globalPos();
    for (QListWidget* list : { ui->parserMessagesListWidget, ui->finderMessagesListWidget, ui->builderMessagesListWidget}) {
        // The checks involving underMouse do not work well enough on macOS, and result in right-click sometimes
        // not showing any context menu at all. Most likely it is a bug in Qt, which does not affect other systems.
        // For this reason we reimplement this manually.
        if (list->rect().contains(list->mapFromGlobal(gp))) {
            contextEventWidget = list;
            QListWidgetItem* item = list->itemAt(list->mapFromGlobal(gp));
            if (item)
                enableMessagesCopyActions(item);
            ui->menuMessageActions->exec(gp);
            contextEventWidget = nullptr;
            break;
        }
    }
    
    QPoint pt = event->pos();
    if (!ui->structureTreeView->rect().contains(ui->structureTreeView->mapFromGlobal(gp))) {
        QWidget* widget = childAt(pt);
        while (widget) {
            if (qobject_cast<QDockWidget*>(widget))
                return;
            widget = widget->parentWidget();
        }
        QMenu* menu = this->createPopupMenu();
        if (menu) {
            menu->exec(gp);
            menu->deleteLater();
        }
    }
    
    QModelIndex index = ui->structureTreeView->indexAt(ui->structureTreeView->viewport()->mapFrom(this, pt));
    if (!index.isValid()) {
        return;
    }
    
    QMenu* menu = nullptr;
    switch (model->type(index)) {
        case Types::Capsule:        menu = ui->menuCapsuleActions;                         break;
        case Types::Image:          menu = ui->menuImageActions;                           break;
        case Types::Region:         menu = ui->menuRegionActions;                          break;
        case Types::Padding:        menu = ui->menuPaddingActions;                         break;
        case Types::Volume:         menu = ui->menuVolumeActions;                          break;
        case Types::File:           menu = ui->menuFileActions;                            break;
        case Types::Section:        menu = ui->menuSectionActions;                         break;
        case Types::VssStore:
        case Types::Vss2Store:
        case Types::FdcStore:
        case Types::SysFStore:
        case Types::EvsaStore:
        case Types::FtwStore:
        case Types::PhoenixFlashMapStore:
        case Types::InsydeFlashDeviceMapStore:
        case Types::DellDvarStore:
        case Types::NvarGuidStore:
        case Types::CmdbStore:
        case Types::FptStore:
        case Types::CpdStore:
        case Types::BpdtStore:      menu = ui->menuStoreActions;                           break;
        case Types::FreeSpace:      break; // No menu needed for FreeSpace item
        default:                    menu = ui->menuEntryActions;                           break;
    }

    if (menu) {
        QList<QAction*> actions = menu->actions();
        QAction *separator = new QAction(nullptr);
        separator->setSeparator(true);
        QMenu::exec(
            actions << separator << ui->actionExpandItemRecursively << ui->actionCollapseItemRecursively, gp);
        delete separator;
    }
}

void UEFITool::readSettings()
{
    QSettings settings(this);
    restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    QByteArray state = settings.value("mainWindow/windowState").toByteArray();
    if (state.size() > 0x100)   // stupid check for transition from classic UI to docks
        restoreState(state);
    else
        resetDocks();
    ui->structureTreeView->setColumnWidth(0, settings.value("tree/columnWidth0", ui->structureTreeView->columnWidth(0)).toInt());
    ui->structureTreeView->setColumnWidth(1, settings.value("tree/columnWidth1", ui->structureTreeView->columnWidth(1)).toInt());
    ui->structureTreeView->setColumnWidth(2, settings.value("tree/columnWidth2", ui->structureTreeView->columnWidth(2)).toInt());
    ui->structureTreeView->setColumnWidth(3, settings.value("tree/columnWidth3", ui->structureTreeView->columnWidth(3)).toInt());
    markingEnabled = settings.value("tree/markingEnabled", true).toBool();
    ui->actionToggleBootGuardMarking->setChecked(markingEnabled);
    openImageDir = settings.value("paths/openImageDir", ".").toString();
    openGuidDatabaseDir = settings.value("paths/openGuidDatabaseDir", ".").toString();
    extractDir = settings.value("paths/extractDir", ".").toString();
    recentFiles = settings.value("paths/recentFiles").toStringList();

    // Set monospace font
    QString fontName;
    int fontSize;
#if defined Q_OS_MACOS
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
    currentFont.setStyleHint(QFont::Monospace);
    QApplication::setFont(currentFont);
    QFont hexFont = currentFont;
    hexFont.setStretch(QFont::SemiCondensed);
    selectedHexView.setFont(hexFont);
}

void UEFITool::writeSettings()
{
    QSettings settings(this);
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());
    settings.setValue("tree/columnWidth0", ui->structureTreeView->columnWidth(0));
    settings.setValue("tree/columnWidth1", ui->structureTreeView->columnWidth(1));
    settings.setValue("tree/columnWidth2", ui->structureTreeView->columnWidth(2));
    settings.setValue("tree/columnWidth3", ui->structureTreeView->columnWidth(3));
    settings.setValue("tree/markingEnabled", markingEnabled);
    settings.setValue("mainWindow/fontName", currentFont.family());
    settings.setValue("mainWindow/fontSize", currentFont.pointSize());
    settings.setValue("paths/openImageDir", openImageDir);
    settings.setValue("paths/openGuidDatabaseDir", openGuidDatabaseDir);
    settings.setValue("paths/extractDir", extractDir);
    settings.setValue("paths/recentFiles", recentFiles);
}

void UEFITool::showFitTable()
{
    std::vector<std::pair<std::vector<UString>, UModelIndex> > fitTable = ffsParser->getFitTable();
    if (fitTable.empty()) {
        // Disable FIT tab
        enableDock(ui->fitDock, false);
        return;
    }
    
    // Enable FIT tab
    enableDock(ui->fitDock, true);
    
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
    ui->fitDock->raise();
}

void UEFITool::showSecurityInfo()
{
    // Get security info
    UString secInfo = ffsParser->getSecurityInfo();
    if (secInfo.isEmpty()) {
        enableDock(ui->securityDock, false);
        return;
    }
    
    enableDock(ui->securityDock, true);
    ui->securityEdit->setPlainText(secInfo);
    ui->securityDock->raise();
}

void UEFITool::loadGuidDatabase()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select GUID database file to load"), openGuidDatabaseDir, tr("Comma-separated values files (*.csv);;All files (*)"));
    if (!path.isEmpty()) {
        initGuidDatabase(path);
        if (!currentPath.isEmpty() && QMessageBox::Yes == QMessageBox::information(this, tr("New GUID database loaded"), tr("Apply new GUID database on the opened file?\nUnsaved changes and tree position will be lost."), QMessageBox::Yes, QMessageBox::No))
            openImageFile(currentPath);
        openGuidDatabaseDir = QFileInfo(path).absolutePath();
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

void UEFITool::clearRecentlyOpenedFilesList()
{
    recentFiles.clear();
    updateRecentFilesMenu();
}

void UEFITool::copyItemName()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    clipboard->clear();
    clipboard->setText(model->name(index));
}

void UEFITool::expandItemRecursively()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    // Expand the whole section
    recursivelyUpdateItemExpandedState(index, true);
}

void UEFITool::collapseItemRecursively()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    // Collapse the whole section
    ui->structureTreeView->collapse(index);
    recursivelyUpdateItemExpandedState(index, false);
}

void UEFITool::recursivelyUpdateItemExpandedState(QModelIndex index, bool state)
{
    if (!index.isValid())
        return;
    
    ui->structureTreeView->setExpanded(index, state);
    
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = model->index(i, 0, index);
        recursivelyUpdateItemExpandedState(current, state);
    }
}

void UEFITool::hashCrc32()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doCrc32(data);
}

void UEFITool::hashSha1()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doSha1(data);
}

void UEFITool::hashSha256()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doSha256(data);
}

void UEFITool::hashSha384()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doSha384(data);
}

void UEFITool::hashSha512()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doSha512(data);
}

void UEFITool::hashSm3()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    doSm3(data);
}

void UEFITool::hashBodyCrc32()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doCrc32(data);
}

void UEFITool::hashBodySha1()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doSha1(data);
}

void UEFITool::hashBodySha256()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doSha256(data);
}

void UEFITool::hashBodySha384()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doSha384(data);
}

void UEFITool::hashBodySha512()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doSha512(data);
}

void UEFITool::hashBodySm3()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->body(index);
    doSm3(data);
}

void UEFITool::hashUncompressedCrc32()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doCrc32(data);
}

void UEFITool::hashUncompressedSha1()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doSha1(data);
}

void UEFITool::hashUncompressedSha256()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doSha256(data);
}

void UEFITool::hashUncompressedSha384()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doSha384(data);
}

void UEFITool::hashUncompressedSha512()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doSha512(data);
}

void UEFITool::hashUncompressedSm3()
{
    QModelIndex index = ui->structureTreeView->selectionModel()->currentIndex();
    if (!index.isValid())
        return;
    
    QByteArray data = model->uncompressedData(index);
    doSm3(data);
}

void UEFITool::doCrc32(QByteArray data)
{
    uint32_t crc = (uint32_t)crc32(0, (const uint8_t*)data.constData(), (unsigned int)data.size());
    QString value = usprintf("%08X", crc);
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("CRC32"), value, QMessageBox::Ok);
}

void UEFITool::doSha1(QByteArray data)
{
    UINT8 digest[SHA1_HASH_SIZE] = {};
    QString value;

    // SHA1
    sha1(data.constData(), data.size(), digest);
    for (UINT8 i = 0; i < SHA1_HASH_SIZE; i++) {
        value += usprintf("%02X", digest[i]);
    }
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("SHA1"), value, QMessageBox::Ok);
}

void UEFITool::doSha256(QByteArray data)
{
    UINT8 digest[SHA256_HASH_SIZE] = {};
    QString value;

    // SHA2-256
    sha256(data.constData(), data.size(), digest);
    for (UINT8 i = 0; i < SHA256_HASH_SIZE; i++) {
        value += usprintf("%02X", digest[i]);
    }
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("SHA2-256"), value, QMessageBox::Ok);
}

void UEFITool::doSha384(QByteArray data)
{
    UINT8 digest[SHA384_HASH_SIZE] = {};
    QString value;

    // SHA2-384
    sha384(data.constData(), data.size(), digest);
    for (UINT8 i = 0; i < SHA384_HASH_SIZE; i++) {
        value += usprintf("%02X", digest[i]);
    }
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("SHA2-384"), value, QMessageBox::Ok);
}

void UEFITool::doSha512(QByteArray data)
{
    UINT8 digest[SHA512_HASH_SIZE] = {};
    QString value;

    // SHA2-512
    sha512(data.constData(), data.size(), digest);
    for (UINT8 i = 0; i < SHA512_HASH_SIZE; i++) {
        value += usprintf("%02X", digest[i]);
    }
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("SHA2-512"), value, QMessageBox::Ok);
}

void UEFITool::doSm3(QByteArray data)
{
    UINT8 digest[SM3_HASH_SIZE] = {};
    QString value;

    // SM3
    sm3(data.constData(), data.size(), digest);
    for (UINT8 i = 0; i < SM3_HASH_SIZE; i++) {
        value += usprintf("%02X", digest[i]);
    }
    
    clipboard->clear();
    clipboard->setText(value);
    QMessageBox::information(this, tr("SM3"), value, QMessageBox::Ok);
}
