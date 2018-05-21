QT       += core
QT       -= gui

TARGET    = UEFIFind
TEMPLATE  = app
CONFIG   += console
CONFIG   -= app_bundle

SOURCES  += uefifind_main.cpp \
 uefifind.cpp \
 ../common/guiddatabase.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/nvram.cpp \
 ../common/ffsparser.cpp \
 ../common/peimage.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/utility.cpp \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/Tiano/EfiTianoDecompress.c \
 ../common/ustring.cpp \
 ../common/sha256.c

HEADERS  += uefifind.h \
 ../common/guiddatabase.h \
 ../common/basetypes.h \
 ../common/descriptor.h \
 ../common/gbe.h \
 ../common/me.h \
 ../common/ffs.h \
 ../common/nvram.h \
 ../common/ffsparser.h \
 ../common/peimage.h \
 ../common/types.h \
 ../common/treeitem.h \
 ../common/treemodel.h \
 ../common/utility.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h \
 ../common/ustring.h \
 ../common/ubytearray.h \
 ../common/bootguard.h \
 ../common/sha256.h \
 ../version.h
