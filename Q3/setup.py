#!/usr/bin/env python3
"""
setup.py - Build script for vibration analysis C extension
"""

from setuptools import setup, Extension

# Define the extension module
vibration_module = Extension(
    'vibration',  # Module name
    sources=['vibration_analysis.c'],  # C source files
    extra_compile_args=['-Wall', '-O3', '-march=native'],  # Optimization flags
    extra_link_args=['-lm'],  # Link math library
)

setup(
    name='vibration',
    version='1.0',
    description='Real-time vibration analysis module',
    author='Student',
    ext_modules=[vibration_module],
    python_requires='>=3.6',
)