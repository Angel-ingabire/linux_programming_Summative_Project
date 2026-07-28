/* =============================================================================
 * Project 3: Developing a Python C Extension for High-Performance Data Processing
 * =============================================================================
 * File: sensor_analysis.c
 * Description: Python C extension module that performs statistical operations
 *              on sensor data directly in C for improved performance.
 *
 * Module Name: sensor_analysis
 *
 * Functions:
 *   - average(data)     : Arithmetic mean of sensor readings
 *   - range_value(data) : Difference between max and min values
 *   - variance(data)    : Sample variance of readings
 *   - count_above(data, limit) : Count of readings > limit
 *   - statistics(data)  : Returns dict with samples, average, min, max
 *
 * Memory Management:
 *   No dynamic memory allocation is required because:
 *   1. Input data is accessed directly from the Python list/tuple via
 *      PyList_GetItem / PyTuple_GetItem which return borrowed references.
 *   2. Output values are returned as Python objects (float, int, dict)
 *      created by API functions like PyFloat_FromDouble, Py_BuildValue.
 *   3. All calculations use stack-allocated C variables.
 *
 * Time Complexity:
 *   - average()    : O(n) - single pass sum + division
 *   - range_value(): O(n) - single pass find min/max
 *   - variance()   : O(n) - single pass for mean, O(n) for variance = O(n)
 *   - count_above(): O(n) - single pass comparison
 *   - statistics() : O(n) - single pass for all stats
 *
 * Numerical Accuracy Considerations:
 *   - Uses double (64-bit floating point) for all calculations.
 *   - Two-pass variance calculation provides better numerical stability
 *     than the one-pass algorithm for large datasets.
 *   - Kahan summation algorithm could be added for extreme precision,
 *     but introduces overhead. Standard double precision is sufficient
 *     for IoT sensor data (typically 2-4 significant decimal digits).
 * =============================================================================
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>
#include <float.h>

/* ===========================================================================
 * Helper: validate_and_extract
 * ---------------------------------------------------------------------------
 * Purpose: Validates the input Python object is a sequence (list or tuple)
 *          of numeric values and extracts them into a C double array.
 *
 * Python Object Access:
 *   - Uses PySequence_Check() to verify the input is a sequence type.
 *   - Uses PySequence_Length() to get the number of elements.
 *   - Uses PySequence_GetItem() to retrieve each element (returns new ref).
 *   - Uses PyNumber_Check() to verify each element is numeric.
 *   - Uses PyFloat_AsDouble() to convert each element to C double.
 *
 * Memory Management:
 *   - The returned C array is heap-allocated (calloc) and must be freed
 *     by the caller. This is the only place dynamic allocation is needed,
 *     and it's the minimal amount necessary to efficiently process the data.
 *
 * Edge Cases:
 *   - Empty sequence: handled by returning NULL with a ValueError.
 *   - Non-numeric elements: handled by raising a TypeError.
 * ===========================================================================
 */
static double *validate_and_extract(PyObject *data, Py_ssize_t *size)
{
    /* Validate that input is a sequence (list or tuple) */
    if (!PySequence_Check(data))
    {
        PyErr_SetString(PyExc_TypeError,
                        "Input must be a list or tuple of numeric values");
        return NULL;
    }

    /* Get the length of the sequence */
    *size = PySequence_Length(data);
    if (*size == 0)
    {
        PyErr_SetString(PyExc_ValueError,
                        "Input data cannot be empty");
        return NULL;
    }

    /* Allocate C array to hold the double values */
    double *values = (double *)calloc(*size, sizeof(double));
    if (!values)
    {
        PyErr_SetString(PyExc_MemoryError,
                        "Failed to allocate memory for data processing");
        return NULL;
    }

    /* Extract each element, converting from Python object to C double */
    for (Py_ssize_t i = 0; i < *size; i++)
    {
        PyObject *item = PySequence_GetItem(data, i); /* New reference */
        if (!item)
        {
            free(values);
            return NULL; /* PySequence_GetItem already set an exception */
        }

        /* Validate that the item is a number */
        if (!PyNumber_Check(item))
        {
            PyErr_SetString(PyExc_TypeError,
                            "All elements must be numeric values");
            Py_DECREF(item);
            free(values);
            return NULL;
        }

        /* Convert to C double */
        values[i] = PyFloat_AsDouble(item);
        Py_DECREF(item); /* Release the borrowed reference */

        /* Check for conversion error */
        if (PyErr_Occurred())
        {
            free(values);
            return NULL;
        }
    }

    return values;
}

/* ===========================================================================
 * Function: average(data)
 * ---------------------------------------------------------------------------
 * Purpose: Calculates the arithmetic mean of sensor readings.
 *
 * Formula: mean = (1/n) * sum(x_i) for i = 1 to n
 *
 * Time Complexity: O(n) - single pass through the data
 * Numerical Accuracy: Standard double precision; sum accumulates in a
 *                     double for sufficient precision with sensor data.
 *
 * Python API Usage:
 *   - Parses arguments using PyArg_ParseTuple
 *   - Returns result as Python float via PyFloat_FromDouble
 * ===========================================================================
 */
static PyObject *sensor_average(PyObject *self, PyObject *args)
{
    PyObject *data;
    Py_ssize_t size;
    double *values;

    /* Parse Python arguments: expects a single sequence argument */
    if (!PyArg_ParseTuple(args, "O", &data))
    {
        return NULL;
    }

    /* Validate and extract data into C array */
    values = validate_and_extract(data, &size);
    if (!values)
    {
        return NULL;
    }

    /* Calculate sum */
    double sum = 0.0;
    for (Py_ssize_t i = 0; i < size; i++)
    {
        sum += values[i];
    }

    free(values); /* Release dynamically allocated memory */

    /* Return arithmetic mean as Python float */
    return PyFloat_FromDouble(sum / (double)size);
}

/* ===========================================================================
 * Function: range_value(data)
 * ---------------------------------------------------------------------------
 * Purpose: Returns the range (max - min) of the dataset.
 *
 * Algorithm:
 *   - Initialize min to DBL_MAX and max to -DBL_MAX
 *   - Single pass: compare each value against current min/max
 *
 * Time Complexity: O(n) - single pass, constant space
 *
 * Edge Cases:
 *   - Single element: returns 0.0 (max == min)
 * ===========================================================================
 */
static PyObject *sensor_range(PyObject *self, PyObject *args)
{
    PyObject *data;
    Py_ssize_t size;
    double *values;

    if (!PyArg_ParseTuple(args, "O", &data))
    {
        return NULL;
    }

    values = validate_and_extract(data, &size);
    if (!values)
    {
        return NULL;
    }

    /* Initialize min and max to extreme values */
    double min_val = DBL_MAX;
    double max_val = -DBL_MAX;

    /* Single pass: find both min and max simultaneously */
    for (Py_ssize_t i = 0; i < size; i++)
    {
        if (values[i] < min_val)
            min_val = values[i];
        if (values[i] > max_val)
            max_val = values[i];
    }

    free(values);

    /* Range = difference between max and min */
    return PyFloat_FromDouble(max_val - min_val);
}

/* ===========================================================================
 * Function: variance(data)
 * ---------------------------------------------------------------------------
 * Purpose: Returns the sample variance of the readings.
 *
 * Formula (two-pass for numerical stability):
 *   mean = (1/n) * sum(x_i)
 *   variance = (1/(n-1)) * sum((x_i - mean)^2)
 *
 * Why two-pass?
 *   - The two-pass approach is more numerically stable than one-pass
 *     algorithms, especially for datasets with values close together.
 *   - For sensor data with typical variance, standard double precision
 *     is more than adequate for either approach.
 *   - We use sample variance (divide by n-1) for unbiased estimation.
 *
 * Time Complexity: O(n) - first pass for mean, second pass for variance
 * ===========================================================================
 */
static PyObject *sensor_variance(PyObject *self, PyObject *args)
{
    PyObject *data;
    Py_ssize_t size;
    double *values;

    if (!PyArg_ParseTuple(args, "O", &data))
    {
        return NULL;
    }

    values = validate_and_extract(data, &size);
    if (!values)
    {
        return NULL;
    }

    if (size == 1)
    {
        free(values);
        /* Sample variance of a single element is 0 (or undefined; return 0) */
        return PyFloat_FromDouble(0.0);
    }

    /* Pass 1: Calculate arithmetic mean */
    double sum = 0.0;
    for (Py_ssize_t i = 0; i < size; i++)
    {
        sum += values[i];
    }
    double mean = sum / (double)size;

    /* Pass 2: Calculate sum of squared differences from mean */
    double sum_sq_diff = 0.0;
    for (Py_ssize_t i = 0; i < size; i++)
    {
        double diff = values[i] - mean;
        sum_sq_diff += diff * diff; /* (x_i - mean)^2 */
    }

    free(values);

    /* Sample variance: divide by (n-1) for unbiased estimate */
    return PyFloat_FromDouble(sum_sq_diff / (double)(size - 1));
}

/* ===========================================================================
 * Function: count_above(data, limit)
 * ---------------------------------------------------------------------------
 * Purpose: Counts the number of readings greater than the specified limit.
 *
 * Algorithm: Single pass comparison against the threshold value.
 *
 * Time Complexity: O(n) - single pass, no extra space
 *
 * Python API Usage:
 *   - Parses two arguments: sequence (O) and limit (d for double)
 * ===========================================================================
 */
static PyObject *sensor_count_above(PyObject *self, PyObject *args)
{
    PyObject *data;
    double limit;
    Py_ssize_t size;
    double *values;

    /* Parse arguments: sequence and a double limit value */
    if (!PyArg_ParseTuple(args, "Od", &data, &limit))
    {
        return NULL;
    }

    values = validate_and_extract(data, &size);
    if (!values)
    {
        return NULL;
    }

    /* Count elements strictly greater than the limit */
    Py_ssize_t count = 0;
    for (Py_ssize_t i = 0; i < size; i++)
    {
        if (values[i] > limit)
        {
            count++;
        }
    }

    free(values);

    /* Return count as Python integer */
    return PyLong_FromSsize_t(count);
}

/* ===========================================================================
 * Function: statistics(data)
 * ---------------------------------------------------------------------------
 * Purpose: Returns a comprehensive statistics dictionary containing:
 *   - "samples":  number of data points
 *   - "average":  arithmetic mean
 *   - "minimum":  smallest value
 *   - "maximum":  largest value
 *
 * Algorithm: Single pass through the data to compute all statistics.
 *   This is more efficient than calling individual functions because
 *   we only traverse the data once.
 *
 * Time Complexity: O(n) - single pass
 *
 * Python API Usage:
 *   - Uses Py_BuildValue to create and return a Python dictionary
 *     with formatted key-value pairs.
 * ===========================================================================
 */
static PyObject *sensor_statistics(PyObject *self, PyObject *args)
{
    PyObject *data;
    Py_ssize_t size;
    double *values;

    if (!PyArg_ParseTuple(args, "O", &data))
    {
        return NULL;
    }

    values = validate_and_extract(data, &size);
    if (!values)
    {
        return NULL;
    }

    /* Single pass: compute sum, min, and max simultaneously */
    double sum = 0.0;
    double min_val = DBL_MAX;
    double max_val = -DBL_MAX;

    for (Py_ssize_t i = 0; i < size; i++)
    {
        double v = values[i];
        sum += v;
        if (v < min_val)
            min_val = v;
        if (v > max_val)
            max_val = v;
    }

    free(values);

    double average = sum / (double)size;

    /* Build and return a Python dictionary with all statistics */
    /* Py_BuildValue creates Python objects from C values using format strings:
     *   "s"  = C string -> Python str
     *   "i"  = C int    -> Python int
     *   "d"  = C double -> Python float
     */
    return Py_BuildValue("{s:i, s:d, s:d, s:d}",
                         "samples", (int)size,
                         "average", average,
                         "minimum", min_val,
                         "maximum", max_val);
}

/* ===========================================================================
 * Method Table Definition
 * ---------------------------------------------------------------------------
 * This structure maps Python function names to their C implementations.
 * Each entry includes:
 *   - name:      Python-visible function name
 *   - func:      C function pointer (PyCFunction)
 *   - flags:     Calling convention (METH_VARARGS = standard args)
 *   - doc:       Documentation string
 *
 * METH_VARARGS indicates the function accepts positional arguments
 * passed as a PyObject* tuple to the C function.
 * ===========================================================================
 */
static PyMethodDef SensorMethods[] = {
    {"average", sensor_average, METH_VARARGS,
     "Calculate the arithmetic mean of sensor readings.\n"
     "Args: data (list/tuple of numeric values)\n"
     "Returns: float - the mean value"},
    {"range_value", sensor_range, METH_VARARGS,
     "Calculate the range (max - min) of sensor readings.\n"
     "Args: data (list/tuple of numeric values)\n"
     "Returns: float - the range value"},
    {"variance", sensor_variance, METH_VARARGS,
     "Calculate the sample variance of sensor readings.\n"
     "Args: data (list/tuple of numeric values)\n"
     "Returns: float - the sample variance"},
    {"count_above", sensor_count_above, METH_VARARGS,
     "Count readings above a specified limit.\n"
     "Args: data (list/tuple), limit (float)\n"
     "Returns: int - number of readings above the limit"},
    {"statistics", sensor_statistics, METH_VARARGS,
     "Compute comprehensive statistics for sensor readings.\n"
     "Args: data (list/tuple of numeric values)\n"
     "Returns: dict with keys: samples, average, minimum, maximum"},
    {NULL, NULL, 0, NULL} /* Sentinel: marks end of method table */
};

/* ===========================================================================
 * Module Definition Structure
 * ---------------------------------------------------------------------------
 * Defines the module metadata including name, documentation, methods,
 * and version information. This structure is used by Python's import
 * system to initialize the module.
 * ===========================================================================
 */
static struct PyModuleDef sensor_analysis_module = {
    PyModuleDef_HEAD_INIT, /* Standard header */
    "sensor_analysis",     /* Module name (internal) */
    "High-performance sensor data analysis module\n"
    "Provides statistical functions implemented in C for speed.\n"
    "Functions: average, range_value, variance, count_above, statistics",
    -1,           /* Per-interpreter state (-1 = global) */
    SensorMethods /* Method table pointer */
};

/* ===========================================================================
 * Module Initialization Function
 * ---------------------------------------------------------------------------
 * This is the entry point Python calls when importing the module.
 * Must be named PyInit_<module_name> (case-sensitive).
 *
 * Returns: PyObject* (the initialized module) or NULL on failure.
 * ===========================================================================
 */
PyMODINIT_FUNC PyInit_sensor_analysis(void)
{
    /* Create the module using the definition structure */
    return PyModule_Create(&sensor_analysis_module);
}
