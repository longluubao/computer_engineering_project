"""
Zonal Gateway topology model — automotive-grade, interactive data layer.

Based on automotive Zonal E/E Architecture:
  - Central Zone Gateway (HPCU) = Raspberry Pi 4  (always present)
  - Zone Controllers            = Raspberry Pi boards at physical locations
  - Each Zone Controller hosts ECUs with full HW/SW configuration

The topology starts **empty** (0 zones).  The user builds it
interactively via the dashboard, or loads a standard 6-zone preset.
"""

from __future__ import annotations

import uuid
from dataclasses import dataclass, field

from PySide6.QtCore import QObject, Signal


# ── ECU Domain catalogue ──────────────────────────────────────────────

ECU_DOMAINS: dict[str, dict] = {
    "Powertrain": {
        "color_attr": "ECU_POWERTRAIN",
        "short": "PT",
        "desc": "Engine, Transmission, EV Motor, BMS",
    },
    "Infotainment": {
        "color_attr": "ECU_INFOTAINMENT",
        "short": "IVI",
        "desc": "Head Unit, Display, Audio, V2X, Telematics",
    },
    "Body": {
        "color_attr": "ECU_BODY",
        "short": "BDY",
        "desc": "Lighting, Doors, Windows, Seats, HVAC",
    },
    "Chassis": {
        "color_attr": "ECU_CHASSIS",
        "short": "CHS",
        "desc": "ABS, ESP, EPS, Suspension, TPMS",
    },
    "ADAS": {
        "color_attr": "ECU_ADAS",
        "short": "ADAS",
        "desc": "Camera, Radar, Lidar, Ultrasonic, Parking",
    },
}


# ── MCU catalogue (real automotive-grade microcontrollers) ────────────

MCU_CATALOGUE: dict[str, dict] = {
    "Infineon TC397": {
        "vendor": "Infineon Technologies",
        "core": "TriCore 1.6P (6-core)",
        "speed_mhz": 300,
        "flash_kb": 16_384,
        "ram_kb": 6_144,
        "typical": "Powertrain, Safety-Critical",
    },
    "NXP S32K344": {
        "vendor": "NXP Semiconductors",
        "core": "ARM Cortex-M7",
        "speed_mhz": 160,
        "flash_kb": 4_096,
        "ram_kb": 512,
        "typical": "Powertrain, Chassis",
    },
    "STM32H743": {
        "vendor": "STMicroelectronics",
        "core": "ARM Cortex-M7",
        "speed_mhz": 480,
        "flash_kb": 2_048,
        "ram_kb": 1_024,
        "typical": "Gateway, ADAS Sensor Fusion",
    },
    "STM32F446RE": {
        "vendor": "STMicroelectronics",
        "core": "ARM Cortex-M4F",
        "speed_mhz": 180,
        "flash_kb": 512,
        "ram_kb": 128,
        "typical": "Body Control, Comfort",
    },
    "Renesas RH850/U2A": {
        "vendor": "Renesas Electronics",
        "core": "G4MH / RISC-V",
        "speed_mhz": 400,
        "flash_kb": 16_384,
        "ram_kb": 3_686,
        "typical": "Chassis, ADAS",
    },
    "Renesas R-Car H3": {
        "vendor": "Renesas Electronics",
        "core": "ARM Cortex-A57 + A53",
        "speed_mhz": 1_500,
        "flash_kb": 0,
        "ram_kb": 4_194_304,
        "typical": "Infotainment SoC, HMI",
    },
    "ESP32-S3": {
        "vendor": "Espressif Systems",
        "core": "Xtensa LX7 (dual-core)",
        "speed_mhz": 240,
        "flash_kb": 16_384,
        "ram_kb": 512,
        "typical": "V2X, Wi-Fi / BLE Gateway",
    },
    "Raspberry Pi Pico (RP2040)": {
        "vendor": "Raspberry Pi",
        "core": "ARM Cortex-M0+ (dual-core)",
        "speed_mhz": 133,
        "flash_kb": 2_048,
        "ram_kb": 264,
        "typical": "Sensor Interface, Actuator",
    },
    "Arduino Mega (ATmega2560)": {
        "vendor": "Arduino / Microchip",
        "core": "AVR ATmega2560",
        "speed_mhz": 16,
        "flash_kb": 256,
        "ram_kb": 8,
        "typical": "Simple Sensor, LED, Relay",
    },
    "Generic / Simulated": {
        "vendor": "Virtual",
        "core": "Simulated",
        "speed_mhz": 0,
        "flash_kb": 0,
        "ram_kb": 0,
        "typical": "Simulation Only",
    },
}


# ── Protocol / option catalogues ──────────────────────────────────────

LOCAL_BUS_OPTIONS = ["CAN", "CAN-FD", "LIN", "FlexRay", "Ethernet"]

BACKBONE_OPTIONS = [
    "Ethernet 100BASE-T1",
    "Ethernet 1000BASE-T1",
    "Ethernet 10BASE-T1S",
]

SECURITY_OPTIONS = [
    "PQC (ML-DSA-65)",
    "Classic MAC (HMAC)",
    "None",
]

ASIL_LEVELS = ["QM", "ASIL-A", "ASIL-B", "ASIL-C", "ASIL-D"]

ZONE_HARDWARE_OPTIONS = [
    "Raspberry Pi 4 Model B",
    "Raspberry Pi 3 Model B+",
    "Raspberry Pi 5",
    "Simulated (Virtual)",
]

CAN_ADAPTER_OPTIONS = [
    "MCP2515 (SPI)",
    "Waveshare CAN HAT",
    "PiCAN 2",
    "Peak PCAN-USB",
    "Kvaser Leaf Light",
    "None (Ethernet Only)",
    "Simulated",
]

ECU_STATUS_OPTIONS = [
    "Simulated",
    "Connected",
    "Disconnected",
]


# ── ECU presets per domain — common real-world ECUs ───────────────────
# Each entry: (name, default_local_bus, default_security, mcu_model, asil)

ECU_PRESETS: dict[str, list[tuple[str, str, str, str, str]]] = {
    "Powertrain": [
        ("Engine Control Module",     "CAN-FD", "PQC (ML-DSA-65)",    "Infineon TC397",       "ASIL-D"),
        ("Transmission Control Unit", "CAN-FD", "PQC (ML-DSA-65)",    "NXP S32K344",          "ASIL-C"),
        ("EV Motor Controller",       "CAN-FD", "PQC (ML-DSA-65)",    "Infineon TC397",       "ASIL-D"),
        ("Battery Management System", "CAN",    "Classic MAC (HMAC)",  "NXP S32K344",          "ASIL-C"),
    ],
    "Body": [
        ("Body Control Module",       "CAN",    "Classic MAC (HMAC)",  "STM32F446RE",          "ASIL-A"),
        ("Door Control Unit",         "LIN",    "None",                "Raspberry Pi Pico (RP2040)", "QM"),
        ("Seat Control Unit",         "LIN",    "None",                "STM32F446RE",          "QM"),
        ("LED Headlight Controller",  "LIN",    "None",                "Raspberry Pi Pico (RP2040)", "QM"),
    ],
    "Chassis": [
        ("ABS Module",                    "CAN-FD", "PQC (ML-DSA-65)",    "Infineon TC397",       "ASIL-D"),
        ("Electronic Stability Control",  "CAN-FD", "PQC (ML-DSA-65)",    "Infineon TC397",       "ASIL-D"),
        ("Electric Power Steering",       "CAN-FD", "PQC (ML-DSA-65)",    "NXP S32K344",          "ASIL-D"),
        ("Tire Pressure Monitor",         "CAN",    "Classic MAC (HMAC)",  "STM32F446RE",          "ASIL-A"),
    ],
    "ADAS": [
        ("Front Camera Module",    "Ethernet", "PQC (ML-DSA-65)",    "STM32H743",            "ASIL-B"),
        ("Front Radar Module",     "CAN-FD",   "PQC (ML-DSA-65)",    "Renesas RH850/U2A",    "ASIL-B"),
        ("Lidar Processing Unit",  "Ethernet", "PQC (ML-DSA-65)",    "Renesas RH850/U2A",    "ASIL-B"),
        ("Parking Assist ECU",     "CAN",      "Classic MAC (HMAC)",  "STM32F446RE",          "ASIL-A"),
    ],
    "Infotainment": [
        ("Head Unit",                "Ethernet", "PQC (ML-DSA-65)",    "Renesas R-Car H3",     "QM"),
        ("Instrument Cluster",       "Ethernet", "Classic MAC (HMAC)", "STM32H743",            "ASIL-B"),
        ("Telematics Control Unit",  "Ethernet", "PQC (ML-DSA-65)",    "ESP32-S3",             "QM"),
        ("V2X Communication Module", "Ethernet", "PQC (ML-DSA-65)",    "ESP32-S3",             "QM"),
    ],
}


# ── 1-zone demo preset (single zone for demo/thesis) ───────────────────
# ECU tuples: (name, domain, bus, security, mcu_model, asil, node_addr)

_1ZONE_DEMO: dict[str, dict] = {
    "Central Demo": {
        "desc": "Demonstration zone: body control, head unit, sensors",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Raspberry Pi 4 Model B",
        "ip_address": "192.168.1.10",
        "can_adapter": "MCP2515 (SPI)",
        "ecus": [
            ("Body Control Module", "Body",          "CAN",      "PQC (ML-DSA-65)",    "STM32F446RE",           "ASIL-A", "0x200"),
            ("Head Unit",           "Infotainment",  "Ethernet", "PQC (ML-DSA-65)",    "Renesas R-Car H3",      "QM",     "0x400"),
            ("Door Control Unit",   "Body",          "LIN",      "None",               "Raspberry Pi Pico",     "QM",     "0x220"),
            ("Front Radar Module",  "ADAS",          "CAN-FD",   "PQC (ML-DSA-65)",    "Renesas RH850/U2A",     "ASIL-B", "0x310"),
        ],
    },
}


# ── 6-zone preset (standard automotive zonal layout) ──────────────────
# ECU tuples: (name, domain, bus, security, mcu_model, asil, node_addr)

# ── Raspberry Pi 40-pin GPIO pinout ──────────────────────────────────
# pin → {gpio: BCM number or None, name: label, type: category}

GPIO_PINOUT_40PIN: dict[int, dict] = {
    1:  {"gpio": None, "name": "3V3 Power",    "type": "power_3v3"},
    2:  {"gpio": None, "name": "5V Power",     "type": "power_5v"},
    3:  {"gpio": 2,    "name": "GPIO2 (SDA1)", "type": "i2c"},
    4:  {"gpio": None, "name": "5V Power",     "type": "power_5v"},
    5:  {"gpio": 3,    "name": "GPIO3 (SCL1)", "type": "i2c"},
    6:  {"gpio": None, "name": "Ground",       "type": "ground"},
    7:  {"gpio": 4,    "name": "GPIO4 (GPCLK0)", "type": "gpio"},
    8:  {"gpio": 14,   "name": "GPIO14 (TXD)", "type": "uart"},
    9:  {"gpio": None, "name": "Ground",       "type": "ground"},
    10: {"gpio": 15,   "name": "GPIO15 (RXD)", "type": "uart"},
    11: {"gpio": 17,   "name": "GPIO17",       "type": "gpio"},
    12: {"gpio": 18,   "name": "GPIO18 (PWM0)", "type": "pwm"},
    13: {"gpio": 27,   "name": "GPIO27",       "type": "gpio"},
    14: {"gpio": None, "name": "Ground",       "type": "ground"},
    15: {"gpio": 22,   "name": "GPIO22",       "type": "gpio"},
    16: {"gpio": 23,   "name": "GPIO23",       "type": "gpio"},
    17: {"gpio": None, "name": "3V3 Power",    "type": "power_3v3"},
    18: {"gpio": 24,   "name": "GPIO24",       "type": "gpio"},
    19: {"gpio": 10,   "name": "GPIO10 (MOSI)", "type": "spi"},
    20: {"gpio": None, "name": "Ground",       "type": "ground"},
    21: {"gpio": 9,    "name": "GPIO9 (MISO)", "type": "spi"},
    22: {"gpio": 25,   "name": "GPIO25",       "type": "gpio"},
    23: {"gpio": 11,   "name": "GPIO11 (SCLK)", "type": "spi"},
    24: {"gpio": 8,    "name": "GPIO8 (CE0)",  "type": "spi"},
    25: {"gpio": None, "name": "Ground",       "type": "ground"},
    26: {"gpio": 7,    "name": "GPIO7 (CE1)",  "type": "spi"},
    27: {"gpio": 0,    "name": "GPIO0 (ID_SD)", "type": "i2c"},
    28: {"gpio": 1,    "name": "GPIO1 (ID_SC)", "type": "i2c"},
    29: {"gpio": 5,    "name": "GPIO5",        "type": "gpio"},
    30: {"gpio": None, "name": "Ground",       "type": "ground"},
    31: {"gpio": 6,    "name": "GPIO6",        "type": "gpio"},
    32: {"gpio": 12,   "name": "GPIO12 (PWM0)", "type": "pwm"},
    33: {"gpio": 13,   "name": "GPIO13 (PWM1)", "type": "pwm"},
    34: {"gpio": None, "name": "Ground",       "type": "ground"},
    35: {"gpio": 19,   "name": "GPIO19 (MISO1)", "type": "spi"},
    36: {"gpio": 16,   "name": "GPIO16",       "type": "gpio"},
    37: {"gpio": 26,   "name": "GPIO26",       "type": "gpio"},
    38: {"gpio": 20,   "name": "GPIO20 (MOSI1)", "type": "spi"},
    39: {"gpio": None, "name": "Ground",       "type": "ground"},
    40: {"gpio": 21,   "name": "GPIO21 (SCLK1)", "type": "spi"},
}

PIN_TYPE_COLORS: dict[str, str] = {
    "power_5v":  "#EF4444",
    "power_3v3": "#F59E0B",
    "ground":    "#1E293B",
    "spi":       "#0098CE",
    "i2c":       "#10B981",
    "uart":      "#F59E0B",
    "pwm":       "#7C3AED",
    "gpio":      "#94A3B8",
}

# ── CAN adapter wiring details ──────────────────────────────────────

ADAPTER_WIRING: dict[str, dict] = {
    "MCP2515 (SPI)": {
        "interface": "SPI0",
        "transceiver": "MCP2551 / TJA1050",
        "pins": [
            {"adapter": "VCC",  "function": "3.3V Power",   "gpio": None, "pin": 1,  "notes": "3.3V supply"},
            {"adapter": "GND",  "function": "Ground",       "gpio": None, "pin": 6,  "notes": "Common ground"},
            {"adapter": "MOSI", "function": "SPI0 MOSI",    "gpio": 10,   "pin": 19, "notes": "Master Out"},
            {"adapter": "MISO", "function": "SPI0 MISO",    "gpio": 9,    "pin": 21, "notes": "Master In"},
            {"adapter": "SCK",  "function": "SPI0 SCLK",    "gpio": 11,   "pin": 23, "notes": "SPI Clock"},
            {"adapter": "CS",   "function": "SPI0 CE0",     "gpio": 8,    "pin": 24, "notes": "Chip Select"},
            {"adapter": "INT",  "function": "Interrupt",     "gpio": 25,   "pin": 22, "notes": "CAN interrupt"},
        ],
    },
    "Waveshare CAN HAT": {
        "interface": "SPI0 (dual channel)",
        "transceiver": "SN65HVD230",
        "pins": [
            {"adapter": "VCC",   "function": "3.3V Power",   "gpio": None, "pin": 1,  "notes": "3.3V supply"},
            {"adapter": "GND",   "function": "Ground",       "gpio": None, "pin": 6,  "notes": "Common ground"},
            {"adapter": "MOSI",  "function": "SPI0 MOSI",    "gpio": 10,   "pin": 19, "notes": "Master Out"},
            {"adapter": "MISO",  "function": "SPI0 MISO",    "gpio": 9,    "pin": 21, "notes": "Master In"},
            {"adapter": "SCK",   "function": "SPI0 SCLK",    "gpio": 11,   "pin": 23, "notes": "SPI Clock"},
            {"adapter": "CS0",   "function": "SPI0 CE0",     "gpio": 8,    "pin": 24, "notes": "Channel 0 CS"},
            {"adapter": "CS1",   "function": "SPI0 CE1",     "gpio": 7,    "pin": 26, "notes": "Channel 1 CS"},
            {"adapter": "INT0",  "function": "Interrupt CH0", "gpio": 25,  "pin": 22, "notes": "CAN0 interrupt"},
            {"adapter": "INT1",  "function": "Interrupt CH1", "gpio": 24,  "pin": 18, "notes": "CAN1 interrupt"},
        ],
    },
    "PiCAN 2": {
        "interface": "SPI0",
        "transceiver": "TJA1051",
        "pins": [
            {"adapter": "VCC",  "function": "3.3V Power",   "gpio": None, "pin": 1,  "notes": "3.3V supply"},
            {"adapter": "GND",  "function": "Ground",       "gpio": None, "pin": 6,  "notes": "Common ground"},
            {"adapter": "MOSI", "function": "SPI0 MOSI",    "gpio": 10,   "pin": 19, "notes": "Master Out"},
            {"adapter": "MISO", "function": "SPI0 MISO",    "gpio": 9,    "pin": 21, "notes": "Master In"},
            {"adapter": "SCK",  "function": "SPI0 SCLK",    "gpio": 11,   "pin": 23, "notes": "SPI Clock"},
            {"adapter": "CS",   "function": "SPI0 CE0",     "gpio": 8,    "pin": 24, "notes": "Chip Select"},
            {"adapter": "INT",  "function": "Interrupt",     "gpio": 25,   "pin": 22, "notes": "CAN interrupt"},
        ],
    },
    "Peak PCAN-USB": {
        "interface": "USB",
        "transceiver": "Integrated",
        "pins": [],
    },
    "Kvaser Leaf Light": {
        "interface": "USB",
        "transceiver": "Integrated",
        "pins": [],
    },
    "None (Ethernet Only)": {
        "interface": "N/A",
        "transceiver": "N/A",
        "pins": [],
    },
    "Simulated": {
        "interface": "Virtual",
        "transceiver": "N/A",
        "pins": [],
    },
}

# ── MCU peripheral pin mappings ─────────────────────────────────────

MCU_PERIPHERALS: dict[str, dict[str, str]] = {
    "Infineon TC397": {
        "can": "MultiCAN+ (8 nodes: CAN0-7)",
        "spi": "QSPI0-5",
        "uart": "ASCLIN0-11",
        "ethernet": "GbE MAC (RGMII)",
        "i2c": "I2C0-1",
    },
    "NXP S32K344": {
        "can": "FlexCAN0-5 (FD capable)",
        "spi": "LPSPI0-4",
        "uart": "LPUART0-11",
        "ethernet": "ENET (RMII)",
        "i2c": "LPI2C0-1",
    },
    "STM32H743": {
        "can": "FDCAN1 (PD0/PD1), FDCAN2 (PB12/PB13)",
        "spi": "SPI1 (PA5/PB4/PB5)",
        "uart": "USART1 (PA9/PA10)",
        "ethernet": "ETH RMII (PA1/PA2/PA7)",
        "i2c": "I2C1 (PB6/PB7)",
    },
    "STM32F446RE": {
        "can": "CAN1 (PB8/PB9), CAN2 (PB5/PB6)",
        "spi": "SPI1 (PA5/PA6/PA7)",
        "uart": "USART2 (PA2/PA3)",
        "ethernet": "N/A",
        "i2c": "I2C1 (PB6/PB7)",
    },
    "Renesas RH850/U2A": {
        "can": "RSCFD (6 channels, FD capable)",
        "spi": "CSIH0-3",
        "uart": "RLIN30-3",
        "ethernet": "Ethernet AVB",
        "i2c": "RIIC0-1",
    },
    "Renesas R-Car H3": {
        "can": "CAN-FD (2 channels)",
        "spi": "MSIOF0-3",
        "uart": "SCIF0-5",
        "ethernet": "AVB (GbE RGMII)",
        "i2c": "I2C0-6",
    },
    "ESP32-S3": {
        "can": "TWAI0 (GPIO4/GPIO5)",
        "spi": "SPI2 (any GPIO), SPI3 (any GPIO)",
        "uart": "UART0-2 (any GPIO)",
        "ethernet": "N/A (Wi-Fi only)",
        "i2c": "I2C0-1 (any GPIO)",
    },
    "Raspberry Pi Pico (RP2040)": {
        "can": "N/A (external MCP2515)",
        "spi": "SPI0 (GP16-19), SPI1 (GP10-13)",
        "uart": "UART0 (GP0/GP1), UART1 (GP4/GP5)",
        "ethernet": "N/A",
        "i2c": "I2C0 (GP4/GP5), I2C1 (GP6/GP7)",
    },
    "Arduino Mega (ATmega2560)": {
        "can": "N/A (external MCP2515)",
        "spi": "SPI (D50-53)",
        "uart": "UART0-3",
        "ethernet": "N/A (W5500 shield)",
        "i2c": "TWI (D20/D21)",
    },
    "Generic / Simulated": {
        "can": "Virtual CAN",
        "spi": "Virtual SPI",
        "uart": "Virtual UART",
        "ethernet": "Virtual Ethernet",
        "i2c": "Virtual I2C",
    },
}


# ── 6-zone preset (standard automotive zonal layout) ──────────────────
# ECU tuples: (name, domain, bus, security, mcu_model, asil, node_addr)

_6ZONE_PRESET: dict[str, dict] = {
    "Front Left": {
        "desc": "Front-left zone: headlights, ABS, front camera",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Raspberry Pi 4 Model B",
        "ip_address": "192.168.1.11",
        "can_adapter": "MCP2515 (SPI)",
        "ecus": [
            ("ABS Module",          "Chassis", "CAN-FD",   "PQC (ML-DSA-65)",   "Infineon TC397",  "ASIL-D", "0x101"),
            ("LED Headlight L",     "Body",    "LIN",      "None",               "Raspberry Pi Pico (RP2040)", "QM", "0x210"),
            ("Front Camera Module", "ADAS",    "Ethernet", "PQC (ML-DSA-65)",   "STM32H743",       "ASIL-B", "0x310"),
        ],
    },
    "Front Right": {
        "desc": "Front-right zone: engine, headlights, ESP",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Raspberry Pi 4 Model B",
        "ip_address": "192.168.1.12",
        "can_adapter": "MCP2515 (SPI)",
        "ecus": [
            ("Engine Control Module", "Powertrain", "CAN-FD",   "PQC (ML-DSA-65)", "Infineon TC397",  "ASIL-D", "0x100"),
            ("LED Headlight R",       "Body",       "LIN",      "None",             "Raspberry Pi Pico (RP2040)", "QM", "0x211"),
            ("ESP Module",            "Chassis",    "CAN-FD",   "PQC (ML-DSA-65)", "Infineon TC397",  "ASIL-D", "0x102"),
        ],
    },
    "Central Left": {
        "desc": "Central-left zone: driver door, head unit, steering",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Raspberry Pi 3 Model B+",
        "ip_address": "192.168.1.13",
        "can_adapter": "Waveshare CAN HAT",
        "ecus": [
            ("Driver Door Module",    "Body",          "LIN",      "None",             "STM32F446RE", "QM",     "0x220"),
            ("Head Unit",             "Infotainment",  "Ethernet", "PQC (ML-DSA-65)", "Renesas R-Car H3", "QM", "0x400"),
            ("Electric Power Steering", "Chassis",     "CAN-FD",   "PQC (ML-DSA-65)", "NXP S32K344", "ASIL-D", "0x103"),
        ],
    },
    "Central Right": {
        "desc": "Central-right zone: passenger door, cluster, radar",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Raspberry Pi 3 Model B+",
        "ip_address": "192.168.1.14",
        "can_adapter": "Waveshare CAN HAT",
        "ecus": [
            ("Passenger Door Module", "Body",          "LIN",      "None",              "STM32F446RE", "QM",     "0x221"),
            ("Instrument Cluster",    "Infotainment",  "Ethernet", "Classic MAC (HMAC)", "STM32H743",  "ASIL-B", "0x401"),
            ("Radar Module",          "ADAS",          "CAN-FD",   "PQC (ML-DSA-65)",  "Renesas RH850/U2A", "ASIL-B", "0x311"),
        ],
    },
    "Rear Left": {
        "desc": "Rear-left zone: tail light, BMS, rear camera",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Simulated (Virtual)",
        "ip_address": "",
        "can_adapter": "Simulated",
        "ecus": [
            ("Tail Light L",        "Body",    "LIN",      "None",              "Raspberry Pi Pico (RP2040)", "QM",     "0x212"),
            ("Battery Mgmt System", "Powertrain", "CAN",   "Classic MAC (HMAC)", "NXP S32K344",  "ASIL-C", "0x104"),
            ("Rear Camera Module",  "ADAS",    "Ethernet", "PQC (ML-DSA-65)",  "STM32H743",    "ASIL-B", "0x312"),
        ],
    },
    "Rear Right": {
        "desc": "Rear-right zone: tail light, parking assist, TCU",
        "backbone": "Ethernet 100BASE-T1",
        "hardware": "Simulated (Virtual)",
        "ip_address": "",
        "can_adapter": "Simulated",
        "ecus": [
            ("Tail Light R",           "Body",          "LIN",      "None",              "Raspberry Pi Pico (RP2040)", "QM", "0x213"),
            ("Parking Assist ECU",     "ADAS",          "CAN",      "Classic MAC (HMAC)", "STM32F446RE", "ASIL-A", "0x313"),
            ("Telematics Control Unit", "Infotainment", "Ethernet", "PQC (ML-DSA-65)",  "ESP32-S3",    "QM",     "0x402"),
        ],
    },
}


# ── Data classes ──────────────────────────────────────────────────────

@dataclass
class RouteConfig:
    """A single PDU route between two ECUs."""
    uid: str = field(default_factory=lambda: uuid.uuid4().hex[:6])
    source_ecu_uid: str = ""
    source_ecu_name: str = ""
    source_zone: str = ""
    source_bus: str = "CAN"
    dest_ecu_uid: str = ""
    dest_ecu_name: str = ""
    dest_zone: str = ""
    dest_bus: str = "CAN"
    pdu_id: str = "0x0000"
    pdu_name: str = ""
    mode: str = "IF"           # IF or TP
    secoc: str = "None"        # "PQC (ML-DSA-65)" / "Classic MAC (HMAC)" / "None"
    direction: str = "Tx"      # Tx / Rx / Bidirectional
    status: str = "Configured" # Configured / Active / Error


@dataclass
class ECUConfig:
    """A single ECU within a zone controller — full HW/SW description."""
    uid: str = field(default_factory=lambda: uuid.uuid4().hex[:6])
    name: str = ""
    domain: str = "Body"                    # key in ECU_DOMAINS
    local_bus: str = "CAN"                  # one of LOCAL_BUS_OPTIONS
    security: str = "PQC (ML-DSA-65)"       # one of SECURITY_OPTIONS
    mcu_model: str = "Generic / Simulated"  # key in MCU_CATALOGUE
    asil: str = "QM"                        # one of ASIL_LEVELS
    node_address: str = "0x00"              # CAN ID or Ethernet IP
    status: str = "Simulated"               # one of ECU_STATUS_OPTIONS


@dataclass
class GatewayConfig:
    """Central Zone Gateway (HPCU) — the device running this software."""
    uid: str = "gateway"
    name: str = "Zone Gateway (HPCU)"
    description: str = "Central Zone Gateway — Ethernet backbone hub, PDU router"
    hardware: str = "Raspberry Pi 4 Model B"
    ip_address: str = "192.168.1.1"
    can_adapter: str = "MCP2515 (SPI)"
    backbone: str = "Ethernet 100BASE-T1"
    role: str = "HPCU"  # High Performance Compute Unit


@dataclass
class ZoneController:
    """A zone controller (Raspberry Pi) with hosted ECUs and hardware config."""
    uid: str = field(default_factory=lambda: uuid.uuid4().hex[:8])
    name: str = ""
    description: str = ""
    backbone: str = "Ethernet 100BASE-T1"   # one of BACKBONE_OPTIONS
    hardware: str = "Simulated (Virtual)"   # one of ZONE_HARDWARE_OPTIONS
    ip_address: str = ""                    # e.g. "192.168.1.11"
    can_adapter: str = "Simulated"          # one of CAN_ADAPTER_OPTIONS
    ecus: list[ECUConfig] = field(default_factory=list)


# ── Topology Model (singleton) ────────────────────────────────────────

class TopologyModel(QObject):
    """
    Observable data model for the zonal gateway topology.

    Starts EMPTY (0 zones).  Use ``load_6zone_preset()`` for a
    standard automotive layout, or build zones one-by-one.
    """

    changed = Signal()

    def __init__(self, parent: QObject | None = None):
        super().__init__(parent)
        self._gateway = GatewayConfig()
        self._zones: list[ZoneController] = []
        self._routes: list[RouteConfig] = []
        # Start empty — user builds the topology

    # ── Gateway (HPCU) ──────────────────────────────────────────────────

    @property
    def gateway(self) -> GatewayConfig:
        return self._gateway

    def update_gateway(self, **kwargs) -> None:
        """Update any writable field(s) of the gateway by keyword."""
        for key, val in kwargs.items():
            if val is not None and hasattr(self._gateway, key):
                setattr(self._gateway, key, val)
        self.changed.emit()

    # ── Queries ────────────────────────────────────────────────────────

    @property
    def zones(self) -> list[ZoneController]:
        return list(self._zones)

    @property
    def zone_count(self) -> int:
        return len(self._zones)

    @property
    def total_ecus(self) -> int:
        return sum(len(z.ecus) for z in self._zones)

    def zone_by_uid(self, uid: str) -> ZoneController | None:
        for z in self._zones:
            if z.uid == uid:
                return z
        return None

    # ── Commands ───────────────────────────────────────────────────────

    def clear(self) -> None:
        """Remove all zones — back to empty canvas."""
        self._zones.clear()
        self.changed.emit()

    def load_6zone_preset(self) -> None:
        """Load the standard 6-zone automotive layout with real ECU hardware."""
        self._zones.clear()
        for name, cfg in _6ZONE_PRESET.items():
            ecus = [
                ECUConfig(
                    name=n, domain=d, local_bus=lb, security=sec,
                    mcu_model=mcu, asil=asil, node_address=addr,
                )
                for n, d, lb, sec, mcu, asil, addr in cfg["ecus"]
            ]
            self._zones.append(ZoneController(
                name=name,
                description=cfg["desc"],
                backbone=cfg["backbone"],
                hardware=cfg.get("hardware", "Simulated (Virtual)"),
                ip_address=cfg.get("ip_address", ""),
                can_adapter=cfg.get("can_adapter", "Simulated"),
                ecus=ecus,
            ))
        self.changed.emit()

    def load_1zone_demo(self) -> None:
        """Load the 1-zone demonstration layout for thesis/demo."""
        self._zones.clear()
        for name, cfg in _1ZONE_DEMO.items():
            ecus = [
                ECUConfig(
                    name=n, domain=d, local_bus=lb, security=sec,
                    mcu_model=mcu, asil=asil, node_address=addr,
                )
                for n, d, lb, sec, mcu, asil, addr in cfg["ecus"]
            ]
            self._zones.append(ZoneController(
                name=name,
                description=cfg["desc"],
                backbone=cfg["backbone"],
                hardware=cfg.get("hardware", "Simulated (Virtual)"),
                ip_address=cfg.get("ip_address", ""),
                can_adapter=cfg.get("can_adapter", "Simulated"),
                ecus=ecus,
            ))
        self.changed.emit()

    @property
    def pqc_secured_ecus(self) -> int:
        """Count of ECUs using PQC authentication."""
        count = 0
        for zone in self._zones:
            for ecu in zone.ecus:
                if "PQC" in ecu.security:
                    count += 1
        return count

    @property
    def active_domains(self) -> set[str]:
        """Set of unique ECU domains currently in use."""
        domains = set()
        for zone in self._zones:
            for ecu in zone.ecus:
                domains.add(ecu.domain)
        return domains

    @property
    def backbone_status(self) -> str:
        """Check if any real hardware backbone is configured."""
        for zone in self._zones:
            if zone.hardware != "Simulated (Virtual)" and zone.ip_address:
                return "Online"
        if self._zones:
            return "Simulated"
        return "Offline"

    def add_zone(self, name: str, description: str = "",
                 backbone: str = "Ethernet 100BASE-T1",
                 hardware: str = "Simulated (Virtual)",
                 ip_address: str = "",
                 can_adapter: str = "Simulated") -> ZoneController:
        z = ZoneController(
            name=name, description=description, backbone=backbone,
            hardware=hardware, ip_address=ip_address, can_adapter=can_adapter,
        )
        self._zones.append(z)
        self.changed.emit()
        return z

    def remove_zone(self, uid: str) -> bool:
        before = len(self._zones)
        self._zones = [z for z in self._zones if z.uid != uid]
        if len(self._zones) < before:
            self.changed.emit()
            return True
        return False

    def update_zone(self, uid: str, **kwargs) -> None:
        """Update any writable field(s) of a zone by keyword."""
        z = self.zone_by_uid(uid)
        if not z:
            return
        for key, val in kwargs.items():
            if val is not None and hasattr(z, key):
                setattr(z, key, val)
        self.changed.emit()

    def set_zone_ecus(self, uid: str, ecus: list[ECUConfig]) -> None:
        """Replace the entire ECU list for a zone."""
        z = self.zone_by_uid(uid)
        if z:
            z.ecus = list(ecus)
            self.changed.emit()

    def add_ecu(self, zone_uid: str, name: str = "New ECU",
                domain: str = "Body", local_bus: str = "CAN",
                security: str = "PQC (ML-DSA-65)",
                mcu_model: str = "Generic / Simulated",
                asil: str = "QM",
                node_address: str = "0x00") -> ECUConfig | None:
        z = self.zone_by_uid(zone_uid)
        if not z:
            return None
        ecu = ECUConfig(
            name=name, domain=domain, local_bus=local_bus,
            security=security, mcu_model=mcu_model,
            asil=asil, node_address=node_address,
        )
        z.ecus.append(ecu)
        self.changed.emit()
        return ecu

    def rename_zone(self, uid: str, new_name: str) -> None:
        """Convenience shortcut for renaming a zone."""
        self.update_zone(uid, name=new_name)

    def remove_ecu(self, zone_uid: str, ecu_uid: str) -> bool:
        z = self.zone_by_uid(zone_uid)
        if not z:
            return False
        before = len(z.ecus)
        z.ecus = [e for e in z.ecus if e.uid != ecu_uid]
        if len(z.ecus) < before:
            self.changed.emit()
            return True
        return False

    # ── Route management ────────────────────────────────────────────────

    @property
    def routes(self) -> list[RouteConfig]:
        return list(self._routes)

    def add_route(self, route: RouteConfig) -> None:
        self._routes.append(route)
        self.changed.emit()

    def remove_route(self, uid: str) -> bool:
        before = len(self._routes)
        self._routes = [r for r in self._routes if r.uid != uid]
        if len(self._routes) < before:
            self.changed.emit()
            return True
        return False

    def clear_routes(self) -> None:
        self._routes.clear()
        self.changed.emit()

    def auto_generate_routes(self) -> int:
        """Generate routes for all ECU pairs across zones. Returns count."""
        self._routes.clear()
        # Collect all (ecu, zone_name) pairs
        all_ecus: list[tuple[ECUConfig, str]] = []
        for zone in self._zones:
            for ecu in zone.ecus:
                all_ecus.append((ecu, zone.name))

        _SEC_RANK = {"PQC (ML-DSA-65)": 2, "Classic MAC (HMAC)": 1, "None": 0}
        counter = 0
        for i, (ecu_a, zone_a) in enumerate(all_ecus):
            for j in range(i + 1, len(all_ecus)):
                ecu_b, zone_b = all_ecus[j]
                # Determine SecOC: highest security of the pair
                sec_a = _SEC_RANK.get(ecu_a.security, 0)
                sec_b = _SEC_RANK.get(ecu_b.security, 0)
                secoc = ecu_a.security if sec_a >= sec_b else ecu_b.security
                # Determine mode
                uses_eth = "Ethernet" in ecu_a.local_bus or "Ethernet" in ecu_b.local_bus
                uses_pqc = "PQC" in secoc
                mode = "TP" if (uses_eth or uses_pqc) else "IF"

                pdu_id = f"0x{0x100 + counter:04X}"
                self._routes.append(RouteConfig(
                    source_ecu_uid=ecu_a.uid,
                    source_ecu_name=ecu_a.name,
                    source_zone=zone_a,
                    source_bus=ecu_a.local_bus,
                    dest_ecu_uid=ecu_b.uid,
                    dest_ecu_name=ecu_b.name,
                    dest_zone=zone_b,
                    dest_bus=ecu_b.local_bus,
                    pdu_id=pdu_id,
                    pdu_name=f"{ecu_a.name}_to_{ecu_b.name}",
                    mode=mode,
                    secoc=secoc,
                    direction="Bidirectional",
                    status="Configured",
                ))
                counter += 1

        self.changed.emit()
        return counter


# ── Module-level singleton ────────────────────────────────────────────

topology = TopologyModel()
