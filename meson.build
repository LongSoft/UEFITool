project('UEFITool', ['c', 'cpp'],
  version: 'A62',
  license: 'BSD-2-Clause',
  meson_version: '>=0.45.0',
  default_options : ['c_std=c11', 'cpp_std=c++11'],
)

zlib = dependency('zlib')

subdir('common')
subdir('UEFIExtract')
subdir('UEFIFind')
