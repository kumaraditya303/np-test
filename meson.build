project(
  'datetime_metadata',
  'c',
  version : '0.1.0',
  meson_version : '>=1.3.0',
  default_options : ['warning_level=2', 'c_std=c11'],
)

py = import('python').find_installation(pure : false)

# NumPy's C headers (numpy.get_include()).
numpy_include = run_command(
  py,
  ['-c', 'import numpy; print(numpy.get_include())'],
  check : true,
).stdout().strip()

# Use a raw -I include arg rather than include_directories(): the numpy
# headers live inside the in-tree .venv, which include_directories() rejects.
numpy_dep = declare_dependency(compile_args : ['-I' + numpy_include])

# The same source builds two ways, selected by -Dlimited_api:
#   true  (default): Limited C API / stable ABI (abi3) — one binary for 3.12+.
#   false          : full, version-specific CPython API — used for the
#                    free-threaded 3.14t build (abi3 is not used there).
ext_kwargs = {
  'dependencies' : [numpy_dep],
  'install' : true,
}
if get_option('limited_api')
  ext_kwargs += {'limited_api' : '3.12'}
  message('Building datetime_metadata against the Limited C API (abi3)')
else
  message('Building datetime_metadata against the full CPython API')
endif

py.extension_module(
  'datetime_metadata',
  'src/datetime_metadata.c',
  kwargs : ext_kwargs,
)
