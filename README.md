# datetime_metadata

A small CPython extension built against the **Limited C API** (`Py_LIMITED_API`,
stable ABI 3.12) that exercises NumPy's `datetime64` / `timedelta64` unit
metadata (`PyArray_DatetimeMetaData`).

## Build

```sh
uv venv
uv pip install numpy meson meson-python ninja
uv pip install -e . --no-build-isolation   # editable install via meson-python
```

Or compile directly with meson:

```sh
meson setup build
meson compile -C build
```

## Use

```python
import datetime_metadata as dm
dm.self_test()                       # -> 10 (number of checks)
dm.get_metadata("datetime64[10ms]")  # -> {'unit': 'ms', 'base': 8, 'num': 10}
```
