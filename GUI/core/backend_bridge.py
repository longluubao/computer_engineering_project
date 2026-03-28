"""
Bridge between the Python/Qt GUI and the SecOC C shared library.

Loads SecOCLibShared.dll / libSecOCLibShared.so via ctypes and wraps
every GUIInterface_* function with Pythonic typing & error handling.
"""

from __future__ import annotations

import ctypes
import os
import sys
import time
from pathlib import Path
from typing import Optional, Tuple

from config.settings import Settings
from .data_models import (
    AuthMode,
    PduData,
    PduType,
    PerfSample,
    VerificationResult,
    VerificationStatus,
    AttackResult,
)


class BackendBridge:
    """
    Singleton-style wrapper around the C shared library.

    Usage
    -----
    >>> bridge = BackendBridge()
    >>> bridge.load()
    >>> bridge.init()
    >>> result = bridge.authenticate(0, bytes.fromhex("01A34F00FF221188"))
    """

    # ── Class-level state ────────────────────────────────────────────
    _lib: Optional[ctypes.CDLL] = None
    _loaded: bool = False

    # ── Loading ──────────────────────────────────────────────────────

    def load(self, dll_path: Optional[str] = None) -> None:
        """Load the shared library.  Raises OSError on failure."""
        if self._loaded:
            return

        path = dll_path or Settings.dll_path()

        # On Windows, pre-load liboqs so it's available when SecOCLib loads
        if sys.platform == "win32":
            oqs_path = Settings.liboqs_dll_path()
            if os.path.isfile(oqs_path):
                ctypes.CDLL(oqs_path)

        self._lib = ctypes.CDLL(path)
        self._setup_prototypes()
        self._loaded = True

    @property
    def is_loaded(self) -> bool:
        return self._loaded

    # ── Prototype declarations ───────────────────────────────────────

    def _setup_prototypes(self) -> None:
        """Declare argument & return types for every C function."""
        lib = self._lib

        # GUIInterface_init(void) -> void
        lib.GUIInterface_init.argtypes = []
        lib.GUIInterface_init.restype = None

        # GUIInterface_authenticate(configId, *data, len) -> char*
        lib.GUIInterface_authenticate.argtypes = [
            ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_uint8,
        ]
        lib.GUIInterface_authenticate.restype = ctypes.c_char_p

        # GUIInterface_authenticate_PQC(configId, *data, len) -> char*
        lib.GUIInterface_authenticate_PQC.argtypes = [
            ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_uint8,
        ]
        lib.GUIInterface_authenticate_PQC.restype = ctypes.c_char_p

        # GUIInterface_verify(configId) -> char*
        lib.GUIInterface_verify.argtypes = [ctypes.c_uint8]
        lib.GUIInterface_verify.restype = ctypes.c_char_p

        # GUIInterface_verify_PQC(configId) -> char*
        lib.GUIInterface_verify_PQC.argtypes = [ctypes.c_uint8]
        lib.GUIInterface_verify_PQC.restype = ctypes.c_char_p

        # GUIInterface_getSecuredPDU(configId, *len) -> char*
        lib.GUIInterface_getSecuredPDU.argtypes = [
            ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8),
        ]
        lib.GUIInterface_getSecuredPDU.restype = ctypes.c_char_p

        # GUIInterface_getSecuredRxPDU(configId, *len, *securedlen) -> char*
        lib.GUIInterface_getSecuredRxPDU.argtypes = [
            ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8),
        ]
        lib.GUIInterface_getSecuredRxPDU.restype = ctypes.c_char_p

        # GUIInterface_getAuthPdu(configId, *len) -> char*
        lib.GUIInterface_getAuthPdu.argtypes = [
            ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8),
        ]
        lib.GUIInterface_getAuthPdu.restype = ctypes.c_char_p

        # GUIInterface_alterFreshness(configId) -> void
        lib.GUIInterface_alterFreshness.argtypes = [ctypes.c_uint8]
        lib.GUIInterface_alterFreshness.restype = None

        # GUIInterface_alterAuthenticator(configId) -> void
        lib.GUIInterface_alterAuthenticator.argtypes = [ctypes.c_uint8]
        lib.GUIInterface_alterAuthenticator.restype = None

        # GUIInterface_transmit(configId) -> char*
        lib.GUIInterface_transmit.argtypes = [ctypes.c_uint8]
        lib.GUIInterface_transmit.restype = ctypes.c_char_p

        # GUIInterface_receive(*rxId, *finalRxLen) -> char*
        lib.GUIInterface_receive.argtypes = [
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_uint8),
        ]
        lib.GUIInterface_receive.restype = ctypes.c_char_p

    # ── High-level API ───────────────────────────────────────────────

    def init(self) -> None:
        """Initialize SecOC + Ethernet backend."""
        self._ensure_loaded()
        self._lib.GUIInterface_init()

    def authenticate(
        self,
        config_id: int,
        data: bytes,
        mode: AuthMode = AuthMode.CLASSIC,
    ) -> Tuple[str, float]:
        """
        Sign / MAC a PDU.  Returns (status_string, elapsed_microseconds).
        """
        self._ensure_loaded()
        buf = (ctypes.c_uint8 * len(data))(*data)
        t0 = time.perf_counter_ns()
        if mode == AuthMode.PQC:
            result = self._lib.GUIInterface_authenticate_PQC(config_id, buf, len(data))
        else:
            result = self._lib.GUIInterface_authenticate(config_id, buf, len(data))
        elapsed = (time.perf_counter_ns() - t0) / 1_000  # → µs
        return (result.decode() if result else "E_NOT_OK"), elapsed

    def verify(
        self,
        config_id: int,
        mode: AuthMode = AuthMode.CLASSIC,
    ) -> VerificationResult:
        """Verify a previously-received secured PDU."""
        self._ensure_loaded()
        t0 = time.perf_counter_ns()
        if mode == AuthMode.PQC:
            raw = self._lib.GUIInterface_verify_PQC(config_id)
        else:
            raw = self._lib.GUIInterface_verify(config_id)
        elapsed = (time.perf_counter_ns() - t0) / 1_000
        msg = raw.decode() if raw else "E_NOT_OK"

        status = self._parse_verify_status(msg)
        return VerificationResult(
            config_id=config_id,
            status=status,
            raw_message=msg,
            elapsed_us=elapsed,
            auth_mode=mode,
        )

    def get_secured_pdu(self, config_id: int) -> Tuple[str, int]:
        """Return (hex_string, length) for the Tx secured PDU."""
        self._ensure_loaded()
        length = ctypes.c_uint8(0)
        result = self._lib.GUIInterface_getSecuredPDU(config_id, ctypes.byref(length))
        hex_str = result.decode() if result else ""
        return hex_str, length.value

    def get_secured_rx_pdu(self, config_id: int) -> Tuple[str, int, int]:
        """Return (hex_string, str_len, secured_len) for the Rx secured PDU."""
        self._ensure_loaded()
        str_len = ctypes.c_uint8(0)
        sec_len = ctypes.c_uint8(0)
        result = self._lib.GUIInterface_getSecuredRxPDU(
            config_id, ctypes.byref(str_len), ctypes.byref(sec_len)
        )
        hex_str = result.decode() if result else ""
        return hex_str, str_len.value, sec_len.value

    def get_auth_pdu(self, config_id: int) -> Tuple[bytes, int]:
        """Return (raw_bytes, length) of the authenticated Rx PDU."""
        self._ensure_loaded()
        length = ctypes.c_uint8(0)
        result = self._lib.GUIInterface_getAuthPdu(config_id, ctypes.byref(length))
        if result and length.value > 0:
            raw = ctypes.string_at(result, length.value)
            return raw, length.value
        return b"", 0

    def alter_freshness(self, config_id: int) -> None:
        """Tamper with the freshness value (attack simulation)."""
        self._ensure_loaded()
        self._lib.GUIInterface_alterFreshness(config_id)

    def alter_authenticator(self, config_id: int) -> None:
        """Tamper with the MAC / signature (attack simulation)."""
        self._ensure_loaded()
        self._lib.GUIInterface_alterAuthenticator(config_id)

    def transmit(self, config_id: int) -> str:
        """Transmit the secured PDU over Ethernet."""
        self._ensure_loaded()
        result = self._lib.GUIInterface_transmit(config_id)
        return result.decode() if result else "E_NOT_OK"

    def receive(self) -> Tuple[str, int, int]:
        """Receive a secured PDU.  Returns (status, rx_id, final_rx_len)."""
        self._ensure_loaded()
        rx_id = ctypes.c_uint8(0)
        final_len = ctypes.c_uint8(0)
        result = self._lib.GUIInterface_receive(
            ctypes.byref(rx_id), ctypes.byref(final_len)
        )
        msg = result.decode() if result else "E_NOT_OK"
        return msg, rx_id.value, final_len.value

    # ── Attack helpers ───────────────────────────────────────────────

    def simulate_replay_attack(
        self, config_id: int, mode: AuthMode = AuthMode.CLASSIC
    ) -> AttackResult:
        """Replay: alter freshness, then verify."""
        self.alter_freshness(config_id)
        vr = self.verify(config_id, mode)
        return AttackResult(
            attack_name="Replay Attack",
            detected=not vr.is_ok,
            detail=vr.raw_message,
            elapsed_us=vr.elapsed_us,
        )

    def simulate_tamper_attack(
        self, config_id: int, mode: AuthMode = AuthMode.CLASSIC
    ) -> AttackResult:
        """Tamper: alter authenticator, then verify."""
        self.alter_authenticator(config_id)
        vr = self.verify(config_id, mode)
        return AttackResult(
            attack_name="Tamper Payload",
            detected=not vr.is_ok,
            detail=vr.raw_message,
            elapsed_us=vr.elapsed_us,
        )

    # ── Private helpers ──────────────────────────────────────────────

    def _ensure_loaded(self) -> None:
        if not self._loaded:
            raise RuntimeError(
                "BackendBridge not loaded. Call bridge.load() first."
            )

    @staticmethod
    def _parse_verify_status(msg: str) -> VerificationStatus:
        msg_lower = msg.lower()
        if msg == "E_OK":
            return VerificationStatus.SUCCESS
        if "freshness" in msg_lower:
            return VerificationStatus.FRESHNESS_FAILURE
        if "not authentic" in msg_lower or "sig failed" in msg_lower:
            return VerificationStatus.VERIFICATION_FAIL
        if "build" in msg_lower:
            return VerificationStatus.AUTH_BUILD_FAILURE
        return VerificationStatus.ERROR

