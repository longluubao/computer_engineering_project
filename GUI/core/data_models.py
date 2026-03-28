"""
Lightweight data-classes used throughout the GUI.

These are *pure Python* – no Qt dependency – so they can be used
in background threads, tests, or serialised to JSON trivially.
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Optional


# ── Enumerations ─────────────────────────────────────────────────────

class PduType(Enum):
    """Maps to SecOC_PduType_Type in C."""
    IF = auto()
    TP = auto()


class AuthMode(Enum):
    """Classic HMAC vs Post-Quantum signatures."""
    CLASSIC = auto()
    PQC     = auto()


class VerificationStatus(Enum):
    """Possible verification outcomes."""
    PENDING            = auto()
    SUCCESS            = auto()
    FRESHNESS_FAILURE  = auto()
    AUTH_BUILD_FAILURE = auto()
    VERIFICATION_FAIL  = auto()
    ERROR              = auto()


class KeyExchangeState(Enum):
    """Mirror of PQC_KE_STATE_* in C."""
    IDLE        = 0
    INITIATED   = 1
    RESPONDED   = 2
    ESTABLISHED = 3
    FAILED      = 4


# ── Data classes ─────────────────────────────────────────────────────

@dataclass
class PduData:
    """Represents a single PDU in transit."""
    config_id: int
    raw_bytes: bytes = b""
    secured_hex: str = ""
    auth_hex: str = ""
    freshness_hex: str = ""
    authenticator_hex: str = ""
    pdu_type: PduType = PduType.IF
    auth_mode: AuthMode = AuthMode.CLASSIC
    timestamp: float = field(default_factory=time.time)

    @property
    def raw_hex(self) -> str:
        return " ".join(f"{b:02X}" for b in self.raw_bytes)

    @property
    def raw_len(self) -> int:
        return len(self.raw_bytes)


@dataclass
class PqcKeyInfo:
    """ML-DSA / ML-KEM key metadata for display."""
    algorithm: str = "ML-DSA-65"
    public_key_preview: str = ""
    secret_key_preview: str = ""
    key_size_bytes: int = 0
    generated: bool = False
    timestamp: float = 0.0


@dataclass
class VerificationResult:
    """Outcome of a verify operation."""
    config_id: int
    status: VerificationStatus = VerificationStatus.PENDING
    raw_message: str = ""
    elapsed_us: float = 0.0
    auth_mode: AuthMode = AuthMode.CLASSIC
    timestamp: float = field(default_factory=time.time)

    @property
    def is_ok(self) -> bool:
        return self.status == VerificationStatus.SUCCESS


@dataclass
class PerfSample:
    """A single performance measurement."""
    operation: str = ""          # e.g. "ML-DSA Sign", "HMAC Generate"
    elapsed_us: float = 0.0
    data_size: int = 0
    timestamp: float = field(default_factory=time.time)

    @property
    def throughput_kbps(self) -> float:
        if self.elapsed_us <= 0:
            return 0.0
        return (self.data_size * 8) / self.elapsed_us * 1_000  # kbit/s


@dataclass
class AttackResult:
    """Result of an attack simulation."""
    attack_name: str = ""
    detected: bool = False
    detail: str = ""
    elapsed_us: float = 0.0
    timestamp: float = field(default_factory=time.time)

