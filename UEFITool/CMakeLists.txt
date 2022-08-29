CMAKE_MINIMUM_REQUIRED(VERSION 3.16)

PROJECT(UEFITool LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Widgets)
QT_STANDARD_PROJECT_SETUP()

SET(PROJECT_FORMS
 uefitool.ui
 searchdialog.ui
 hexviewdialog.ui
 gotobasedialog.ui
 gotoaddressdialog.ui
 )

SET(PROJECT_HEADERS 
 uefitool.h
 hexspinbox.h
 searchdialog.h
 hexviewdialog.h
 gotobasedialog.h
 gotoaddressdialog.h
)

SET(PROJECT_SOURCES 
 icons/uefitool.icns
 uefitool.rc
 uefitool_main.cpp
 uefitool.cpp
 searchdialog.cpp
 hexviewdialog.cpp
 guidlineedit.cpp
 ffsfinder.cpp
 hexspinbox.cpp
 qhexedit2/qhexedit.cpp
 qhexedit2/chunks.cpp
 qhexedit2/commands.cpp
 ../common/fitparser.cpp 
 ../common/guiddatabase.cpp 
 ../common/nvram.cpp 
 ../common/nvramparser.cpp 
 ../common/meparser.cpp 
 ../common/ffsops.cpp 
 ../common/types.cpp 
 ../common/descriptor.cpp 
 ../common/ffs.cpp 
 ../common/peimage.cpp 
 ../common/utility.cpp 
 ../common/ffsbuilder.cpp 
 ../common/ffsparser.cpp 
 ../common/ffsreport.cpp 
 ../common/ffsutils.cpp 
 ../common/treeitem.cpp 
 ../common/treemodel.cpp 
 ../common/LZMA/LzmaCompress.c 
 ../common/LZMA/LzmaDecompress.c 
 ../common/LZMA/SDK/C/CpuArch.c 
 ../common/LZMA/SDK/C/Bra.c 
 ../common/LZMA/SDK/C/Bra86.c 
 ../common/LZMA/SDK/C/LzFind.c 
 ../common/LZMA/SDK/C/LzmaDec.c 
 ../common/LZMA/SDK/C/LzmaEnc.c 
 ../common/Tiano/EfiTianoDecompress.c 
 ../common/Tiano/EfiTianoCompress.c 
 ../common/Tiano/EfiTianoCompressLegacy.c 
 ../common/ustring.cpp 
 ../common/digest/sha1.c 
 ../common/digest/sha256.c 
 ../common/digest/sha512.c 
 ../common/digest/sm3.c 
 ../common/generated/intel_acbp_v1.cpp 
 ../common/generated/intel_acbp_v2.cpp 
 ../common/generated/intel_keym_v1.cpp 
 ../common/generated/intel_keym_v2.cpp 
 ../common/generated/intel_acm.cpp 
 ../common/kaitai/kaitaistream.cpp 
 ../common/zlib/adler32.c 
 ../common/zlib/compress.c 
 ../common/zlib/crc32.c 
 ../common/zlib/deflate.c 
 ../common/zlib/gzclose.c 
 ../common/zlib/gzlib.c 
 ../common/zlib/gzread.c 
 ../common/zlib/gzwrite.c 
 ../common/zlib/inflate.c 
 ../common/zlib/infback.c 
 ../common/zlib/inftrees.c 
 ../common/zlib/inffast.c 
 ../common/zlib/trees.c 
 ../common/zlib/uncompr.c 
 ../common/zlib/zutil.c 
)

QT_ADD_RESOURCES(PROJECT_SOURCES 
 uefitool.qrc
)

ADD_DEFINITIONS(
 -DU_ENABLE_NVRAM_PARSING_SUPPORT
 -DU_ENABLE_ME_PARSING_SUPPORT
 -DU_ENABLE_FIT_PARSING_SUPPORT
 -DU_ENABLE_GUID_DATABASE_SUPPORT
)

SET_SOURCE_FILES_PROPERTIES(icons/uefitool.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

ADD_EXECUTABLE(UEFITool ${PROJECT_HEADERS} ${PROJECT_FORMS} ${PROJECT_SOURCES})

TARGET_INCLUDE_DIRECTORIES(UEFITool PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

TARGET_LINK_LIBRARIES(UEFITool PRIVATE Qt6::Widgets)

SET_TARGET_PROPERTIES(UEFITool PROPERTIES
 WIN32_EXECUTABLE ON
 MACOSX_BUNDLE ON
 MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
)
