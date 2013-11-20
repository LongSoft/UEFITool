/********************************************************************************
** Form generated from reading UI file 'uefitool.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UEFITOOL_H
#define UI_UEFITOOL_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UEFITool
{
public:
    QAction *actionInsertAfter;
    QAction *actionInsertBefore;
    QAction *actionReplace;
    QAction *actionExtract;
    QAction *actionExtractBody;
    QAction *actionExtractUncompressed;
    QAction *actionRemove;
    QAction *actionOpenImageFile;
    QAction *actionInsertInto;
    QAction *actionSaveImageFile;
    QAction *actionRebuild;
    QAction *actionChangeToTiano;
    QAction *actionChangeToEfi11;
    QAction *actionChangeToLzma;
    QAction *actionAbout;
    QAction *actionAboutQt;
    QAction *actionQuit;
    QAction *actionChangeToNone;
    QWidget *centralWidget;
    QGridLayout *gridLayout_2;
    QGroupBox *structureGroupBox;
    QGridLayout *gridLayout;
    QTreeView *structureTreeView;
    QGroupBox *infoGroupBox;
    QVBoxLayout *verticalLayout;
    QPlainTextEdit *infoEdit;
    QGroupBox *messageGroupBox;
    QHBoxLayout *horizontalLayout;
    QListWidget *messageListWidget;
    QStatusBar *statusBar;
    QToolBar *toolBar;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuAction;
    QMenu *menuChangeCompressionTo;
    QMenu *menuHelp;

    void setupUi(QMainWindow *UEFITool)
    {
        if (UEFITool->objectName().isEmpty())
            UEFITool->setObjectName(QString::fromUtf8("UEFITool"));
        UEFITool->resize(800, 483);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(UEFITool->sizePolicy().hasHeightForWidth());
        UEFITool->setSizePolicy(sizePolicy);
        UEFITool->setAcceptDrops(true);
        actionInsertAfter = new QAction(UEFITool);
        actionInsertAfter->setObjectName(QString::fromUtf8("actionInsertAfter"));
        actionInsertAfter->setEnabled(false);
        actionInsertBefore = new QAction(UEFITool);
        actionInsertBefore->setObjectName(QString::fromUtf8("actionInsertBefore"));
        actionInsertBefore->setEnabled(false);
        actionReplace = new QAction(UEFITool);
        actionReplace->setObjectName(QString::fromUtf8("actionReplace"));
        actionReplace->setEnabled(false);
        actionExtract = new QAction(UEFITool);
        actionExtract->setObjectName(QString::fromUtf8("actionExtract"));
        actionExtract->setEnabled(false);
        actionExtractBody = new QAction(UEFITool);
        actionExtractBody->setObjectName(QString::fromUtf8("actionExtractBody"));
        actionExtractBody->setEnabled(false);
        actionExtractUncompressed = new QAction(UEFITool);
        actionExtractUncompressed->setObjectName(QString::fromUtf8("actionExtractUncompressed"));
        actionExtractUncompressed->setEnabled(false);
        actionRemove = new QAction(UEFITool);
        actionRemove->setObjectName(QString::fromUtf8("actionRemove"));
        actionRemove->setEnabled(false);
        actionOpenImageFile = new QAction(UEFITool);
        actionOpenImageFile->setObjectName(QString::fromUtf8("actionOpenImageFile"));
        actionInsertInto = new QAction(UEFITool);
        actionInsertInto->setObjectName(QString::fromUtf8("actionInsertInto"));
        actionInsertInto->setEnabled(false);
        actionSaveImageFile = new QAction(UEFITool);
        actionSaveImageFile->setObjectName(QString::fromUtf8("actionSaveImageFile"));
        actionSaveImageFile->setEnabled(false);
        actionRebuild = new QAction(UEFITool);
        actionRebuild->setObjectName(QString::fromUtf8("actionRebuild"));
        actionRebuild->setEnabled(false);
        actionChangeToTiano = new QAction(UEFITool);
        actionChangeToTiano->setObjectName(QString::fromUtf8("actionChangeToTiano"));
        actionChangeToEfi11 = new QAction(UEFITool);
        actionChangeToEfi11->setObjectName(QString::fromUtf8("actionChangeToEfi11"));
        actionChangeToLzma = new QAction(UEFITool);
        actionChangeToLzma->setObjectName(QString::fromUtf8("actionChangeToLzma"));
        actionAbout = new QAction(UEFITool);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        actionAbout->setMenuRole(QAction::AboutRole);
        actionAboutQt = new QAction(UEFITool);
        actionAboutQt->setObjectName(QString::fromUtf8("actionAboutQt"));
        actionAboutQt->setMenuRole(QAction::AboutQtRole);
        actionQuit = new QAction(UEFITool);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        actionQuit->setMenuRole(QAction::QuitRole);
        actionChangeToNone = new QAction(UEFITool);
        actionChangeToNone->setObjectName(QString::fromUtf8("actionChangeToNone"));
        centralWidget = new QWidget(UEFITool);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy1);
        gridLayout_2 = new QGridLayout(centralWidget);
        gridLayout_2->setSpacing(5);
        gridLayout_2->setContentsMargins(5, 5, 5, 5);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        structureGroupBox = new QGroupBox(centralWidget);
        structureGroupBox->setObjectName(QString::fromUtf8("structureGroupBox"));
        sizePolicy.setHeightForWidth(structureGroupBox->sizePolicy().hasHeightForWidth());
        structureGroupBox->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(structureGroupBox);
        gridLayout->setSpacing(5);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        structureTreeView = new QTreeView(structureGroupBox);
        structureTreeView->setObjectName(QString::fromUtf8("structureTreeView"));
        structureTreeView->setIndentation(10);
        structureTreeView->setHeaderHidden(false);
        structureTreeView->header()->setCascadingSectionResizes(true);
        structureTreeView->header()->setDefaultSectionSize(200);
        structureTreeView->header()->setStretchLastSection(true);

        gridLayout->addWidget(structureTreeView, 0, 0, 1, 2);


        gridLayout_2->addWidget(structureGroupBox, 0, 0, 1, 1);

        infoGroupBox = new QGroupBox(centralWidget);
        infoGroupBox->setObjectName(QString::fromUtf8("infoGroupBox"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(infoGroupBox->sizePolicy().hasHeightForWidth());
        infoGroupBox->setSizePolicy(sizePolicy2);
        infoGroupBox->setMaximumSize(QSize(220, 16777215));
        verticalLayout = new QVBoxLayout(infoGroupBox);
        verticalLayout->setSpacing(5);
        verticalLayout->setContentsMargins(5, 5, 5, 5);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        infoEdit = new QPlainTextEdit(infoGroupBox);
        infoEdit->setObjectName(QString::fromUtf8("infoEdit"));
        sizePolicy.setHeightForWidth(infoEdit->sizePolicy().hasHeightForWidth());
        infoEdit->setSizePolicy(sizePolicy);
        infoEdit->setUndoRedoEnabled(false);
        infoEdit->setReadOnly(true);
        infoEdit->setCenterOnScroll(false);

        verticalLayout->addWidget(infoEdit);


        gridLayout_2->addWidget(infoGroupBox, 0, 1, 1, 1);

        messageGroupBox = new QGroupBox(centralWidget);
        messageGroupBox->setObjectName(QString::fromUtf8("messageGroupBox"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(messageGroupBox->sizePolicy().hasHeightForWidth());
        messageGroupBox->setSizePolicy(sizePolicy3);
        messageGroupBox->setMaximumSize(QSize(16777215, 120));
        horizontalLayout = new QHBoxLayout(messageGroupBox);
        horizontalLayout->setSpacing(5);
        horizontalLayout->setContentsMargins(5, 5, 5, 5);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        messageListWidget = new QListWidget(messageGroupBox);
        messageListWidget->setObjectName(QString::fromUtf8("messageListWidget"));

        horizontalLayout->addWidget(messageListWidget);


        gridLayout_2->addWidget(messageGroupBox, 1, 0, 1, 2);

        UEFITool->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(UEFITool);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        UEFITool->setStatusBar(statusBar);
        toolBar = new QToolBar(UEFITool);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setEnabled(true);
        UEFITool->addToolBar(Qt::LeftToolBarArea, toolBar);
        menuBar = new QMenuBar(UEFITool);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 800, 22));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuAction = new QMenu(menuBar);
        menuAction->setObjectName(QString::fromUtf8("menuAction"));
        menuChangeCompressionTo = new QMenu(menuAction);
        menuChangeCompressionTo->setObjectName(QString::fromUtf8("menuChangeCompressionTo"));
        menuChangeCompressionTo->setEnabled(true);
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        UEFITool->setMenuBar(menuBar);

        toolBar->addAction(actionOpenImageFile);
        toolBar->addAction(actionSaveImageFile);
        toolBar->addSeparator();
        toolBar->addAction(actionRebuild);
        toolBar->addSeparator();
        toolBar->addAction(actionRemove);
        toolBar->addSeparator();
        toolBar->addAction(actionExtract);
        toolBar->addAction(actionExtractBody);
        toolBar->addAction(actionExtractUncompressed);
        toolBar->addSeparator();
        toolBar->addAction(actionInsertInto);
        toolBar->addAction(actionInsertBefore);
        toolBar->addAction(actionInsertAfter);
        toolBar->addSeparator();
        toolBar->addAction(actionReplace);
        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuAction->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionOpenImageFile);
        menuFile->addAction(actionSaveImageFile);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);
        menuAction->addAction(actionRebuild);
        menuAction->addSeparator();
        menuAction->addAction(actionRemove);
        menuAction->addSeparator();
        menuAction->addAction(actionExtract);
        menuAction->addAction(actionExtractBody);
        menuAction->addAction(actionExtractUncompressed);
        menuAction->addSeparator();
        menuAction->addAction(actionInsertInto);
        menuAction->addAction(actionInsertAfter);
        menuAction->addAction(actionInsertBefore);
        menuAction->addSeparator();
        menuAction->addAction(actionReplace);
        menuAction->addSeparator();
        menuAction->addAction(menuChangeCompressionTo->menuAction());
        menuChangeCompressionTo->addAction(actionChangeToNone);
        menuChangeCompressionTo->addAction(actionChangeToTiano);
        menuChangeCompressionTo->addAction(actionChangeToEfi11);
        menuChangeCompressionTo->addAction(actionChangeToLzma);
        menuHelp->addAction(actionAbout);
        menuHelp->addAction(actionAboutQt);

        retranslateUi(UEFITool);

        QMetaObject::connectSlotsByName(UEFITool);
    } // setupUi

    void retranslateUi(QMainWindow *UEFITool)
    {
        UEFITool->setWindowTitle(QApplication::translate("UEFITool", "UEFITool 0.9.3", 0, QApplication::UnicodeUTF8));
        actionInsertAfter->setText(QApplication::translate("UEFITool", "Insert &after...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionInsertAfter->setToolTip(QApplication::translate("UEFITool", "Insert an object from file after selected object", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionInsertAfter->setShortcut(QApplication::translate("UEFITool", "Ctrl+Shift+I", 0, QApplication::UnicodeUTF8));
        actionInsertBefore->setText(QApplication::translate("UEFITool", "Insert &before...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionInsertBefore->setToolTip(QApplication::translate("UEFITool", "Insert object from file before selected object", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionInsertBefore->setShortcut(QApplication::translate("UEFITool", "Ctrl+Alt+I", 0, QApplication::UnicodeUTF8));
        actionReplace->setText(QApplication::translate("UEFITool", "Rep&lace", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionReplace->setToolTip(QApplication::translate("UEFITool", "Replace selected object with an object from file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionReplace->setShortcut(QApplication::translate("UEFITool", "Ctrl+R", 0, QApplication::UnicodeUTF8));
        actionExtract->setText(QApplication::translate("UEFITool", "E&xtract as is...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionExtract->setToolTip(QApplication::translate("UEFITool", "Extract selected object as is to file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionExtract->setShortcut(QApplication::translate("UEFITool", "Ctrl+E", 0, QApplication::UnicodeUTF8));
        actionExtractBody->setText(QApplication::translate("UEFITool", "Extract &without header...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionExtractBody->setToolTip(QApplication::translate("UEFITool", "Extract selected object without header to file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionExtractBody->setShortcut(QApplication::translate("UEFITool", "Ctrl+Shift+E", 0, QApplication::UnicodeUTF8));
        actionExtractUncompressed->setText(QApplication::translate("UEFITool", "Extract &uncompressed...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionExtractUncompressed->setToolTip(QApplication::translate("UEFITool", "Extract selected FFS file uncompressing it ", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionExtractUncompressed->setShortcut(QApplication::translate("UEFITool", "Ctrl+Alt+E", 0, QApplication::UnicodeUTF8));
        actionRemove->setText(QApplication::translate("UEFITool", "Re&move", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionRemove->setToolTip(QApplication::translate("UEFITool", "Remove selected object", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionRemove->setShortcut(QApplication::translate("UEFITool", "Ctrl+Del", 0, QApplication::UnicodeUTF8));
        actionOpenImageFile->setText(QApplication::translate("UEFITool", "&Open image file...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionOpenImageFile->setToolTip(QApplication::translate("UEFITool", "Open image file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionOpenImageFile->setShortcut(QApplication::translate("UEFITool", "Ctrl+O", 0, QApplication::UnicodeUTF8));
        actionInsertInto->setText(QApplication::translate("UEFITool", "Insert &into...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionInsertInto->setToolTip(QApplication::translate("UEFITool", "Insert object from file into selected object", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionInsertInto->setShortcut(QApplication::translate("UEFITool", "Ctrl+I", 0, QApplication::UnicodeUTF8));
        actionSaveImageFile->setText(QApplication::translate("UEFITool", "&Save image file...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionSaveImageFile->setToolTip(QApplication::translate("UEFITool", "Save modified image file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionSaveImageFile->setShortcut(QApplication::translate("UEFITool", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        actionRebuild->setText(QApplication::translate("UEFITool", "&Rebuild", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionRebuild->setToolTip(QApplication::translate("UEFITool", "Rebuild selected object", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionRebuild->setShortcut(QApplication::translate("UEFITool", "Ctrl+Space", 0, QApplication::UnicodeUTF8));
        actionChangeToTiano->setText(QApplication::translate("UEFITool", "&Tiano", 0, QApplication::UnicodeUTF8));
        actionChangeToTiano->setShortcut(QApplication::translate("UEFITool", "Ctrl+T", 0, QApplication::UnicodeUTF8));
        actionChangeToEfi11->setText(QApplication::translate("UEFITool", "&EFI 1.1", 0, QApplication::UnicodeUTF8));
        actionChangeToEfi11->setShortcut(QApplication::translate("UEFITool", "Ctrl+Shift+T", 0, QApplication::UnicodeUTF8));
        actionChangeToLzma->setText(QApplication::translate("UEFITool", "&LZMA", 0, QApplication::UnicodeUTF8));
        actionChangeToLzma->setShortcut(QApplication::translate("UEFITool", "Ctrl+L", 0, QApplication::UnicodeUTF8));
        actionAbout->setText(QApplication::translate("UEFITool", "&About UEFITool", 0, QApplication::UnicodeUTF8));
        actionAbout->setShortcut(QApplication::translate("UEFITool", "F1", 0, QApplication::UnicodeUTF8));
        actionAboutQt->setText(QApplication::translate("UEFITool", "About &Qt", 0, QApplication::UnicodeUTF8));
        actionQuit->setText(QApplication::translate("UEFITool", "&Quit", 0, QApplication::UnicodeUTF8));
        actionChangeToNone->setText(QApplication::translate("UEFITool", "&Uncompressed", 0, QApplication::UnicodeUTF8));
        actionChangeToNone->setShortcut(QApplication::translate("UEFITool", "Ctrl+U", 0, QApplication::UnicodeUTF8));
        structureGroupBox->setTitle(QApplication::translate("UEFITool", "Structure", 0, QApplication::UnicodeUTF8));
        infoGroupBox->setTitle(QApplication::translate("UEFITool", "Information", 0, QApplication::UnicodeUTF8));
        messageGroupBox->setTitle(QApplication::translate("UEFITool", "Message", 0, QApplication::UnicodeUTF8));
        toolBar->setWindowTitle(QApplication::translate("UEFITool", "toolBar", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("UEFITool", "&File", 0, QApplication::UnicodeUTF8));
        menuAction->setTitle(QApplication::translate("UEFITool", "A&ction", 0, QApplication::UnicodeUTF8));
        menuChangeCompressionTo->setTitle(QApplication::translate("UEFITool", "Change &compression to", 0, QApplication::UnicodeUTF8));
        menuHelp->setTitle(QApplication::translate("UEFITool", "H&elp", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class UEFITool: public Ui_UEFITool {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UEFITOOL_H
