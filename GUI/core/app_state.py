"""
Reactive application state — the single source of truth.

Any page or service can read *and* subscribe to state changes.
Mutations go through explicit methods so we can emit signals
and (optionally) persist snapshots.

Usage
-----
>>> from core.app_state import state
>>> state.auth_mode                       # read
>>> state.set_auth_mode(AuthMode.PQC)     # write + emit
>>> state.changed.connect(my_handler)     # subscribe
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import List

from PySide6.QtCore import QObject, Signal

from core.data_models import (
    AuthMode,
    PduType,
    PerfSample,
    VerificationResult,
    VerificationStatus,
    AttackResult,
    KeyExchangeState,
)


@dataclass
class _Counters:
    """Cumulative counters for the current session."""
    tx_total: int = 0
    rx_total: int = 0
    verify_ok: int = 0
    verify_fail: int = 0
    attacks_detected: int = 0
    attacks_total: int = 0
    key_exchanges: int = 0


class AppState(QObject):
    """Singleton reactive state container."""

    # ── Signals ────────────────────────────────────────────────────────
    changed           = Signal()           # any mutation
    mode_changed      = Signal(str)        # "CLASSIC" | "PQC"
    init_changed      = Signal(bool)       # backend loaded?
    counters_changed  = Signal()
    perf_sample_added = Signal(object)     # PerfSample

    # ── Construction ───────────────────────────────────────────────────

    def __init__(self):
        super().__init__()
        self._backend_ready: bool = False
        self._auth_mode: AuthMode = AuthMode.CLASSIC
        self._counters = _Counters()
        self._perf_history: List[PerfSample] = []
        self._last_verify: VerificationResult | None = None
        self._ke_state: KeyExchangeState = KeyExchangeState.IDLE

    # ── Read-only properties ──────────────────────────────────────────

    @property
    def backend_ready(self) -> bool:
        return self._backend_ready

    @property
    def auth_mode(self) -> AuthMode:
        return self._auth_mode

    @property
    def counters(self) -> _Counters:
        return self._counters

    @property
    def perf_history(self) -> List[PerfSample]:
        return list(self._perf_history)

    @property
    def last_verify(self) -> VerificationResult | None:
        return self._last_verify

    @property
    def ke_state(self) -> KeyExchangeState:
        return self._ke_state

    @property
    def success_rate(self) -> float:
        """Verification success rate as 0.0–100.0."""
        total = self._counters.verify_ok + self._counters.verify_fail
        if total == 0:
            return 100.0
        return (self._counters.verify_ok / total) * 100.0

    @property
    def avg_latency_us(self) -> float:
        """Average latency (µs) of the last 50 samples."""
        recent = self._perf_history[-50:]
        if not recent:
            return 0.0
        return sum(s.elapsed_us for s in recent) / len(recent)

    # ── Mutations ─────────────────────────────────────────────────────

    def set_backend_ready(self, ready: bool) -> None:
        self._backend_ready = ready
        self.init_changed.emit(ready)
        self.changed.emit()

    def set_auth_mode(self, mode: AuthMode) -> None:
        self._auth_mode = mode
        self.mode_changed.emit(mode.name)
        self.changed.emit()

    def record_tx(self) -> None:
        self._counters.tx_total += 1
        self.counters_changed.emit()
        self.changed.emit()

    def record_rx(self) -> None:
        self._counters.rx_total += 1
        self.counters_changed.emit()
        self.changed.emit()

    def record_verify(self, result: VerificationResult) -> None:
        self._last_verify = result
        if result.is_ok:
            self._counters.verify_ok += 1
        else:
            self._counters.verify_fail += 1
        self.counters_changed.emit()
        self.changed.emit()

    def record_attack(self, result: AttackResult) -> None:
        self._counters.attacks_total += 1
        if result.detected:
            self._counters.attacks_detected += 1
        self.counters_changed.emit()
        self.changed.emit()

    def record_key_exchange(self, new_state: KeyExchangeState) -> None:
        self._ke_state = new_state
        if new_state == KeyExchangeState.ESTABLISHED:
            self._counters.key_exchanges += 1
        self.counters_changed.emit()
        self.changed.emit()

    def add_perf_sample(self, sample: PerfSample) -> None:
        self._perf_history.append(sample)
        # Keep bounded
        if len(self._perf_history) > 500:
            self._perf_history = self._perf_history[-500:]
        self.perf_sample_added.emit(sample)
        self.changed.emit()

    def reset(self) -> None:
        """Reset all counters and history (new session)."""
        self._counters = _Counters()
        self._perf_history.clear()
        self._last_verify = None
        self._ke_state = KeyExchangeState.IDLE
        self.counters_changed.emit()
        self.changed.emit()


# Module-level singleton
state = AppState()

