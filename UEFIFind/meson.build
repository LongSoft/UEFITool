executable(
  'UEFIFind',
  sources: [
    'uefifind_main.cpp',
    'uefifind.cpp',
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
