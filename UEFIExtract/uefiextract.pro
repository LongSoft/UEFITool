QT      += core
QT      -= gui

TARGET   = UEFIExtract
TEMPLATE = app
CONFIG  += console
CONFIG  -= app_bundle
DEFINES += "U_ENABLE_NVRAM_PARSING_SUPPORT"

SOURCES += \
 uefiextract_main.cpp \
 ffsdumper.cpp \
 ../common/guiddatabase.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/nvram.cpp \
 ../common/nvramparser.cpp \
 ../common/ffsparser.cpp \
 ../common/ffsreport.cpp \
 ../common/peimage.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/utility.cpp \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/Tiano/EfiTianoDecompress.c \
 ../common/ustring.cpp \
 ../common/sha256.c

HEADERS += \
 ffsdumper.h \
 ../common/guiddatabase.h \
 ../common/basetypes.h \
 ../common/descriptor.h \
 ../common/gbe.h \
 ../common/me.h \
 ../common/ffs.h \
 ../common/nvram.h \
 ../common/nvramparser.h \
 ../common/ffsparser.h \
 ../common/ffsreport.h \
 ../common/peimage.h \
 ../common/types.h \
 ../common/treeitem.h \
 ../common/treemodel.h \
 ../common/utility.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h \
 ../common/ubytearray.h \
 ../common/ustring.h \
 ../common/bootguard.h \
 ../common/sha256.h \
 ../version.h

