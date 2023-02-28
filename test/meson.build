cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

numbers = executable('numbers',
  'numbers.c',
  dependencies: [vkhel, m_dep])
test('numbers', numbers)

ntt = executable('ntt',
  'ntt.c',
  dependencies: vkhel)
test('ntt', ntt)

vector = executable('vector',
  'vector.c',
  dependencies: vkhel)
test('vector', vector)
