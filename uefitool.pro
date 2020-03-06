QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET    = UEFITool
TEMPLATE  = app

SOURCES  += uefitool_main.cpp \
 uefitool.cpp \
 searchdialog.cpp \
 types.cpp \
 descriptor.cpp \
 ffs.cpp \
 peimage.cpp \
 ffsengine.cpp \
 treeitem.cpp \
 treemodel.cpp \
 messagelistitem.cpp \
 guidlineedit.cpp \
 LZMA/LzmaCompress.c \
 LZMA/LzmaDecompress.c \
 LZMA/SDK/C/LzFind.c \
 LZMA/SDK/C/LzmaDec.c \
 LZMA/SDK/C/LzmaEnc.c \
 LZMA/SDK/C/Bra86.c \
 Tiano/EfiTianoDecompress.c \
 Tiano/EfiTianoCompress.c \
 Tiano/EfiTianoCompressLegacy.c

HEADERS  += uefitool.h \
 searchdialog.h \
 basetypes.h \
 descriptor.h \
 gbe.h \
 me.h \
 ffs.h \
 peimage.h \
 types.h \
 ffsengine.h \
 treeitem.h \
 treemodel.h \
 messagelistitem.h \
 guidlineedit.h \
 version.h \
 LZMA/LzmaCompress.h \
 LZMA/LzmaDecompress.h \
 LZMA/x86Convert.h \
 Tiano/EfiTianoDecompress.h \
 Tiano/EfiTianoCompress.h

FORMS    += uefitool.ui \
 searchdialog.ui

RC_FILE   = uefitool.rc

ICON      = uefitool.icns

