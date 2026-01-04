executable(
  'UEFIExtract',
  sources: [
    'uefiextract_main.cpp',
    'ffsdumper.cpp',
    'uefidump.cpp',
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
