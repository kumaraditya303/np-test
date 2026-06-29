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

py.extension_module(
  'datetime_metadata',
  'src/datetime_metadata.c',
  dependencies : [numpy_dep],
  limited_api : '3.12',
  install : true,
)
