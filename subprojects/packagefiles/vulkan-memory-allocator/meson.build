project('VulkanMemoryAllocator', 'cpp', version: '3.0.1')

dep_vulkan = dependency('vulkan', required: true)
inc_dirs = include_directories('include')

lib = static_library(
  meson.project_name(),
  'vk_mem_alloc.cpp',
  include_directories: inc_dirs,
  dependencies: [dep_vulkan],
)

dep = declare_dependency(
  link_with: lib,
  include_directories: inc_dirs,
  dependencies: [dep_vulkan],
)
