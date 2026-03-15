test_sources = files(
  'test_utility.cpp',
  'test_ffs.cpp',
  'test_types.cpp',
  'test_descriptor.cpp',
  'catch2/catch_amalgamated.cpp',
)

uefitool_tests = executable('uefitool_tests',
  sources: test_sources,
  include_directories: include_directories('catch2', '..'),
  link_with: [uefitoolcommon, lzma, bstrlib, brotli],
  dependencies: [zlib],
  cpp_args: [
    '-DU_ENABLE_NVRAM_PARSING_SUPPORT',
    '-DU_ENABLE_ME_PARSING_SUPPORT',
    '-DU_ENABLE_FIT_PARSING_SUPPORT',
    '-DU_ENABLE_GUID_DATABASE_SUPPORT',
  ],
  # Note: override_options requires Meson >= 0.51. The root meson.build
  # declares >=0.45.0 but C++14 override is needed for Catch2 v3.
  override_options: ['cpp_std=c++14'],
)

test('uefitool_tests', uefitool_tests)
