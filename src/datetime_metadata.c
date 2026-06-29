/*
 * datetime_metadata
 * -----------------
 * A tiny CPython extension built against the *Limited C API* (Py_LIMITED_API)
 * that exercises NumPy's datetime64 / timedelta64 unit-metadata C API.
 *
 * It reads the `PyArray_DatetimeMetaData` carried by a datetime/timedelta
 * dtype's `c_metadata` slot and exposes it to Python, and provides a small
 * self-test that round-trips a handful of unit strings through NumPy.
 */

/* Target the 3.12 stable ABI. Must come before any CPython header.
 * Meson may also pass this via the `limited_api` kwarg, so guard it. */
#ifndef Py_LIMITED_API
#  define Py_LIMITED_API 0x030C0000
#endif

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* Ask for the modern (non-deprecated) NumPy C API. */
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL datetime_metadata_ARRAY_API
#include <numpy/arrayobject.h>
#include <numpy/ndarraytypes.h>

/* Map an NPY_DATETIMEUNIT enum value to its canonical NumPy string code. */
static const char *
unit_to_str(NPY_DATETIMEUNIT base)
{
    switch (base) {
        case NPY_FR_Y:       return "Y";
        case NPY_FR_M:       return "M";
        case NPY_FR_W:       return "W";
        case NPY_FR_D:       return "D";
        case NPY_FR_h:       return "h";
        case NPY_FR_m:       return "m";
        case NPY_FR_s:       return "s";
        case NPY_FR_ms:      return "ms";
        case NPY_FR_us:      return "us";
        case NPY_FR_ns:      return "ns";
        case NPY_FR_ps:      return "ps";
        case NPY_FR_fs:      return "fs";
        case NPY_FR_as:      return "as";
        case NPY_FR_GENERIC: return "generic";
        default:             return "error";
    }
}

/*
 * Pull the PyArray_DatetimeMetaData out of a datetime64/timedelta64 dtype.
 * Returns 0 on success and fills *out; returns -1 with an exception set
 * if `descr` is not a datetime-like dtype.
 */
static int
get_meta(PyArray_Descr *descr, PyArray_DatetimeMetaData *out)
{
    char type = PyDataType_TYPE(descr);
    if (type != 'M' && type != 'm') {
        PyErr_SetString(PyExc_TypeError,
                        "expected a datetime64 or timedelta64 dtype");
        return -1;
    }

    /* For datetime-like dtypes c_metadata points at a
     * PyArray_DatetimeDTypeMetaData whose .meta field is what we want. */
    PyArray_DatetimeDTypeMetaData *dt_meta =
        (PyArray_DatetimeDTypeMetaData *)PyDataType_C_METADATA(descr);
    if (dt_meta == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dtype is missing datetime c_metadata");
        return -1;
    }

    *out = dt_meta->meta;
    return 0;
}

/*
 * get_metadata(dtype) -> dict
 *
 * Accepts anything convertible to a NumPy dtype (a dtype object, a type, or a
 * string such as "datetime64[10ms]") and returns its unit metadata.
 */
static PyObject *
get_metadata(PyObject *Py_UNUSED(self), PyObject *arg)
{
    PyArray_Descr *descr = NULL;
    if (!PyArray_DescrConverter(arg, &descr)) {
        return NULL;
    }

    PyArray_DatetimeMetaData meta;
    if (get_meta(descr, &meta) < 0) {
        Py_DECREF(descr);
        return NULL;
    }
    Py_DECREF(descr);

    return Py_BuildValue("{s:s,s:i,s:i}",
                         "unit", unit_to_str(meta.base),
                         "base", (int)meta.base,
                         "num", meta.num);
}

/*
 * self_test() -> int
 *
 * Round-trips a set of unit specifications through np.dtype() and verifies
 * that the C-side metadata matches expectations. Returns the number of
 * checks performed, or raises AssertionError on the first mismatch.
 */
static int
check_one(const char *spec, NPY_DATETIMEUNIT expect_base, int expect_num)
{
    PyArray_Descr *descr = NULL;
    PyObject *spec_obj = PyUnicode_FromString(spec);
    if (spec_obj == NULL) {
        return -1;
    }
    int ok = PyArray_DescrConverter(spec_obj, &descr);
    Py_DECREF(spec_obj);
    if (!ok) {
        return -1;
    }

    PyArray_DatetimeMetaData meta;
    if (get_meta(descr, &meta) < 0) {
        Py_DECREF(descr);
        return -1;
    }
    Py_DECREF(descr);

    if (meta.base != expect_base || meta.num != expect_num) {
        PyErr_Format(PyExc_AssertionError,
                     "%s: got (base=%d, num=%d), expected (base=%d, num=%d)",
                     spec, (int)meta.base, meta.num,
                     (int)expect_base, expect_num);
        return -1;
    }
    return 0;
}

static PyObject *
self_test(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    struct { const char *spec; NPY_DATETIMEUNIT base; int num; } cases[] = {
        {"datetime64[Y]",     NPY_FR_Y,       1},
        {"datetime64[M]",     NPY_FR_M,       1},
        {"datetime64[D]",     NPY_FR_D,       1},
        {"datetime64[h]",     NPY_FR_h,       1},
        {"datetime64[10ms]",  NPY_FR_ms,     10},
        {"datetime64[us]",    NPY_FR_us,      1},
        {"datetime64[5ns]",   NPY_FR_ns,      5},
        {"timedelta64[s]",    NPY_FR_s,       1},
        {"timedelta64[7D]",   NPY_FR_D,       7},
        {"datetime64",        NPY_FR_GENERIC, 1},
    };
    int n = (int)(sizeof(cases) / sizeof(cases[0]));
    for (int i = 0; i < n; i++) {
        if (check_one(cases[i].spec, cases[i].base, cases[i].num) < 0) {
            return NULL;
        }
    }
    return PyLong_FromLong(n);
}

static PyMethodDef methods[] = {
    {"get_metadata", get_metadata, METH_O,
     "get_metadata(dtype) -> dict with 'unit', 'base', 'num' for a "
     "datetime64/timedelta64 dtype."},
    {"self_test", self_test, METH_NOARGS,
     "Round-trip a set of unit specs through NumPy and verify the C-side "
     "metadata. Returns the number of checks performed."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "datetime_metadata",
    "Limited C API + NumPy datetime64 unit-metadata demo/test.",
    -1,
    methods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_datetime_metadata(void)
{
    PyObject *m = PyModule_Create(&moduledef);
    if (m == NULL) {
        return NULL;
    }
    /* Initialise the NumPy C API table. Returns NULL on failure. */
    import_array();
    if (PyErr_Occurred()) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
