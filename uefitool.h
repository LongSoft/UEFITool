/* uefitool.h

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef __UEFITOOL_H__
#define __UEFITOOL_H__

#include <QMainWindow>
#include <QByteArray>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QString>
#include <QTreeView>
#include <QUrl>
#include <QPixmap>

#include "basetypes.h"
#include "ffs.h"
#include "ffsengine.h"
#include "searchdialog.h"



namespace Ui {
    class UEFITool;
}

class UEFITool : public QMainWindow
{
    Q_OBJECT


public:

    explicit UEFITool(QWidget *parent = 0);
    ~UEFITool();

  //  QSize sizeHint() const override;
    void openImageFile(QString path);
    void setProgramPath(QString path);

    private slots:
    void init();
    void populateUi(const QModelIndex &current);
    void scrollTreeView(QListWidgetItem* item);

    void openImageFile();
    void openImageFileInNewWindow();
    void saveImageFile();
    void search();

    void extract(const UINT8 mode);
    void extractAsIs();
    void extractBody();

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

    void about();
    void aboutQt();

    void exit();
    void writeSettings();
    void toggleViews();
    void setMaxView();
    void setFullView();
    void saveScreenshot();
    void setZoomFactor();
    void hideWindowPanes();
    void updateSplitValues();

private:
    QPixmap originalPixmap;
    Ui::UEFITool* ui;
    FfsEngine* ffsEngine;
    SearchDialog* searchDialog;
    QClipboard* clipboard;
    QString currentDir;
    QString currentProgramPath;
    QQueue<MessageListItem> messageItems;
    const QString version;
    bool firstRun;
    int zoomFactor;
    int boxHeight;
    int boxWidth;

    void createMask();
    void showMessages();

    void setRoundedStyle();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void readSettings();



    QPoint dragPosition;




protected:
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void moveEvent(QMoveEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;


};

#endif
