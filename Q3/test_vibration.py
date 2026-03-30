#!/usr/bin/env python3
"""
test_vibration.py - Test script for vibration analysis module
Demonstrates all functions with sample data and edge cases
"""

import vibration
import math

def print_section(title):
    """Print formatted section header"""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)

def test_normal_case():
    """Test with normal vibration data"""
    print_section("Normal Case: Sample Vibration Readings")
    
    # Simulate vibration data from machinery
    data = [12.5, 14.2, 13.8, 15.1, 11.9, 13.5, 14.8, 12.2, 13.1, 14.5]
    
    print(f"Data: {data}")
    print(f"Length: {len(data)} readings")
    print()
    
    # Test each function
    peak = vibration.peak_to_peak(data)
    print(f"peak_to_peak: {peak:.3f} m/s²")
    
    rms_val = vibration.rms(data)
    print(f"rms: {rms_val:.3f} m/s²")
    
    std = vibration.std_dev(data)
    print(f"std_dev: {std:.3f} m/s²")
    
    threshold = 13.5
    count = vibration.above_threshold(data, threshold)
    print(f"above_threshold({threshold}): {count} readings")
    
    summary = vibration.summary(data)
    print(f"summary: {summary}")

def test_edge_case_empty():
    """Test with empty list"""
    print_section("Edge Case: Empty List")
    
    data = []
    print(f"Data: {data}")
    
    # Test each function with exception handling
    try:
        result = vibration.peak_to_peak(data)
        print(f"peak_to_peak: {result}")
    except ValueError as e:
        print(f"peak_to_peak: Error - {e}")
    
    try:
        result = vibration.rms(data)
        print(f"rms: {result}")
    except ValueError as e:
        print(f"rms: Error - {e}")
    
    try:
        result = vibration.std_dev(data)
        print(f"std_dev: {result}")
    except ValueError as e:
        print(f"std_dev: Error - {e}")
    
    try:
        result = vibration.above_threshold(data, 10.0)
        print(f"above_threshold: {result}")
    except ValueError as e:
        print(f"above_threshold: Error - {e}")
    
    try:
        result = vibration.summary(data)
        print(f"summary: {result}")
    except ValueError as e:
        print(f"summary: Error - {e}")

def test_edge_case_single_value():
    """Test with single value"""
    print_section("Edge Case: Single Reading")
    
    data = [15.5]
    print(f"Data: {data}")
    
    peak = vibration.peak_to_peak(data)
    print(f"peak_to_peak: {peak:.3f} m/s²")
    
    rms_val = vibration.rms(data)
    print(f"rms: {rms_val:.3f} m/s²")
    
    std = vibration.std_dev(data)
    print(f"std_dev: {std:.3f} m/s² (sample std dev of single value = 0)")
    
    threshold = 14.0
    count = vibration.above_threshold(data, threshold)
    print(f"above_threshold({threshold}): {count} reading")
    
    summary = vibration.summary(data)
    print(f"summary: {summary}")

def test_edge_case_invalid_input():
    """Test with invalid input types"""
    print_section("Edge Case: Invalid Input Types")
    
    # Test with string
    try:
        result = vibration.peak_to_peak("not a list")
        print(f"String input: {result}")
    except TypeError as e:
        print(f"String input: Error - {e}")
    
    # Test with integer list (should work, automatically converts)
    data = [1, 2, 3, 4, 5]
    print(f"Integer list: {data}")
    result = vibration.rms(data)
    print(f"rms of integers: {result:.3f}")
    
    # Test with mixed types
    try:
        data = [1.0, 2, "three", 4.0]
        result = vibration.rms(data)
        print(f"Mixed types: {result}")
    except TypeError as e:
        print(f"Mixed types: Error - {e}")

def test_large_dataset():
    """Test performance with larger dataset"""
    print_section("Performance Test: 1 Million Readings")
    
    import random
    import time
    
    # Generate 1 million random vibration readings
    data = [random.uniform(0, 100) for _ in range(1000000)]
    
    print(f"Dataset size: {len(data)} readings")
    
    # Test each function and measure time
    start = time.time()
    peak = vibration.peak_to_peak(data)
    print(f"peak_to_peak: {peak:.3f} m/s² (time: {(time.time() - start)*1000:.2f} ms)")
    
    start = time.time()
    rms_val = vibration.rms(data)
    print(f"rms: {rms_val:.3f} m/s² (time: {(time.time() - start)*1000:.2f} ms)")
    
    start = time.time()
    std = vibration.std_dev(data)
    print(f"std_dev: {std:.3f} m/s² (time: {(time.time() - start)*1000:.2f} ms)")
    
    start = time.time()
    count = vibration.above_threshold(data, 50.0)
    print(f"above_threshold(50.0): {count} readings (time: {(time.time() - start)*1000:.2f} ms)")
    
    start = time.time()
    summary = vibration.summary(data)
    print(f"summary: computed (time: {(time.time() - start)*1000:.2f} ms)")

def test_accuracy():
    """Verify accuracy against Python math"""
    print_section("Accuracy Test: Compare with Python math")
    
    data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
    
    # Python implementation for comparison
    def py_peak_to_peak(data):
        return max(data) - min(data)
    
    def py_rms(data):
        return math.sqrt(sum(x*x for x in data) / len(data))
    
    def py_std_dev(data):
        mean = sum(data) / len(data)
        variance = sum((x - mean)**2 for x in data) / (len(data) - 1)
        return math.sqrt(variance)
    
    print(f"Data: {data}")
    print()
    
    # Compare results
    print(f"peak_to_peak - C: {vibration.peak_to_peak(data):.6f}")
    print(f"peak_to_peak - Python: {py_peak_to_peak(data):.6f}")
    print(f"Match: {abs(vibration.peak_to_peak(data) - py_peak_to_peak(data)) < 1e-10}")
    print()
    
    print(f"rms - C: {vibration.rms(data):.6f}")
    print(f"rms - Python: {py_rms(data):.6f}")
    print(f"Match: {abs(vibration.rms(data) - py_rms(data)) < 1e-10}")
    print()
    
    print(f"std_dev - C: {vibration.std_dev(data):.6f}")
    print(f"std_dev - Python: {py_std_dev(data):.6f}")
    print(f"Match: {abs(vibration.std_dev(data) - py_std_dev(data)) < 1e-10}")

def main():
    """Run all tests"""
    print("\n" + "=" * 60)
    print("  VIBRATION ANALYSIS MODULE - TEST SUITE")
    print("=" * 60)
    
    test_normal_case()
    test_edge_case_empty()
    test_edge_case_single_value()
    test_edge_case_invalid_input()
    test_accuracy()
    test_large_dataset()
    
    print("\n" + "=" * 60)
    print("  ALL TESTS COMPLETED")
    print("=" * 60)

if __name__ == "__main__":
    main()