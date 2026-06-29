"""Tests for the datetime_metadata C extension (Limited C API + NumPy)."""

import numpy as np

import datetime_metadata as dm


def test_self_test_runs():
    # The C-side self test returns the number of checks it performed.
    assert dm.self_test() == 10


def test_from_string():
    meta = dm.get_metadata("datetime64[10ms]")
    assert meta == {"unit": "ms", "base": 8, "num": 10}


def test_from_dtype_object():
    meta = dm.get_metadata(np.dtype("datetime64[ns]"))
    assert meta["unit"] == "ns"
    assert meta["num"] == 1


def test_timedelta():
    meta = dm.get_metadata(np.dtype("timedelta64[7D]"))
    assert meta["unit"] == "D"
    assert meta["num"] == 7


def test_generic_unit():
    meta = dm.get_metadata(np.dtype("datetime64"))
    assert meta["unit"] == "generic"


def test_rejects_non_datetime():
    try:
        dm.get_metadata(np.dtype("float64"))
    except TypeError:
        pass
    else:
        raise AssertionError("expected TypeError for non-datetime dtype")


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_") and callable(fn):
            fn()
            print(f"ok  {name}")
    print("All tests passed.")
