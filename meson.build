project('vkhel', 'c')

dep_vulkan = dependency('vulkan', required: true)

sources = files([
  'src/kernels/elemfma.c',
  'src/kernels/elemmodbytwo.c',
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

if get_option('debug')
  add_project_arguments('-DVKHEL_DEBUG', language: 'c')
endif

subdir('src/kernels/shaders')

vma_proj = subproject('vulkan-memory-allocator')
vma_dep = vma_proj.get_variable('dep')

vkhel_deps = [dep_vulkan, vma_dep]
vkhel_incs = include_directories('include/vkhel')
vkhel_priv_incs = [vkhel_incs, include_directories('include')]
install_headers('include/vkhel/vkhel.h')

lib_vkhel_priv = static_library(
  meson.project_name() + '_priv',
  sources,
  include_directories: vkhel_priv_incs,
  dependencies: vkhel_deps,
)

symbols_file = 'vkhel.syms'
lib_vkhel = library(
  meson.project_name(),
  include_directories: [vkhel_incs],
  dependencies: vkhel_deps,
  link_whole: lib_vkhel_priv,
  link_args: '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), symbols_file),
  link_depends: symbols_file,
  install: true,
)

vkhel_priv = declare_dependency(
  link_with: lib_vkhel_priv,
  dependencies: vkhel_deps,
  include_directories: vkhel_priv_incs,
)

vkhel = declare_dependency(
  link_with: lib_vkhel,
  dependencies: vkhel_deps,
  include_directories: vkhel_incs,
)

subdir('examples')
subdir('test')

pkg = import('pkgconfig')
pkg.generate(lib_vkhel)
