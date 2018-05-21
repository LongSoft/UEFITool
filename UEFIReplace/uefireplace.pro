QT       += core
QT       -= gui

TARGET    = UEFIReplace
TEMPLATE  = app
CONFIG   += console
CONFIG   -= app_bundle
DEFINES  += _CONSOLE

SOURCES  += uefireplace_main.cpp \
 uefireplace.cpp \
 ../types.cpp \
 ../descriptor.cpp \
 ../ffs.cpp \
 ../ffsengine.cpp \
 ../peimage.cpp \
 ../treeitem.cpp \
 ../treemodel.cpp \
 ../LZMA/LzmaCompress.c \
 ../LZMA/LzmaDecompress.c \
 ../LZMA/SDK/C/LzFind.c \
 ../LZMA/SDK/C/LzmaDec.c \
 ../LZMA/SDK/C/LzmaEnc.c \
 ../Tiano/EfiTianoDecompress.c \
 ../Tiano/EfiTianoCompress.c \
 ../Tiano/EfiTianoCompressLegacy.c

HEADERS  += uefireplace.h \
 ../basetypes.h \
 ../descriptor.h \
 ../gbe.h \
 ../me.h \
 ../ffs.h \
 ../peimage.h \
 ../types.h \
 ../ffsengine.h \
 ../treeitem.h \
 ../treemodel.h \
 ../version.h \
 ../LZMA/LzmaCompress.h \
 ../LZMA/LzmaDecompress.h \
 ../Tiano/EfiTianoDecompress.h \
 ../Tiano/EfiTianoCompress.h