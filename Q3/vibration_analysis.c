/*
 * vibration_analysis.c - Python C Extension for Real-time Vibration Analysis
 * 
 * Provides efficient statistical and signal analysis functions for vibration data
 * All computations are implemented in C for maximum performance
 */

#include <Python.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
 * Helper Function: Convert Python sequence to C double array
 * Validates input, extracts data, returns pointer to stack array
 * Returns 1 on success, 0 on failure (with Python exception set)
 * ============================================================ */
static int convert_to_doubles(PyObject *data, double **arr, int *len) {
    PyObject *iter;
    PyObject *item;
    int size;
    int i;
    double *temp;
    
    // Check if data is a list or tuple
    if (!PyList_Check(data) && !PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, 
            "Input must be a list or tuple of floats");
        return 0;
    }
    
    // Get length
    size = PySequence_Size(data);
    if (size < 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot get sequence size");
        return 0;
    }
    
    *len = size;
    
    // Handle empty input
    if (size == 0) {
        *arr = NULL;
        return 1;
    }
    
    // Allocate temporary array on stack (caller provides buffer)
    // Note: For large arrays, this could cause stack overflow
    // For production, use heap allocation
    temp = (double *)malloc(size * sizeof(double));
    if (!temp) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
        return 0;
    }
    
    // Extract each element and validate type
    for (i = 0; i < size; i++) {
        item = PySequence_GetItem(data, i);
        if (!item) {
            free(temp);
            return 0;
        }
        
        // Convert to float (handles int, float, etc.)
        temp[i] = PyFloat_AsDouble(item);
        Py_DECREF(item);
        
        // Check conversion error
        if (PyErr_Occurred()) {
            free(temp);
            return 0;
        }
    }
    
    *arr = temp;
    return 1;
}

/* ============================================================
 * Function 1: peak_to_peak(data)
 * Returns the difference between maximum and minimum values
 * Time Complexity: O(n) - single pass
 * ============================================================ */
static PyObject* peak_to_peak(PyObject *self, PyObject *args) {
    PyObject *data;
    double *values = NULL;
    int len;
    int i;
    double min_val, max_val;
    double result;
    
    // Parse argument: a single list or tuple
    if (!PyArg_ParseTuple(args, "O", &data))
        return NULL;
    
    // Convert Python sequence to C array
    if (!convert_to_doubles(data, &values, &len))
        return NULL;
    
    // Handle empty input
    if (len == 0) {
        if (values) free(values);
        PyErr_SetString(PyExc_ValueError, "Cannot compute peak-to-peak of empty sequence");
        return NULL;
    }
    
    // Single pass to find min and max
    min_val = values[0];
    max_val = values[0];
    
    for (i = 1; i < len; i++) {
        if (values[i] < min_val)
            min_val = values[i];
        if (values[i] > max_val)
            max_val = values[i];
    }
    
    result = max_val - min_val;
    
    // Clean up
    if (values) free(values);
    
    return PyFloat_FromDouble(result);
}

/* ============================================================
 * Function 2: rms(data)
 * Returns Root Mean Square value: sqrt(mean(x^2))
 * Time Complexity: O(n) - single pass
 * ============================================================ */
static PyObject* rms(PyObject *self, PyObject *args) {
    PyObject *data;
    double *values = NULL;
    int len;
    int i;
    double sum_sq = 0.0;
    double result;
    
    if (!PyArg_ParseTuple(args, "O", &data))
        return NULL;
    
    if (!convert_to_doubles(data, &values, &len))
        return NULL;
    
    if (len == 0) {
        if (values) free(values);
        PyErr_SetString(PyExc_ValueError, "Cannot compute RMS of empty sequence");
        return NULL;
    }
    
    // Single pass: sum of squares
    for (i = 0; i < len; i++) {
        sum_sq += values[i] * values[i];
    }
    
    result = sqrt(sum_sq / len);
    
    if (values) free(values);
    
    return PyFloat_FromDouble(result);
}

/* ============================================================
 * Function 3: std_dev(data)
 * Returns sample standard deviation
 * Uses two-pass algorithm for numerical stability
 * Time Complexity: O(n) - two passes
 * ============================================================ */
static PyObject* std_dev(PyObject *self, PyObject *args) {
    PyObject *data;
    double *values = NULL;
    int len;
    int i;
    double mean = 0.0;
    double sum_sq = 0.0;
    double variance;
    double result;
    
    if (!PyArg_ParseTuple(args, "O", &data))
        return NULL;
    
    if (!convert_to_doubles(data, &values, &len))
        return NULL;
    
    // Handle edge cases
    if (len == 0) {
        if (values) free(values);
        PyErr_SetString(PyExc_ValueError, "Cannot compute std_dev of empty sequence");
        return NULL;
    }
    
    if (len == 1) {
        if (values) free(values);
        return PyFloat_FromDouble(0.0);  // Sample std dev of single value is 0
    }
    
    // First pass: calculate mean
    for (i = 0; i < len; i++) {
        mean += values[i];
    }
    mean /= len;
    
    // Second pass: sum of squared differences
    for (i = 0; i < len; i++) {
        double diff = values[i] - mean;
        sum_sq += diff * diff;
    }
    
    // Sample variance: divide by (n-1)
    variance = sum_sq / (len - 1);
    result = sqrt(variance);
    
    if (values) free(values);
    
    return PyFloat_FromDouble(result);
}

/* ============================================================
 * Function 4: above_threshold(data, threshold)
 * Returns count of readings strictly greater than threshold
 * Time Complexity: O(n) - single pass
 * ============================================================ */
static PyObject* above_threshold(PyObject *self, PyObject *args) {
    PyObject *data;
    double threshold;
    double *values = NULL;
    int len;
    int i;
    int count = 0;
    
    if (!PyArg_ParseTuple(args, "Od", &data, &threshold))
        return NULL;
    
    if (!convert_to_doubles(data, &values, &len))
        return NULL;
    
    // Single pass: count values above threshold
    for (i = 0; i < len; i++) {
        if (values[i] > threshold) {
            count++;
        }
    }
    
    if (values) free(values);
    
    return PyLong_FromLong(count);
}

/* ============================================================
 * Function 5: summary(data)
 * Returns dictionary with: count, mean, min, max
 * Time Complexity: O(n) - single pass
 * ============================================================ */
static PyObject* summary(PyObject *self, PyObject *args) {
    PyObject *data;
    PyObject *result_dict;
    double *values = NULL;
    int len;
    int i;
    double sum = 0.0;
    double min_val, max_val;
    double mean;
    
    if (!PyArg_ParseTuple(args, "O", &data))
        return NULL;
    
    if (!convert_to_doubles(data, &values, &len))
        return NULL;
    
    // Handle empty input
    if (len == 0) {
        if (values) free(values);
        PyErr_SetString(PyExc_ValueError, "Cannot compute summary of empty sequence");
        return NULL;
    }
    
    // Single pass: sum, min, max
    min_val = values[0];
    max_val = values[0];
    sum = 0.0;
    
    for (i = 0; i < len; i++) {
        sum += values[i];
        if (values[i] < min_val)
            min_val = values[i];
        if (values[i] > max_val)
            max_val = values[i];
    }
    
    mean = sum / len;
    
    // Create dictionary
    result_dict = PyDict_New();
    if (!result_dict) {
        if (values) free(values);
        return NULL;
    }
    
    // Populate dictionary
    PyDict_SetItemString(result_dict, "count", PyLong_FromLong(len));
    PyDict_SetItemString(result_dict, "mean", PyFloat_FromDouble(mean));
    PyDict_SetItemString(result_dict, "min", PyFloat_FromDouble(min_val));
    PyDict_SetItemString(result_dict, "max", PyFloat_FromDouble(max_val));
    
    if (values) free(values);
    
    return result_dict;
}

/* ============================================================
 * Method Table: Maps Python function names to C functions
 * ============================================================ */
static PyMethodDef VibrationMethods[] = {
    {"peak_to_peak", peak_to_peak, METH_VARARGS, 
     "Return peak-to-peak amplitude (max - min)"},
    {"rms", rms, METH_VARARGS,
     "Return Root Mean Square value"},
    {"std_dev", std_dev, METH_VARARGS,
     "Return sample standard deviation"},
    {"above_threshold", above_threshold, METH_VARARGS,
     "Return count of values above threshold"},
    {"summary", summary, METH_VARARGS,
     "Return dictionary with count, mean, min, max"},
    {NULL, NULL, 0, NULL}  // Sentinel
};

/* ============================================================
 * Module Definition
 * ============================================================ */
static struct PyModuleDef vibrationmodule = {
    PyModuleDef_HEAD_INIT,
    "vibration",  // Module name
    "Vibration analysis module for real-time signal processing",  // Docstring
    -1,  // Per-interpreter state size
    VibrationMethods
};

/* ============================================================
 * Initialization Function: Called when module is imported
 * ============================================================ */
PyMODINIT_FUNC PyInit_vibration(void) {
    return PyModule_Create(&vibrationmodule);
}