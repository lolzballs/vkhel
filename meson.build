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

lib_vkhel = library(
  meson.project_name(),
  sources,
  include_directories: vkhel_incs,
  dependencies: vkhel_deps,
)

vkhel = declare_dependency(
  link_with: lib_vkhel,
  dependencies: vkhel_deps,
  include_directories: vkhel_incs,
)

subdir('examples')
subdir('test')
