"""
Centralized signal hub — the event bus for inter-page communication.

Any page or service can emit or subscribe to these signals without
importing or knowing about each other.

Usage
-----
>>> from core.signal_hub import hub
>>> hub.auth_completed.connect(my_handler)
>>> hub.auth_completed.emit("ML-DSA Sign", 250.0)
"""

from __future__ import annotations

from PySide6.QtCore import QObject, Signal


class _SignalHub(QObject):
    """Singleton event bus.  One instance is created at module level."""

    # ── Authentication / Verification ──────────────────────────────────
    auth_completed   = Signal(str, float)       # (operation_name, elapsed_us)
    verify_completed = Signal(str, float, bool)  # (operation, elapsed_us, success)
    operation_error  = Signal(str, str)          # (operation, error_message)

    # ── Key exchange ───────────────────────────────────────────────────
    key_exchange_state_changed = Signal(int, int)  # (session_id, new_state)

    # ── System state ───────────────────────────────────────────────────
    system_initialized = Signal(bool)            # backend loaded?
    mode_changed       = Signal(str)             # "CLASSIC" | "PQC"
    pdu_transmitted    = Signal(int, float)      # (config_id, elapsed_us)
    pdu_received       = Signal(int, float)      # (config_id, elapsed_us)

    # ── Attack simulation ──────────────────────────────────────────────
    attack_result      = Signal(str, bool, str)  # (attack_name, detected, detail)

    # ── Metrics ────────────────────────────────────────────────────────
    perf_sample_added  = Signal(str, float)      # (operation, elapsed_us)

    # ── Log ────────────────────────────────────────────────────────────
    log_message        = Signal(str, str)        # (level, message)


# Module-level singleton
hub = _SignalHub()

