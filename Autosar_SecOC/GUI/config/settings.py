"""
Application-wide settings and constants.

Contains paths, default values, PDU configuration presets,
and any other 'magic numbers' used across the application.
"""

import os
import sys
from pathlib import Path


class Settings:
    """Immutable application settings."""

    # ── Application metadata ─────────────────────────────────────────
    APP_NAME    = "AUTOSAR Ethernet Gateway Configurator"
    APP_VERSION = "2.0.0"
    WINDOW_MIN_W = 1280
    WINDOW_MIN_H = 800

    # ── Thesis / University ───────────────────────────────────────────
    UNIVERSITY   = "Ho Chi Minh City University of Technology"
    UNIVERSITY_SHORT = "HCMUT"
    THESIS_TITLE = "Ethernet Gateway with PQC Secure Key Management"
    THESIS_YEAR  = 2026

    # ── Paths ────────────────────────────────────────────────────────
    PROJECT_ROOT = Path(__file__).resolve().parent.parent          # GUI/
    SECOC_ROOT   = PROJECT_ROOT.parent                              # Autosar_SecOC/

    @classmethod
    def dll_path(cls) -> str:
        """Return the platform-appropriate shared library path."""
        if sys.platform == "win32":
            return str(cls.SECOC_ROOT / "build" / "SecOCLibShared.dll")
        return str(cls.SECOC_ROOT / "build" / "libSecOCLibShared.so")

    @classmethod
    def liboqs_dll_path(cls) -> str:
        """Return the liboqs DLL path (Windows only)."""
        return str(cls.SECOC_ROOT / "external" / "liboqs" / "build" / "bin" / "liboqs.dll")

    # ── SecOC constants (mirror C config) ────────────────────────────
    MAX_PDU_SIZE         = 8192
    DEFAULT_FRESHNESS_LEN = 64     # bits
    DEFAULT_MAC_LEN       = 32     # bits (classic)

    # ── PDU presets for quick testing ────────────────────────────────
    PDU_PRESETS = {
        "CAN – Brake Signal": {
            "config_id": 0,
            "data": "01 A3 4F 00 FF 22 11 88",
            "pdu_type": "IF",
            "description": "ABS brake pressure command",
        },
        "CAN – Steering Angle": {
            "config_id": 1,
            "data": "02 B7 8C 3E 00 00 55 AA",
            "pdu_type": "IF",
            "description": "Electric power steering angle",
        },
        "Ethernet – ADAS Payload": {
            "config_id": 0,
            "data": "10 20 30 40 50 60 70 80 90 A0 B0 C0 D0 E0 F0 FF",
            "pdu_type": "TP",
            "description": "ADAS camera frame metadata",
        },
        "Ethernet – V2X Message": {
            "config_id": 1,
            "data": "AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99",
            "pdu_type": "TP",
            "description": "Vehicle-to-Everything broadcast",
        },
    }

    # ── PQC algorithm metadata ───────────────────────────────────────
    PQC_ALGORITHMS = {
        "ML-KEM-768": {
            "full_name": "Module-Lattice-Based Key-Encapsulation Mechanism",
            "standard": "NIST FIPS 203",
            "pk_bytes": 1184,
            "sk_bytes": 2400,
            "ct_bytes": 1088,
            "ss_bytes": 32,
            "security_level": "NIST Level 3 (~AES-192)",
        },
        "ML-DSA-65": {
            "full_name": "Module-Lattice-Based Digital Signature Algorithm",
            "standard": "NIST FIPS 204",
            "pk_bytes": 1952,
            "sk_bytes": 4032,
            "sig_bytes": 3309,
            "security_level": "NIST Level 3 (~AES-192)",
        },
    }

    # ── Attack simulation presets ────────────────────────────────────
    ATTACK_TYPES = {
        "Replay Attack": {
            "description": "Resend a previously captured secured PDU",
            "target": "freshness",
            "icon": "🔁",
        },
        "Tamper Payload": {
            "description": "Modify the authentic PDU data bytes",
            "target": "payload",
            "icon": "✏️",
        },
        "Forge Signature": {
            "description": "Replace the authenticator with random bytes",
            "target": "authenticator",
            "icon": "🔓",
        },
        "Bit-Flip Attack": {
            "description": "Flip a single bit in the secured PDU",
            "target": "bitflip",
            "icon": "⚡",
        },
    }

    # ── Log buffer ─────────────────────────────────────────────────────
    LOG_BUFFER_SIZE = 500

    # ── Refresh intervals (ms) ───────────────────────────────────────
    TIMER_STATUS_REFRESH  = 1000
    TIMER_PERF_SAMPLE     = 500
    TIMER_ANIMATION_FRAME = 16    # ~60 fps

