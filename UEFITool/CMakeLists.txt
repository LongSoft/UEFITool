CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

PROJECT(UEFITool LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Widgets)

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
 qhexview5/qhexview.h
)

SET(PROJECT_SOURCES 
 icons/uefitool.icns
 uefitool.rc
 uefitool_main.cpp
 uefitool.cpp
 searchdialog.cpp
 hexviewdialog.cpp
 hexlineedit.cpp
 ffsfinder.cpp
 hexspinbox.cpp
 qhexview5/model/buffer/qhexbuffer.cpp
 qhexview5/model/buffer/qdevicebuffer.cpp
 qhexview5/model/buffer/qmemorybuffer.cpp
 qhexview5/model/commands/hexcommand.cpp
 qhexview5/model/commands/insertcommand.cpp
 qhexview5/model/commands/removecommand.cpp
 qhexview5/model/commands/replacecommand.cpp
 qhexview5/model/qhexcursor.cpp
 qhexview5/model/qhexdelegate.cpp
 qhexview5/model/qhexdocument.cpp
 qhexview5/model/qhexmetadata.cpp
 qhexview5/model/qhexutils.cpp
 qhexview5/qhexview.cpp
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
 ../common/generated/ami_nvar.cpp
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
 AUTOMOC ON
 AUTOUIC ON
)

IF(UNIX AND (NOT APPLE) AND (NOT CYGWIN))
 SET_TARGET_PROPERTIES(UEFITool PROPERTIES OUTPUT_NAME uefitool)
 INSTALL(FILES icons/uefitool_16x16.png   DESTINATION share/icons/hicolor/16x16/apps   RENAME uefitool.png)
 INSTALL(FILES icons/uefitool_32x32.png   DESTINATION share/icons/hicolor/32x32/apps   RENAME uefitool.png)
 INSTALL(FILES icons/uefitool_64x64.png   DESTINATION share/icons/hicolor/64x64/apps   RENAME uefitool.png)
 INSTALL(FILES icons/uefitool_128x128.png DESTINATION share/icons/hicolor/128x128/apps RENAME uefitool.png)
 INSTALL(FILES icons/uefitool_256x256.png DESTINATION share/icons/hicolor/256x256/apps RENAME uefitool.png)
 INSTALL(FILES icons/uefitool_512x512.png DESTINATION share/icons/hicolor/512x512/apps RENAME uefitool.png)
 INSTALL(FILES uefitool.desktop DESTINATION share/applications)
ENDIF()

INSTALL(TARGETS UEFITool BUNDLE DESTINATION "/Applications" )
