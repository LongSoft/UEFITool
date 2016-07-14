QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UEFITool
TEMPLATE = app

HEADERS += uefitool.h \
 searchdialog.h \
 hexviewdialog.h \
 messagelistitem.h \
 guidlineedit.h \
 ffsfinder.h \
 ../common/nvram.h \
 ../common/ffsops.h \
 ../common/basetypes.h \
 ../common/descriptor.h \
 ../common/gbe.h \
 ../common/me.h \
 ../common/ffs.h \
 ../common/fit.h \
 ../common/peimage.h \
 ../common/types.h \
 ../common/utility.h \
 ../common/parsingdata.h \
 ../common/ffsbuilder.h \
 ../common/ffsparser.h \
 ../common/treeitem.h \
 ../common/treemodel.h \
 ../common/LZMA/LzmaCompress.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h \
 ../common/Tiano/EfiTianoCompress.h \
 ../common/ustring.h \
 ../common/ubytearray.h \
 ../qhexedit2/qhexedit.h \
 ../qhexedit2/chunks.h \
 ../qhexedit2/commands.h 

SOURCES += uefitool_main.cpp \
 uefitool.cpp \
 searchdialog.cpp \
 hexviewdialog.cpp \
 messagelistitem.cpp \
 guidlineedit.cpp \
 ffsfinder.cpp \
 ../common/nvram.cpp \
 ../common/ffsops.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/peimage.cpp \
 ../common/utility.cpp \
 ../common/ffsbuilder.cpp \
 ../common/ffsparser.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/LZMA/LzmaCompress.c \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/LzFind.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/LZMA/SDK/C/LzmaEnc.c \
 ../common/Tiano/EfiTianoDecompress.c \
 ../common/Tiano/EfiTianoCompress.c \
 ../common/Tiano/EfiTianoCompressLegacy.c \
 ../common/ustring.cpp \
 ../qhexedit2/qhexedit.cpp \
 ../qhexedit2/chunks.cpp \
 ../qhexedit2/commands.cpp

FORMS += uefitool.ui \
 searchdialog.ui \
 hexviewdialog.ui

RC_FILE = uefitool.rc

ICON = icons/uefitool.icns

