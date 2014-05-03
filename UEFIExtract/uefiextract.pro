QT       += core
QT       -= gui

TARGET    = UEFIExtract
TEMPLATE  = app
CONFIG   += console
DEFINES  += _CONSOLE

SOURCES  += uefiextract_main.cpp \
 uefiextract.cpp \
 ../types.cpp \
 ../descriptor.cpp \
 ../ffs.cpp \
 ../ffsengine.cpp \
 ../treeitem.cpp \
 ../treemodel.cpp \
 ../LZMA/LzmaCompress.c \
 ../LZMA/LzmaDecompress.c \
 ../LZMA/SDK/C/LzFind.c \
 ../LZMA/SDK/C/LzmaDec.c \
 ../LZMA/SDK/C/LzmaEnc.c \
 ../Tiano/EfiTianoDecompress.c \
 ../Tiano/EfiTianoCompress.c

HEADERS  += uefiextract.h \
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
 ../LZMA/LzmaCompress.h \
 ../LZMA/LzmaDecompress.h \
 ../Tiano/EfiTianoDecompress.h \
 ../Tiano/EfiTianoCompress.h
 
