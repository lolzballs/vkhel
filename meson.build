project('vkhel', 'c')

dep_vulkan = dependency('vulkan', required: true)

sources = files([
  'src/vkhel.c',
])

subdir('src/shaders')

lib_vkhel = library(
  meson.project_name(),
  sources,
  include_directories: 'include',
  dependencies: [
    dep_vulkan,
  ],
)
