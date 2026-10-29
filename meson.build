project(
  'gambatte-k2',
  'c',
  version: '2.0.0',
  default_options: [
    'c_std=gnu99',
    'warning_level=1',
    'optimization=2',
    'b_ndebug=if-release'
  ],
  meson_version: '>=0.56'
)

cc = meson.get_compiler('c')

gtk_dep = dependency('gtk+-2.0', required: true)
threads_dep = dependency('threads')
m_dep = cc.find_library('m', required: false)

gthread_dep = dependency('gthread-2.0', required: false)
if not gthread_dep.found()
  gthread_dep = cc.find_library('gthread-2.0', required: true)
endif

deps = [
  gtk_dep,
  threads_dep,
  m_dep,
  gthread_dep
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
  'file_picker.c'
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
  link_args: [
    '-L' + meson.current_source_dir() / 'cores',
    '-L' + meson.current_source_dir(),
    '-lgambatte_libretro',
    '-lstdc++'
  ],
  export_dynamic: true,
  install: true
)
