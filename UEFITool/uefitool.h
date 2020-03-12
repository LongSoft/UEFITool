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
#include <QByteArray>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QString>
#include <QTableWidget>
#include <QTreeView>
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

    void openImageFile();
    void openImageFileInNewWindow();
    void saveImageFile();

    void search();
    void goToBase();
    void goToAddress();

    void hexView();
    void bodyHexView();
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
    QString currentDir;
    QString currentPath;
    QString currentProgramPath;
    QFont currentFont;
    const QString version;
    bool markingEnabled;

    bool enableExtractBodyUncompressed(const QModelIndex &current);

    bool eventFilter(QObject* obj, QEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void readSettings();
    void showParserMessages();
    void showFinderMessages();
    void showFitTable();
    void showSecurityInfo();
    void showBuilderMessages();

    enum {
        TAB_PARSER,
        TAB_FIT,
        TAB_SECURITY,
        TAB_SEARCH,
        TAB_BUILDER
    };
};

#endif // UEFITOOL_H
