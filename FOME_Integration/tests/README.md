# FOME Integration Tests

Unit tests for the FOME ECU integration components.

## Test Summary

| Test Suite | Tests | Description |
|------------|-------|-------------|
| test_xcp_protocol | 11 | XCP slave protocol commands |
| test_can_encoding | 17 | RX8 CAN message encoding/decoding |
| test_a2l_generator | 20 | A2L file generation |

## Running Tests

### All Tests
```bash
make test
```

### Individual Test Suites
```bash
# C++ tests
make run-xcp        # XCP protocol tests
make run-can        # CAN encoding tests

# Python tests
make run-a2l        # A2L generator tests
```

### Quick Test (CAN only)
```bash
make quick
```

## Requirements

### C++ Tests
- g++ with C++11 support
- Standard library

### Python Tests
- Python 3.6+
- unittest (standard library)

## Test Coverage

### XCP Protocol (`test_xcp_protocol.cpp`)
- Connect/disconnect
- Memory upload/download
- Short upload
- Set MTA (Memory Transfer Address)
- DAQ list allocation
- DAQ processor info
- Error handling

### CAN Encoding (`test_can_encoding.cpp`)
- RPM encoding (0x201)
- Speed encoding (0x201)
- Throttle encoding (0x201)
- Wheel speed encoding (0x4B1)
- Warning lights (0x420)
- Odometer increments
- Byte order verification

### A2L Generator (`test_a2l_generator.py`)
- File structure
- Measurements
- Characteristics
- Computation methods
- Record layouts
- Address formatting
- Unit specification

## Adding New Tests

### C++ Tests

```cpp
TEST(my_new_test) {
    // Setup
    reset_test_state();

    // Execute
    // ... your test code ...

    // Assert
    ASSERT_EQ(expected, actual);
    ASSERT_TRUE(condition);
}

// Register in main()
RUN_TEST(my_new_test);
```

### Python Tests

```python
def test_my_new_feature(self):
    """Test description."""
    result = function_under_test()
    self.assertEqual(expected, result)
```

## Continuous Integration

Tests can be run in CI with:
```bash
cd FOME_Integration/tests
make test
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Tests failed"
    exit 1
fi
```
