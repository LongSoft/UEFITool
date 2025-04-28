/* uefitool.h

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef UEFITOOL_H
#define UEFITOOL_H

#include <QMainWindow>
#include <QActionGroup>
#include <QByteArray>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFont>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QStyleFactory>
#include <QString>
#include <QTableWidget>
#include <QTreeView>
#include <QToolButton>
#include <QUrl>

#include "../common/basetypes.h"
#include "../common/utility.h"
#include "../common/ffs.h"
#include "../common/ffsparser.h"
#include "../common/ffsops.h"
#include "../common/ffsbuilder.h"
#include "../common/ffsreport.h"
#include "../common/guiddatabase.h"

#include "searchdialog.h"
#include "gotobasedialog.h"
#include "gotoaddressdialog.h"
#include "hexviewdialog.h"
#include "ffsfinder.h"

namespace Ui {
    class UEFITool;
}

class UEFITool : public QMainWindow
{
    Q_OBJECT

public:
    explicit UEFITool(QWidget *parent = 0);
    ~UEFITool();

    void openImageFile(QString path);
    void setProgramPath(QString path) { currentProgramPath = path; }

private slots:
    void init();
    void populateUi(const QItemSelection &selected);
    void populateUi(const QModelIndex &current);
    void scrollTreeView(QListWidgetItem* item); // For messages
    void scrollTreeView(QTableWidgetItem* item); // For FIT table entries

    void onTrackingAction(QAction* action);
    void fileChangedResume();
    void fileChanged(const QString& path);
    void setChangedFileFlag(const bool flag);
    void openImageFile();
    void openImageFileInNewWindow();
    void openRecentImageFile();
    void saveImageFile();

    void search();
    void goToBase();
    void goToAddress();
    void expandTree();

    void hexView();
    void bodyHexView();
    void uncompressedHexView();
    void goToData();

    void extract(const UINT8 mode);
    void extractAsIs();
    void extractBody();
    void extractBodyUncompressed();

    void insert(const UINT8 mode);
    void insertInto();
    void insertBefore();
    void insertAfter();

    void replace(const UINT8 mode);
    void replaceAsIs();
    void replaceBody();

    void rebuild();

    void remove();

    void copyMessage();
    void copyAllMessages();
    void enableMessagesCopyActions(QListWidgetItem* item);
    void clearMessages();

    void toggleBootGuardMarking(bool enabled);
    void toggleCStyleHexValues(bool enabled);
    void setExpandAll();

    void about();
    void aboutQt();

    void exit();
    void writeSettings();

    void loadGuidDatabase();
    void unloadGuidDatabase();
    void loadDefaultGuidDatabase();
    void exportDiscoveredGuids();
    void generateReport();

    void currentTabChanged(int index);

#if QT_VERSION_MAJOR >= 6 && QT_VERSION_MINOR >= 5
    void updateUiForNewColorScheme(Qt::ColorScheme scheme);
#endif

private:
    Ui::UEFITool* ui;
    TreeModel* model;
    FfsParser* ffsParser;
    FfsFinder* ffsFinder;
    FfsReport* ffsReport;
    FfsOperations* ffsOps;
    FfsBuilder* ffsBuilder;
    SearchDialog* searchDialog;
    HexViewDialog* hexViewDialog;
    GoToBaseDialog* goToBaseDialog;
    GoToAddressDialog* goToAddressDialog;
    QClipboard* clipboard;
    QList<QAction*> recentFileActions;
    QFileSystemWatcher watcher;
    QToolButton openedFileLabel;
    QStringList recentFiles;
    QString currentDir;
    QString currentPath;
    QString currentProgramPath;
    QString openImageDir;
    QString openGuidDatabaseDir;
    QString extractDir;
    QFont currentFont;
    const QString version;
    int fileTrackingState;
    bool changedFileFlag;
    bool markingEnabled;
    bool cStyleHexEnabled;

    bool eventFilter(QObject* obj, QEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void updateRecentFilesMenu(const QString& fileName = QString());
    void readSettings();
    void askReopenImageFile();
    void reopenImageFile();
    void showParserMessages();
    void showFinderMessages();
    void showFitTable();
    void showSecurityInfo();
    void showBuilderMessages();
    bool isAllExpanded();
    void saveTreeState(const QModelIndex& index, QHash<QString, bool>& states);
    void restoreTreeState(const QModelIndex& index, const QHash<QString, bool>& states);

    enum {
        TAB_PARSER,
        TAB_FIT,
        TAB_SECURITY,
        TAB_SEARCH,
        TAB_BUILDER
    };

    enum {
        TRACK_MIN = 0,
        TRACK_IGNORE = TRACK_MIN,
        TRACK_ASK,
        TRACK_REOPEN,
        TRACK_MAX = TRACK_REOPEN
    };
};

#endif // UEFITOOL_H
