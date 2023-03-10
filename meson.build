project('vkhel', 'c')

dep_vulkan = dependency('vulkan', required: true)

sources = files([
  'src/kernels/elemfma.c',
  'src/kernels/elemmul.c',
  'src/kernels/elemmulconst.c',
  'src/kernels/elemgtadd.c',
  'src/kernels/elemgtsub.c',
  'src/kernels/nttfwdbutterfly.c',
  'src/kernels/nttrevbutterfly.c',
  'src/ntt_tables.c',
  'src/numbers.c',
  'src/vector.c',
  'src/vkhel.c',
  'src/vulkan.c',
])

subdir('src/kernels/shaders')

vkhel_deps = [dep_vulkan]
vkhel_incs = include_directories('include')

lib_vkhel_priv = static_library(
  meson.project_name() + '_priv',
  sources,
  include_directories: vkhel_incs,
  dependencies: vkhel_deps,
)

symbols_file = 'vkhel.syms'
lib_vkhel = library(
  meson.project_name(),
  dependencies: vkhel_deps,
  link_whole: lib_vkhel_priv,
  link_args: '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), symbols_file),
  link_depends: symbols_file,
)

vkhel_priv = declare_dependency(
  link_with: lib_vkhel_priv,
  dependencies: vkhel_deps,
  include_directories: vkhel_incs,
)

vkhel = declare_dependency(
  link_with: lib_vkhel,
  dependencies: vkhel_deps,
  include_directories: vkhel_incs,
)

subdir('examples')
subdir('test')
