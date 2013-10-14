QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET    = UEFITool
TEMPLATE  = app

SOURCES  += main.cpp \
            uefitool.cpp \
            descriptor.cpp \
            ffs.cpp \
            treeitem.cpp \ 
            treemodel.cpp \
            LZMA/LzmaCompress.c \
            LZMA/LzmaDecompress.c \
            LZMA/SDK/C/LzFind.c \
            LZMA/SDK/C/LzmaDec.c \
            LZMA/SDK/C/LzmaEnc.c \
            Tiano/EfiTianoDecompress.c \
            Tiano/EfiCompress.c \
            Tiano/TianoCompress.c
HEADERS  += uefitool.h \
            basetypes.h \
            descriptor.h \
            ffs.h \
            treeitem.h \
            treeitemtypes.h \			
            treemodel.h \
            LZMA/LzmaCompress.h \
            LZMA/LzmaDecompress.h \
            Tiano/EfiTianoDecompress.h \
            Tiano/EfiTianoCompress.h
FORMS    += uefitool.ui
RC_FILE   = uefitool.rc
ICON      = uefitool.icns
