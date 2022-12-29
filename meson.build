project('vkhel', 'c')

dep_vulkan = dependency('vulkan', required: true)

sources = files([
  'src/vector.c',
  'src/vkhel.c',
  'src/vulkan.c',
])

subdir('src/shaders')

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
