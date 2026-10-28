project(
  'gambatte-k2',
  'c',
  version: '1.2.0',
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

gmodule_dep = dependency('gmodule-2.0', required: false)
if not gmodule_dep.found()
  gmodule_dep = dependency('gmodule-no-export-2.0', required: false)
  if not gmodule_dep.found()
    gmodule_dep = cc.find_library('gmodule-2.0', required: true)
  endif
endif

deps = [
  gtk_dep,
  threads_dep,
  m_dep,
  gthread_dep,
  gmodule_dep
]

sources = files(
  'main.c',
  'retrocore.c',
  'gamejoypad.c',
  'file_picker.c',
  'fbink.c',
  'gray_shm.c',
  'kindle_compat.c'
)

c_args = [
  '-Wno-deprecated-declarations',
  '-Wno-unused-variable',
  '-Wno-unused-function',
  '-Wno-incompatible-pointer-types',
  '-Wno-int-conversion'
]

executable(
  'gambatte-k2',
  sources,
  include_directories: [include_directories('.'), include_directories('include')],
  dependencies: deps,
  c_args: c_args,
  install: true
)
