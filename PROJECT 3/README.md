# Project 3: Python C Extension for High-Performance Data Processing

## Overview

This project implements a Python C extension module (`sensor_analysis`) that performs statistical operations on sensor data directly in C for improved performance. Designed for smart agriculture monitoring platforms processing large volumes of environmental sensor data.

## Files

- `sensor_analysis.c` - C extension source code
- `setup.py` - Build configuration script
- `test_sensor_analysis.py` - Python test program
- `README.md` - This file

## Functions

| Function                   | Description                                | Time Complexity |
| -------------------------- | ------------------------------------------ | --------------- |
| `average(data)`            | Arithmetic mean of readings                | O(n)            |
| `range_value(data)`        | Difference between max and min             | O(n)            |
| `variance(data)`           | Sample variance of readings                | O(n)            |
| `count_above(data, limit)` | Count readings above a limit               | O(n)            |
| `statistics(data)`         | Dictionary with samples, average, min, max | O(n)            |

## Build Instructions

### Prerequisites

- Python 3.x with development headers
- C compiler (gcc, clang, or MSVC)
- setuptools

### Build and Install

```bash
cd "PROJECT 3"
python setup.py build_ext --inplace
```

### Test

```bash
python test_sensor_analysis.py
```

## Design Decisions

### Memory Management

No unnecessary dynamic memory allocation:

- Input data accessed via borrowed references from Python objects
- Output values returned as Python objects via API functions
- Stack-allocated C variables for all calculations
- Only minimal allocation for extracting data to C array

### Numerical Accuracy

- Uses `double` (64-bit floating point) for all calculations
- Two-pass variance calculation for better numerical stability
- Standard double precision sufficient for IoT sensor data

### Input Validation

- Type checking via `PyNumber_Check` and `PySequence_Check`
- Empty dataset handling with `ValueError`
- Non-numeric elements raise `TypeError`
