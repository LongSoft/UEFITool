QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UEFITool
TEMPLATE = app
DEFINES += "U_ENABLE_FIT_PARSING_SUPPORT"
DEFINES += "U_ENABLE_NVRAM_PARSING_SUPPORT"
DEFINES += "U_ENABLE_ME_PARSING_SUPPORT"
DEFINES += "U_ENABLE_GUID_DATABASE_SUPPORT"

HEADERS += uefitool.h \
 searchdialog.h \
 hexviewdialog.h \
 gotobasedialog.h \
 gotoaddressdialog.h \
 guidlineedit.h \
 ffsfinder.h \
 hexspinbox.h \
 ../common/guiddatabase.h \
 ../common/nvram.h \
 ../common/nvramparser.h \
 ../common/meparser.h \
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
 ../common/ffsreport.h \
 ../common/treeitem.h \
 ../common/ffsutils.h \
 ../common/treemodel.h \
 ../common/LZMA/LzmaCompress.h \
 ../common/LZMA/LzmaDecompress.h \
 ../common/Tiano/EfiTianoDecompress.h \
 ../common/Tiano/EfiTianoCompress.h \
 ../common/ustring.h \
 ../common/ubytearray.h \
 ../common/bootguard.h \
 ../common/sha256.h \
 ../common/zlib/zconf.h \
 ../common/zlib/zlib.h \
 ../common/zlib/crc32.h \
 ../common/zlib/deflate.h \
 ../common/zlib/gzguts.h \
 ../common/zlib/inffast.h \
 ../common/zlib/inffixed.h \
 ../common/zlib/inflate.h \
 ../common/zlib/inftrees.h \
 ../common/zlib/trees.h \
 ../common/zlib/zutil.h \
 ../version.h \
 qhexedit2/qhexedit.h \
 qhexedit2/chunks.h \
 qhexedit2/commands.h

SOURCES += uefitool_main.cpp \
 uefitool.cpp \
 searchdialog.cpp \
 hexviewdialog.cpp \
 guidlineedit.cpp \
 ffsfinder.cpp \
 hexspinbox.cpp \
 ../common/guiddatabase.cpp \
 ../common/nvram.cpp \
 ../common/nvramparser.cpp \
 ../common/meparser.cpp \
 ../common/ffsops.cpp \
 ../common/types.cpp \
 ../common/descriptor.cpp \
 ../common/ffs.cpp \
 ../common/peimage.cpp \
 ../common/utility.cpp \
 ../common/ffsbuilder.cpp \
 ../common/ffsparser.cpp \
 ../common/ffsreport.cpp \
 ../common/ffsutils.cpp \
 ../common/treeitem.cpp \
 ../common/treemodel.cpp \
 ../common/LZMA/LzmaCompress.c \
 ../common/LZMA/LzmaDecompress.c \
 ../common/LZMA/SDK/C/Bra86.c \
 ../common/LZMA/SDK/C/LzFind.c \
 ../common/LZMA/SDK/C/LzmaDec.c \
 ../common/LZMA/SDK/C/LzmaEnc.c \
 ../common/Tiano/EfiTianoDecompress.c \
 ../common/Tiano/EfiTianoCompress.c \
 ../common/Tiano/EfiTianoCompressLegacy.c \
 ../common/ustring.cpp \
 ../common/sha256.c \
 ../common/zlib/adler32.c \
 ../common/zlib/compress.c \
 ../common/zlib/crc32.c \
 ../common/zlib/deflate.c \
 ../common/zlib/gzclose.c \
 ../common/zlib/gzlib.c \
 ../common/zlib/gzread.c \
 ../common/zlib/gzwrite.c \
 ../common/zlib/inflate.c \
 ../common/zlib/infback.c \
 ../common/zlib/inftrees.c \
 ../common/zlib/inffast.c \
 ../common/zlib/trees.c \
 ../common/zlib/uncompr.c \
 ../common/zlib/zutil.c \
 qhexedit2/qhexedit.cpp \
 qhexedit2/chunks.cpp \
 qhexedit2/commands.cpp

FORMS += uefitool.ui \
 searchdialog.ui \
 hexviewdialog.ui \
 gotobasedialog.ui \
 gotoaddressdialog.ui

RESOURCES += uefitool.qrc
RC_FILE = uefitool.rc
ICON = icons/uefitool.icns
ICONFILE.files = icons/uefitool.icns
ICONFILE.path = Contents/Resources
QMAKE_BUNDLE_DATA += ICONFILE
QMAKE_INFO_PLIST = Info.plist
