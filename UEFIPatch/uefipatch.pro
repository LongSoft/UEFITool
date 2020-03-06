QT       += core
QT       -= gui

TARGET    = UEFIPatch
TEMPLATE  = app
CONFIG   += console
CONFIG   -= app_bundle
DEFINES  += _CONSOLE

SOURCES  += uefipatch_main.cpp \
 uefipatch.cpp \
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
 ../LZMA/SDK/C/Bra86.c \
 ../Tiano/EfiTianoDecompress.c \
 ../Tiano/EfiTianoCompress.c \
 ../Tiano/EfiTianoCompressLegacy.c

HEADERS  += uefipatch.h \
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
 ../LZMA/x86Convert.h \
 ../Tiano/EfiTianoDecompress.h \
 ../Tiano/EfiTianoCompress.h
 
