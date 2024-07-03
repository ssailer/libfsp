project('libfsp', 'c','cpp',
  version:'1.0.0',
  license:'MPL-2.0',
  license_files: 'LICENSE',
   default_options : [
    'warning_level=3',
    'c_std=c11',
    'buildtype=release',
    'default_library=static',
    'prefer_static=true',
   ]
)

# get systemlevel dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : true)

fcio_dep = dependency('fcio', fallback : ['fcio', 'fcio_dep'])

fsp_inc = include_directories('src')
fsp_sources = files(
  'src/fsp/buffer.c',
  'src/fsp/dsp.c',
  'src/fsp/stats.c',
  'src/fsp/timestamps.c',
  'src/fsp/io.c',
  'src/fsp/record_processor.c',
  'src/fsp/processor.c',
  'src/fsp_l200.c',
)

fsp_lib = library('fsp',
  fsp_sources,
  include_directories : fsp_inc,
  dependencies: [fcio_dep, m_dep],
  install : true
)

fsp_dep = declare_dependency(include_directories : fsp_inc, link_with : fsp_lib, sources : fsp_sources, dependencies : [fcio_dep, m_dep])

fsp_io_sources = files(
  'src/fsp/io.c'
)
fsp_io_dep = declare_dependency(include_directories : fsp_inc, sources : fsp_io_sources, dependencies : [fcio_dep, m_dep])

install_headers(
  'src/fsp/record_processor.h',
  'src/fsp/dsp.h',
  'src/fsp/state.h',
  'src/fsp/stats.h',
  'src/fsp/flags.h',
  'src/fsp/obs.h',
  'src/fsp/io.h',
  'src/fsp_l200.h',
  'src/fsp_channelmaps.h',
  'src/fsp.h',
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(fsp_lib)
