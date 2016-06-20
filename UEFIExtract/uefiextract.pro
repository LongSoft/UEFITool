QT      += core
QT      -= gui

TARGET   = UEFIExtract
TEMPLATE = app
CONFIG  += console
CONFIG  -= app_bundle

SOURCES += uefiextract_main.cpp \
 ffsdumper.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/nvram.cpp \
 ../common/ffsparser.cpp \
 ../common/fitparser.cpp \
 ../common/peimage.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/utility.cpp \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/Tiano/EfiTianoDecompress.c \

HEADERS += ffsdumper.h \
 ../common/basetypes.h \
 ../common/descriptor.h \
 ../common/gbe.h \
 ../common/me.h \
 ../common/ffs.h \
 ../common/nvram.h \
 ../common/ffsparser.h \
 ../common/fitparser.h \
 ../common/peimage.h \
 ../common/types.h \
 ../common/treeitem.h \
 ../common/treemodel.h \
 ../common/utility.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h
 
