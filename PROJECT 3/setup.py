"""
=============================================================================
Build Configuration for sensor_analysis Python C Extension
=============================================================================
File: setup.py
Description: Build script for the sensor_analysis C extension module.
             Uses setuptools to compile and install the module.

Build Instructions:
  1. Ensure Python development headers are installed:
     Linux:  sudo apt-get install python3-dev
     macOS:  (headers included with Xcode)
     Windows: (ensure "Development Tools" was checked during Python install)

  2. Build and install the extension:
     python setup.py build_ext --inplace

     This compiles the C source into a shared library (.pyd on Windows,
     .so on Linux/macOS) that can be imported directly from Python.

  3. Test the module:
     python test_sensor_analysis.py

Dependencies:
  - Python 3.x (with development headers)
  - A C compiler (gcc, clang, or MSVC)
  - setuptools (should be included with Python)
=============================================================================
"""

from setuptools import Extension, setup
import sys

# GCC-specific flags are not valid for MSVC on Windows.
if sys.platform == 'win32':
    extra_compile_args = []
else:
    extra_compile_args = ['-O2', '-Wall']
# Extension parameters:
#   - name: Python-visible module name (must match PyInit_ function name)
#   - sources: list of C source files to compile
#   - include_dirs: additional directories for header search
#   - libraries: system libraries to link against (m = math library)
extension = Extension(
    'sensor_analysis',                    # Module name
    sources=['sensor_analysis.c'],        # Source file(s)
    libraries=[] if sys.platform == 'win32' else ['m'],
    extra_compile_args=extra_compile_args,
)

# Run the setup
setup(
    name='sensor_analysis',
    version='1.0.0',
    description='High-performance sensor data analysis C extension',
    author='Student',
    ext_modules=[extension],
    python_requires='>=3.6',
)

