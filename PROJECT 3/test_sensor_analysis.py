"""
=============================================================================
Python Test Program for sensor_analysis C Extension Module
=============================================================================
This script tests all functions of the sensor_analysis module including:
  - Normal operation with valid data
  - Edge cases (empty list, single element, etc.)
  - Invalid input (non-numeric data, wrong types)

Usage:
  python test_sensor_analysis.py

Expected output includes all function results for various test cases,
including error handling demonstrations.
=============================================================================
"""

import sensor_analysis
import sys


def print_separator(title):
    """Print a formatted section header."""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)


def test_normal_operation():
    """Test all functions with typical sensor data."""
    print_separator("NORMAL OPERATION TESTS")

    # Simulated soil moisture sensor readings (%)
    sensor_data = [23.5, 24.1, 22.8, 23.9, 25.0, 24.7, 23.3, 22.1, 26.2, 24.5]

    print(f"Data: {sensor_data}")
    print(f"Number of samples: {len(sensor_data)}")

    # Test average()
    avg = sensor_analysis.average(sensor_data)
    print(f"Average: {avg:.4f}")

    # Test range_value()
    rng = sensor_analysis.range_value(sensor_data)
    print(f"Range: {rng:.4f}")

    # Test variance()
    var = sensor_analysis.variance(sensor_data)
    print(f"Sample Variance: {var:.4f}")

    # Test count_above()
    limit = 24.0
    count = sensor_analysis.count_above(sensor_data, limit)
    print(f"Count above {limit}: {count}")

    # Test statistics()
    stats = sensor_analysis.statistics(sensor_data)
    print(f"Statistics: {stats}")
    for key, value in stats.items():
        print(f"  {key}: {value}")


def test_tuple_input():
    """Test that tuples work as input (not just lists)."""
    print_separator("TUPLE INPUT TEST")

    data = (15.5, 16.2, 14.8, 15.9, 16.5)
    print(f"Tuple data: {data}")
    print(f"Average: {sensor_analysis.average(data):.4f}")
    print(f"Statistics: {sensor_analysis.statistics(data)}")


def test_single_element():
    """Test with a single data point."""
    print_separator("SINGLE ELEMENT TEST")

    data = [42.0]
    print(f"Data: {data}")
    print(f"Average: {sensor_analysis.average(data):.4f}")
    print(f"Range: {sensor_analysis.range_value(data):.4f}")
    print(f"Variance (single element): {sensor_analysis.variance(data):.4f}")
    print(f"Count above 40: {sensor_analysis.count_above(data, 40.0)}")
    print(f"Statistics: {sensor_analysis.statistics(data)}")


def test_large_dataset():
    """Test performance with a larger dataset (stress test)."""
    print_separator("LARGE DATASET TEST")

    # Generate 100,000 data points
    data = [float(i % 1000) + (i * 0.001) for i in range(100000)]

    import time
    start = time.time()
    avg = sensor_analysis.average(data)
    rng = sensor_analysis.range_value(data)
    var = sensor_analysis.variance(data)
    count = sensor_analysis.count_above(data, 500.0)
    stats = sensor_analysis.statistics(data)
    elapsed = time.time() - start

    print(f"Processed {len(data)} data points in {elapsed:.4f} seconds")
    print(f"Average: {avg:.4f}")
    print(f"Range: {rng:.4f}")
    print(f"Variance: {var:.4f}")
    print(f"Count above 500: {count}")
    print(f"Statistics: {stats}")


def test_boundary_conditions():
    """Test edge cases and boundary conditions."""
    print_separator("BOUNDARY CONDITION TESTS")

    # All identical values
    print("All identical values:")
    data = [25.0, 25.0, 25.0, 25.0, 25.0]
    print(f"  Average: {sensor_analysis.average(data):.4f}")
    print(f"  Range: {sensor_analysis.range_value(data):.4f}")
    print(f"  Variance: {sensor_analysis.variance(data):.4f}")
    print(f"  Statistics: {sensor_analysis.statistics(data)}")

    # Negative values
    print("\nWith negative values:")
    data = [-5.0, 0.0, 10.0, -2.5, 7.5]
    print(f"  Average: {sensor_analysis.average(data):.4f}")
    print(f"  Range: {sensor_analysis.range_value(data):.4f}")
    print(f"  Statistics: {sensor_analysis.statistics(data)}")

    # Floating point precision
    print("\nFloating point precision test:")
    data = [0.1, 0.2, 0.3, 0.4, 0.5]
    print(f"  Average: {sensor_analysis.average(data):.20f}")
    print(f"  Statistics: {sensor_analysis.statistics(data)}")

    # Very small numbers
    print("\nVery small numbers:")
    data = [1e-10, 2e-10, 3e-10]
    print(f"  Average: {sensor_analysis.average(data):.15f}")
    print(f"  Variance: {sensor_analysis.variance(data):.15f}")


def test_error_handling():
    """Test error handling with invalid inputs."""
    print_separator("ERROR HANDLING TESTS")

    # Test 1: Empty list (should raise ValueError)
    print("\n1. Empty list:")
    try:
        sensor_analysis.average([])
    except ValueError as e:
        print(f"   Correctly raised ValueError: {e}")
    except Exception as e:
        print(f"   Raised {type(e).__name__}: {e}")

    # Test 2: Non-numeric data (should raise TypeError)
    print("\n2. Non-numeric data (strings):")
    try:
        sensor_analysis.average(["a", "b", "c"])
    except TypeError as e:
        print(f"   Correctly raised TypeError: {e}")
    except Exception as e:
        print(f"   Raised {type(e).__name__}: {e}")

    # Test 3: Wrong argument type (integer instead of sequence)
    print("\n3. Integer instead of list:")
    try:
        sensor_analysis.average(42)
    except TypeError as e:
        print(f"   Correctly raised TypeError: {e}")
    except Exception as e:
        print(f"   Raised {type(e).__name__}: {e}")

    # Test 4: Mixed numeric/non-numeric
    print("\n4. Mixed data (numbers and None):")
    try:
        sensor_analysis.average([1.0, 2.0, None, 4.0])
    except TypeError as e:
        print(f"   Correctly raised TypeError: {e}")
    except Exception as e:
        print(f"   Raised {type(e).__name__}: {e}")

    # Test 5: count_above with invalid limit
    print("\n5. count_above with string limit:")
    try:
        sensor_analysis.count_above([1.0, 2.0], "invalid")
    except TypeError as e:
        print(f"   Correctly raised TypeError: {e}")
    except Exception as e:
        print(f"   Raised {type(e).__name__}: {e}")


def test_simulated_sensor_data():
    """Test with realistic smart agriculture sensor data."""
    print_separator("SIMULATED SMART AGRICULTURE SENSOR DATA")

    # Soil moisture readings from 10 sensors over 3 time periods
    readings = {
        "Morning":   [22.3, 23.1, 21.8, 24.0, 22.7, 23.5, 22.0, 23.8, 21.5, 24.2],
        "Afternoon": [20.1, 19.8, 21.0, 20.5, 19.2, 20.8, 19.5, 21.2, 20.0, 19.6],
        "Evening":   [24.5, 25.1, 23.8, 25.5, 24.2, 25.0, 24.0, 25.3, 23.5, 25.8],
    }

    for period, data in readings.items():
        print(f"\n{period} Readings:")
        stats = sensor_analysis.statistics(data)
        print(f"  Samples: {stats['samples']}")
        print(f"  Average: {stats['average']:.2f}% moisture")
        print(f"  Min: {stats['minimum']:.2f}%, Max: {stats['maximum']:.2f}%")
        print(f"  Range: {sensor_analysis.range_value(data):.2f}%")
        print(f"  Variance: {sensor_analysis.variance(data):.4f}")
        print(f"  Readings above 23%: {sensor_analysis.count_above(data, 23.0)}")


if __name__ == "__main__":
    print("=" * 60)
    print("  sensor_analysis C Extension - Test Suite")
    print("=" * 60)

    # Check Python and module version
    print(f"Python version: {sys.version}")
    print(f"Module: {sensor_analysis.__doc__}")

    # Run all tests
    test_normal_operation()
    test_tuple_input()
    test_single_element()
    test_large_dataset()
    test_boundary_conditions()
    test_error_handling()
    test_simulated_sensor_data()

    print("\n" + "=" * 60)
    print("  ALL TESTS COMPLETED")
    print("=" * 60)

