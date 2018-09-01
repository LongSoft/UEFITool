cmake_minimum_required(VERSION 3.0)

PROJECT(UEFIReplace)

SET(PROJECT_SOURCES
 uefireplace_main.cpp
 uefireplace.cpp
 ../common/guiddatabase.cpp
 ../common/types.cpp
 ../common/descriptor.cpp
 ../common/ffs.cpp
 ../common/nvram.cpp
 ../common/nvramparser.cpp
 ../common/ffsparser.cpp
 ../common/ffsbuilder.cpp
 ../common/ffsops.cpp
 ../common/peimage.cpp
 ../common/treeitem.cpp
 ../common/treemodel.cpp
 ../common/utility.cpp
 ../common/LZMA/LzmaCompress.c 
 ../common/LZMA/LzmaDecompress.c 
 ../common/LZMA/SDK/C/LzFind.c 
 ../common/LZMA/SDK/C/LzmaDec.c 
 ../common/LZMA/SDK/C/LzmaEnc.c 
 ../common/Tiano/EfiTianoDecompress.c 
 ../common/Tiano/EfiTianoCompress.c 
 ../common/Tiano/EfiTianoCompressLegacy.c 
 ../common/ustring.cpp
 ../common/sha256.c
 ../common/bstrlib/bstrlib.c
 ../common/bstrlib/bstrwrap.cpp
)

SET(PROJECT_HEADERS
 uefireplace.h
 ../common/guiddatabase.h
 ../common/basetypes.h
 ../common/descriptor.h
 ../common/gbe.h
 ../common/me.h
 ../common/ffs.h
 ../common/nvram.h
 ../common/nvramparser.h
 ../common/ffsparser.h
 ../common/ffsbuilder.h
 ../common/ffsops.h
 ../common/peimage.h
 ../common/types.h
 ../common/treeitem.h
 ../common/treemodel.h
 ../common/utility.h
 ../common/LZMA/LzmaDecompress.h
 ../common/Tiano/EfiTianoDecompress.h
 ../common/LZMA/LzmaCompress.h
 ../common/Tiano/EfiTianoCompress.h
 ../common/ubytearray.h
 ../common/ustring.h
 ../common/bootguard.h
 ../common/sha256.h
 ../common/filesystem.h
 ../common/bstrlib/bstrlib.h
 ../common/bstrlib/bstrwrap.h
 ../version.h
)

ADD_DEFINITIONS(-DU_ENABLE_NVRAM_PARSING_SUPPORT -DU_ENABLE_FIT_PARSING_SUPPORT -DU_ENABLE_GUID_DATABASE_SUPPORT)

ADD_EXECUTABLE(UEFIReplace ${PROJECT_SOURCES} ${PROJECT_HEADERS})
