cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

numbers = executable('numbers',
  'numbers.c',
  dependencies: [vkhel_priv, m_dep])
test('numbers', numbers)

ntt = executable('ntt',
  'ntt.c',
  dependencies: vkhel_priv)
test('ntt', ntt)

vector = executable('vector',
  'vector.c',
  dependencies: vkhel_priv)
test('vector', vector)
