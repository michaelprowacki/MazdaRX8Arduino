#!/usr/bin/env python3
"""
A2L File Generator for FOME ECU

Generates ASAM MCD-2MC (A2L) description files for professional
calibration tools (ETAS INCA, Vector CANape, ATI Vision).

Usage:
    python a2l_generator.py --output ecu.a2l --config variables.json
"""

import argparse
import json
from datetime import datetime
from typing import List, Dict, Any

class A2LGenerator:
    """Generates A2L files from variable definitions."""

    def __init__(self, project_name: str = "FOME_RX8_ECU", version: str = "1.0"):
        self.project_name = project_name
        self.version = version
        self.measurements: List[Dict[str, Any]] = []
        self.characteristics: List[Dict[str, Any]] = []
        self.compu_methods: List[Dict[str, Any]] = []
        self.record_layouts: List[Dict[str, Any]] = []

    def add_measurement(self, name: str, address: int, datatype: str,
                       description: str = "", unit: str = "",
                       lower_limit: float = 0, upper_limit: float = 100,
                       conversion: str = "NO_COMPU_METHOD"):
        """Add a measurement variable (read-only from ECU)."""
        self.measurements.append({
            "name": name,
            "address": address,
            "datatype": datatype,
            "description": description,
            "unit": unit,
            "lower_limit": lower_limit,
            "upper_limit": upper_limit,
            "conversion": conversion
        })

    def add_characteristic(self, name: str, address: int, datatype: str,
                          description: str = "", unit: str = "",
                          lower_limit: float = 0, upper_limit: float = 100,
                          conversion: str = "NO_COMPU_METHOD"):
        """Add a characteristic variable (calibratable parameter)."""
        self.characteristics.append({
            "name": name,
            "address": address,
            "datatype": datatype,
            "description": description,
            "unit": unit,
            "lower_limit": lower_limit,
            "upper_limit": upper_limit,
            "conversion": conversion
        })

    def add_compu_method(self, name: str, formula: str, unit: str,
                        description: str = ""):
        """Add a computation method for unit conversion."""
        self.compu_methods.append({
            "name": name,
            "formula": formula,
            "unit": unit,
            "description": description
        })

    def add_standard_ecu_variables(self):
        """Add standard ECU measurements and characteristics."""

        # Computation methods
        self.add_compu_method("CM_RPM", "X*0.25", "rpm", "RPM conversion")
        self.add_compu_method("CM_TEMP", "X*0.1-40", "degC", "Temperature conversion")
        self.add_compu_method("CM_TPS", "X*0.5", "%", "Throttle position conversion")
        self.add_compu_method("CM_AFR", "X*0.1", "AFR", "Air-fuel ratio")
        self.add_compu_method("CM_MAP", "X*0.1", "kPa", "Manifold pressure")
        self.add_compu_method("CM_TIMING", "X*0.5-20", "deg", "Ignition timing")
        self.add_compu_method("CM_VOLTAGE", "X*0.01", "V", "Voltage conversion")
        self.add_compu_method("CM_SPEED", "X", "km/h", "Vehicle speed")
        self.add_compu_method("CM_PRESSURE", "X*0.1", "kPa", "Pressure")

        # Standard measurements
        self.add_measurement("engineRPM", 0x20001000, "UWORD",
                           "Engine speed", "rpm", 0, 10000, "CM_RPM")
        self.add_measurement("coolantTemp", 0x20001002, "SWORD",
                           "Coolant temperature", "degC", -40, 150, "CM_TEMP")
        self.add_measurement("intakeTemp", 0x20001004, "SWORD",
                           "Intake air temperature", "degC", -40, 150, "CM_TEMP")
        self.add_measurement("throttlePosition", 0x20001006, "UBYTE",
                           "Throttle position sensor", "%", 0, 100, "CM_TPS")
        self.add_measurement("manifoldPressure", 0x20001007, "UWORD",
                           "Manifold absolute pressure", "kPa", 0, 300, "CM_MAP")
        self.add_measurement("airFuelRatio", 0x20001009, "UWORD",
                           "Measured air-fuel ratio", "AFR", 10, 20, "CM_AFR")
        self.add_measurement("ignitionTiming", 0x2000100B, "SBYTE",
                           "Current ignition timing", "deg", -20, 60, "CM_TIMING")
        self.add_measurement("batteryVoltage", 0x2000100C, "UWORD",
                           "Battery voltage", "V", 0, 20, "CM_VOLTAGE")
        self.add_measurement("vehicleSpeed", 0x2000100E, "UWORD",
                           "Vehicle speed", "km/h", 0, 300, "CM_SPEED")
        self.add_measurement("oilPressure", 0x20001010, "UWORD",
                           "Engine oil pressure", "kPa", 0, 1000, "CM_PRESSURE")

        # Fuel control measurements
        self.add_measurement("shortTermFuelTrim", 0x20001020, "SBYTE",
                           "Short term fuel trim", "%", -25, 25)
        self.add_measurement("longTermFuelTrim", 0x20001021, "SBYTE",
                           "Long term fuel trim", "%", -25, 25)
        self.add_measurement("injectorPulseWidth", 0x20001022, "UWORD",
                           "Injector pulse width", "us", 0, 20000)
        self.add_measurement("fuelPressure", 0x20001024, "UWORD",
                           "Fuel rail pressure", "kPa", 0, 600, "CM_PRESSURE")

        # Knock control measurements
        self.add_measurement("knockRetard", 0x20001030, "UBYTE",
                           "Knock retard", "deg", 0, 20)
        self.add_measurement("knockLevel", 0x20001031, "UBYTE",
                           "Knock sensor level", "counts", 0, 255)

        # Boost control measurements
        self.add_measurement("boostPressure", 0x20001040, "UWORD",
                           "Boost pressure", "kPa", 0, 300, "CM_MAP")
        self.add_measurement("boostTarget", 0x20001042, "UWORD",
                           "Boost target", "kPa", 0, 300, "CM_MAP")
        self.add_measurement("wastegatePosition", 0x20001044, "UBYTE",
                           "Wastegate position", "%", 0, 100)

        # Standard characteristics (calibratable)
        self.add_characteristic("idleRPMTarget", 0x20002000, "UWORD",
                              "Target idle RPM", "rpm", 500, 2000, "CM_RPM")
        self.add_characteristic("fuelMapBaseValue", 0x20002002, "UWORD",
                              "Base fuel map value", "ms", 0, 20)
        self.add_characteristic("ignitionMapBaseValue", 0x20002004, "SBYTE",
                              "Base ignition timing", "deg", -20, 60, "CM_TIMING")
        self.add_characteristic("boostTargetMax", 0x20002005, "UWORD",
                              "Maximum boost target", "kPa", 0, 300, "CM_MAP")
        self.add_characteristic("revLimit", 0x20002007, "UWORD",
                              "Engine rev limit", "rpm", 0, 12000, "CM_RPM")
        self.add_characteristic("launchRPM", 0x20002009, "UWORD",
                              "Launch control RPM", "rpm", 2000, 6000, "CM_RPM")
        self.add_characteristic("tractionSlipTarget", 0x2000200B, "UBYTE",
                              "Traction control slip target", "%", 0, 30)

    def generate(self) -> str:
        """Generate the A2L file content."""
        lines = []

        # Header
        lines.append("ASAP2_VERSION 1 61")
        lines.append(f'/begin PROJECT {self.project_name} "{self.project_name}"')
        lines.append("")
        lines.append(f'  /begin HEADER "{self.project_name}"')
        lines.append(f'    VERSION "{self.version}"')
        lines.append(f'    PROJECT_NO {self.project_name}')
        lines.append("  /end HEADER")
        lines.append("")

        # Module
        lines.append(f'  /begin MODULE {self.project_name}_Module ""')
        lines.append("")

        # MOD_COMMON
        lines.append("    /begin MOD_COMMON \"\"")
        lines.append("      BYTE_ORDER MSB_LAST")
        lines.append("      ALIGNMENT_BYTE 1")
        lines.append("      ALIGNMENT_WORD 2")
        lines.append("      ALIGNMENT_LONG 4")
        lines.append("    /end MOD_COMMON")
        lines.append("")

        # MOD_PAR
        lines.append("    /begin MOD_PAR \"\"")
        lines.append(f'      VERSION "{self.version}"')
        lines.append('      ADDR_EPK 0x20000000')
        lines.append('      EPK "FOME ECU"')
        lines.append("      CUSTOMER \"Open Source\"")
        lines.append(f'      USER "Generated {datetime.now().strftime("%Y-%m-%d %H:%M")}"')
        lines.append("    /end MOD_PAR")
        lines.append("")

        # Computation methods
        for cm in self.compu_methods:
            lines.append(f'    /begin COMPU_METHOD {cm["name"]} "{cm["description"]}"')
            lines.append(f'      RAT_FUNC "%6.2" "{cm["unit"]}"')
            lines.append(f'      COEFFS 0 1 0 0 0 1')
            lines.append(f'      /begin FORMULA')
            lines.append(f'        "{cm["formula"]}"')
            lines.append(f'      /end FORMULA')
            lines.append(f'    /end COMPU_METHOD')
            lines.append("")

        # Record layouts
        lines.append("    /begin RECORD_LAYOUT UBYTE_SCALAR")
        lines.append("      FNC_VALUES 1 UBYTE ROW_DIR DIRECT")
        lines.append("    /end RECORD_LAYOUT")
        lines.append("")
        lines.append("    /begin RECORD_LAYOUT SBYTE_SCALAR")
        lines.append("      FNC_VALUES 1 SBYTE ROW_DIR DIRECT")
        lines.append("    /end RECORD_LAYOUT")
        lines.append("")
        lines.append("    /begin RECORD_LAYOUT UWORD_SCALAR")
        lines.append("      FNC_VALUES 1 UWORD ROW_DIR DIRECT")
        lines.append("    /end RECORD_LAYOUT")
        lines.append("")
        lines.append("    /begin RECORD_LAYOUT SWORD_SCALAR")
        lines.append("      FNC_VALUES 1 SWORD ROW_DIR DIRECT")
        lines.append("    /end RECORD_LAYOUT")
        lines.append("")

        # Measurements
        for m in self.measurements:
            lines.append(f'    /begin MEASUREMENT {m["name"]} "{m["description"]}"')
            lines.append(f'      {m["datatype"]} {m["conversion"]} 1 0 {m["lower_limit"]} {m["upper_limit"]}')
            lines.append(f'      ECU_ADDRESS 0x{m["address"]:08X}')
            if m["unit"]:
                lines.append(f'      PHYS_UNIT "{m["unit"]}"')
            lines.append(f'    /end MEASUREMENT')
            lines.append("")

        # Characteristics
        for c in self.characteristics:
            record_layout = f'{c["datatype"]}_SCALAR'
            lines.append(f'    /begin CHARACTERISTIC {c["name"]} "{c["description"]}"')
            lines.append(f'      VALUE 0x{c["address"]:08X} {record_layout} 0 {c["conversion"]} {c["lower_limit"]} {c["upper_limit"]}')
            if c["unit"]:
                lines.append(f'      PHYS_UNIT "{c["unit"]}"')
            lines.append(f'    /end CHARACTERISTIC')
            lines.append("")

        # Close module and project
        lines.append(f"  /end MODULE")
        lines.append("")
        lines.append(f"/end PROJECT")

        return "\n".join(lines)

    def load_from_json(self, filepath: str):
        """Load variable definitions from JSON file."""
        with open(filepath, 'r') as f:
            config = json.load(f)

        if "compu_methods" in config:
            for cm in config["compu_methods"]:
                self.add_compu_method(**cm)

        if "measurements" in config:
            for m in config["measurements"]:
                self.add_measurement(**m)

        if "characteristics" in config:
            for c in config["characteristics"]:
                self.add_characteristic(**c)

    def save(self, filepath: str):
        """Save A2L content to file."""
        content = self.generate()
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Generated A2L file: {filepath}")


def main():
    parser = argparse.ArgumentParser(description="Generate A2L files for FOME ECU")
    parser.add_argument("--output", "-o", default="fome_ecu.a2l",
                       help="Output A2L file path")
    parser.add_argument("--config", "-c",
                       help="JSON config file with variable definitions")
    parser.add_argument("--project", "-p", default="FOME_RX8_ECU",
                       help="Project name")
    parser.add_argument("--version", "-v", default="1.0",
                       help="Version string")
    parser.add_argument("--standard", "-s", action="store_true",
                       help="Include standard ECU variables")

    args = parser.parse_args()

    generator = A2LGenerator(args.project, args.version)

    if args.standard:
        generator.add_standard_ecu_variables()

    if args.config:
        generator.load_from_json(args.config)

    generator.save(args.output)


if __name__ == "__main__":
    main()
