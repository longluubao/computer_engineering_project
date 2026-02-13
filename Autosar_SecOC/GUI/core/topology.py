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
        self._zones: list[ZoneController] = []
        # Start empty — user builds the topology

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


# ── Module-level singleton ────────────────────────────────────────────

topology = TopologyModel()
