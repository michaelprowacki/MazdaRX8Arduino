#!/usr/bin/env python3
"""
A2L Generator Unit Tests
"""

import unittest
import tempfile
import os
import sys

# Add tools directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools'))

from a2l_generator import A2LGenerator


class TestA2LGenerator(unittest.TestCase):
    """Tests for A2L file generator."""

    def setUp(self):
        """Set up test fixtures."""
        self.generator = A2LGenerator("TEST_PROJECT", "1.0")

    def test_basic_generation(self):
        """Test basic A2L file generation."""
        content = self.generator.generate()

        self.assertIn("ASAP2_VERSION 1 61", content)
        self.assertIn("/begin PROJECT TEST_PROJECT", content)
        self.assertIn("/end PROJECT", content)

    def test_add_measurement(self):
        """Test adding a measurement variable."""
        self.generator.add_measurement(
            "testMeasurement",
            0x20001000,
            "UWORD",
            "Test measurement",
            "rpm",
            0, 10000
        )

        content = self.generator.generate()

        self.assertIn("testMeasurement", content)
        self.assertIn("0x20001000", content)
        self.assertIn("UWORD", content)

    def test_add_characteristic(self):
        """Test adding a characteristic (calibratable) variable."""
        self.generator.add_characteristic(
            "testCharacteristic",
            0x20002000,
            "UBYTE",
            "Test characteristic",
            "%",
            0, 100
        )

        content = self.generator.generate()

        self.assertIn("testCharacteristic", content)
        self.assertIn("0x20002000", content)
        self.assertIn("/begin CHARACTERISTIC", content)

    def test_add_compu_method(self):
        """Test adding a computation method."""
        self.generator.add_compu_method(
            "CM_TEST",
            "X*0.1",
            "units",
            "Test conversion"
        )

        content = self.generator.generate()

        self.assertIn("CM_TEST", content)
        self.assertIn("X*0.1", content)
        self.assertIn("/begin COMPU_METHOD", content)

    def test_standard_ecu_variables(self):
        """Test adding standard ECU variables."""
        self.generator.add_standard_ecu_variables()

        content = self.generator.generate()

        # Check for standard measurements
        self.assertIn("engineRPM", content)
        self.assertIn("coolantTemp", content)
        self.assertIn("throttlePosition", content)
        self.assertIn("manifoldPressure", content)
        self.assertIn("airFuelRatio", content)

        # Check for standard characteristics
        self.assertIn("idleRPMTarget", content)
        self.assertIn("revLimit", content)

        # Check for computation methods
        self.assertIn("CM_RPM", content)
        self.assertIn("CM_TEMP", content)

    def test_save_file(self):
        """Test saving A2L file to disk."""
        self.generator.add_standard_ecu_variables()

        with tempfile.NamedTemporaryFile(mode='w', suffix='.a2l', delete=False) as f:
            filepath = f.name

        try:
            self.generator.save(filepath)

            self.assertTrue(os.path.exists(filepath))

            with open(filepath, 'r') as f:
                content = f.read()

            self.assertIn("ASAP2_VERSION", content)
            self.assertIn("engineRPM", content)
        finally:
            os.unlink(filepath)

    def test_measurement_address_format(self):
        """Test that addresses are formatted correctly."""
        self.generator.add_measurement(
            "testVar",
            0xDEADBEEF,
            "ULONG"
        )

        content = self.generator.generate()

        self.assertIn("0xDEADBEEF", content)

    def test_multiple_measurements(self):
        """Test adding multiple measurements."""
        for i in range(5):
            self.generator.add_measurement(
                f"var{i}",
                0x20001000 + i * 4,
                "UWORD"
            )

        content = self.generator.generate()

        for i in range(5):
            self.assertIn(f"var{i}", content)

    def test_record_layouts(self):
        """Test that record layouts are generated."""
        content = self.generator.generate()

        self.assertIn("UBYTE_SCALAR", content)
        self.assertIn("UWORD_SCALAR", content)
        self.assertIn("SBYTE_SCALAR", content)
        self.assertIn("SWORD_SCALAR", content)

    def test_mod_common(self):
        """Test MOD_COMMON section."""
        content = self.generator.generate()

        self.assertIn("BYTE_ORDER MSB_LAST", content)
        self.assertIn("ALIGNMENT_WORD 2", content)
        self.assertIn("ALIGNMENT_LONG 4", content)

    def test_limits(self):
        """Test that limits are included in output."""
        self.generator.add_measurement(
            "limitTest",
            0x20001000,
            "SWORD",
            "",
            "degC",
            -40, 150
        )

        content = self.generator.generate()

        self.assertIn("-40", content)
        self.assertIn("150", content)

    def test_unit_specification(self):
        """Test unit specification in output."""
        self.generator.add_measurement(
            "unitTest",
            0x20001000,
            "UWORD",
            "",
            "kPa"
        )

        content = self.generator.generate()

        self.assertIn('PHYS_UNIT "kPa"', content)

    def test_conversion_reference(self):
        """Test conversion method reference."""
        self.generator.add_compu_method("CM_CUSTOM", "X*2", "units")
        self.generator.add_measurement(
            "convTest",
            0x20001000,
            "UWORD",
            "",
            "",
            0, 100,
            "CM_CUSTOM"
        )

        content = self.generator.generate()

        # Measurement should reference the compu method
        self.assertIn("CM_CUSTOM", content)

    def test_empty_generator(self):
        """Test generating with no variables."""
        content = self.generator.generate()

        # Should still produce valid structure
        self.assertIn("ASAP2_VERSION", content)
        self.assertIn("/begin PROJECT", content)
        self.assertIn("/end PROJECT", content)

    def test_fuel_control_variables(self):
        """Test fuel control related variables."""
        self.generator.add_standard_ecu_variables()

        content = self.generator.generate()

        self.assertIn("shortTermFuelTrim", content)
        self.assertIn("longTermFuelTrim", content)
        self.assertIn("injectorPulseWidth", content)

    def test_knock_control_variables(self):
        """Test knock control variables."""
        self.generator.add_standard_ecu_variables()

        content = self.generator.generate()

        self.assertIn("knockRetard", content)
        self.assertIn("knockLevel", content)

    def test_boost_control_variables(self):
        """Test boost control variables."""
        self.generator.add_standard_ecu_variables()

        content = self.generator.generate()

        self.assertIn("boostPressure", content)
        self.assertIn("boostTarget", content)
        self.assertIn("wastegatePosition", content)


class TestA2LFormulas(unittest.TestCase):
    """Tests for A2L formula/conversion generation."""

    def setUp(self):
        self.generator = A2LGenerator()

    def test_rpm_formula(self):
        """Test RPM conversion formula."""
        self.generator.add_compu_method("CM_RPM", "X*0.25", "rpm")
        content = self.generator.generate()

        self.assertIn("X*0.25", content)

    def test_temperature_formula(self):
        """Test temperature conversion formula."""
        self.generator.add_compu_method("CM_TEMP", "X*0.1-40", "degC")
        content = self.generator.generate()

        self.assertIn("X*0.1-40", content)

    def test_multiple_formulas(self):
        """Test multiple conversion formulas."""
        self.generator.add_standard_ecu_variables()
        content = self.generator.generate()

        # Count COMPU_METHOD blocks
        count = content.count("/begin COMPU_METHOD")
        self.assertGreater(count, 5)


if __name__ == '__main__':
    # Run tests with verbose output
    unittest.main(verbosity=2)
