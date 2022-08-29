lzma = static_library('lzma',
  sources: [
    'LZMA/LzmaDecompress.c',
    'LZMA/SDK/C/Bra86.c',
    'LZMA/SDK/C/LzmaDec.c',
    'Tiano/EfiTianoDecompress.c',
  ],
)

bstrlib = static_library('bstrlib',
  sources: [
    'bstrlib/bstrlib.c',
    'bstrlib/bstrwrap.cpp',
  ],
)

uefitoolcommon = static_library('uefitoolcommon',
  sources: [
    'guiddatabase.cpp',
    'types.cpp',
    'descriptor.cpp',
    'ffs.cpp',
    'nvram.cpp',
    'nvramparser.cpp',
    'meparser.cpp',
    'ffsparser.cpp',
    'ffsreport.cpp',
    'peimage.cpp',
    'treeitem.cpp',
    'treemodel.cpp',
    'utility.cpp',
    'ustring.cpp',
    'sha256.c',
  ],
  c_args: [
    '-DU_ENABLE_NVRAM_PARSING_SUPPORT',
    '-DU_ENABLE_ME_PARSING_SUPPORT',
    '-DU_ENABLE_FIT_PARSING_SUPPORT',
    '-DU_ENABLE_GUID_DATABASE_SUPPORT',
  ],
)
