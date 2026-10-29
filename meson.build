project(
  'gambatte-k2',
  ['c', 'cpp'],
  version: '2.0.0',
  default_options: [
    'c_std=gnu99',
    'cpp_std=c++11',
    'warning_level=1',
    'optimization=2',
    'b_ndebug=if-release'
  ],
  meson_version: '>=0.56'
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

gtk_dep = dependency('gtk+-2.0', required: true)
threads_dep = dependency('threads')
m_dep = cc.find_library('m', required: false)

xcb_dep = dependency('xcb', required: false)
if not xcb_dep.found()
  xcb_dep = cc.find_library('xcb', required: true)
endif

xcb_shm_dep = dependency('xcb-shm', required: false)
if not xcb_shm_dep.found()
  xcb_shm_dep = cc.find_library('xcb-shm', required: true)
endif

deps = [
  gtk_dep,
  threads_dep,
  m_dep,
  xcb_dep,
  xcb_shm_dep
]

# Libretro core: link libgambatte_libretro.a (statically built archive)
core_dep = cc.find_library('gambatte_libretro',
  dirs: [
    meson.current_source_dir() / 'cores',
    meson.current_source_dir()
  ],
  required: false
)
if core_dep.found()
  deps += core_dep
endif

sources = files(
  'gambatte-k2.c',
  'file_picker.c',
  'gray_shm.c'
)

c_args = [
  '-Wno-deprecated-declarations',
  '-Wno-unused-variable',
  '-Wno-unused-function',
  '-Wno-incompatible-pointer-types'
]

executable(
  'gambatte-k2',
  sources,
  include_directories: [include_directories('.'), include_directories('include')],
  dependencies: deps,
  c_args: c_args,
  link_args: ['-lstdc++'],
  export_dynamic: true,
  install: true
)
