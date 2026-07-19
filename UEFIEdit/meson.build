executable(
  'UEFIEdit',
  sources: [
    'uefiedit_main.cpp',
    'uefiedit.cpp',
    '../common/ffsops.cpp',
    '../common/ffsbuilder.cpp',
    # Compression routines needed by FfsBuilder
    '../common/LZMA/LzmaCompress.c',
    '../common/LZMA/SDK/C/CpuArch.c',
    '../common/LZMA/SDK/C/LzFind.c',
    '../common/LZMA/SDK/C/LzmaEnc.c',
    '../common/Tiano/EfiTianoCompress.c',
    '../common/Tiano/EfiTianoCompressLegacy.c',
  ],
  link_with: [
    lzma,
    brotli,
    bstrlib,
    uefitoolcommon,
  ],
  dependencies: [
    zlib,
  ],
  install: true,
)