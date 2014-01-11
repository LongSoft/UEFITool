QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET    = UEFITool
TEMPLATE  = app

SOURCES  += main.cpp \
            uefitool.cpp \
            searchdialog.cpp \
            descriptor.cpp \
            ffs.cpp \
            ffsengine.cpp \
            treeitem.cpp \
            treemodel.cpp \
            messagelistitem.cpp \
            LZMA/LzmaCompress.c \
            LZMA/LzmaDecompress.c \
            LZMA/SDK/C/LzFind.c \
            LZMA/SDK/C/LzmaDec.c \
            LZMA/SDK/C/LzmaEnc.c \
            Tiano/EfiTianoDecompress.c \
            Tiano/EfiCompress.c \
            Tiano/TianoCompress.c

HEADERS  += uefitool.h \
            searchdialog.h \
            basetypes.h \
            descriptor.h \
            gbe.h \
            me.h \
            ffs.h \
            ffsengine.h \
            treeitem.h \
            treemodel.h \
            messagelistitem.h \
            LZMA/LzmaCompress.h \
            LZMA/LzmaDecompress.h \
            Tiano/EfiTianoDecompress.h \
            Tiano/EfiTianoCompress.h \
             peimage.h

FORMS    += uefitool.ui \
            searchdialog.ui

RC_FILE   = uefitool.rc

ICON      = uefitool.icns
