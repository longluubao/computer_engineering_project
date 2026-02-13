"""
Attack Simulation page.

Demonstrates how SecOC (both Classic and PQC) detects and prevents:
  • Replay attacks (stale freshness)
  • Payload tampering (altered authenticator)
  • Bit-flip attacks
  • Signature forgery

Each attack follows:  Authenticate → Tamper → Verify → Show result.
Light professional theme.
"""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.data_models import AuthMode, AttackResult
from widgets.common import (
    AnimatedToggle,
    GlowLabel,
    IconButton,
    StatusBadge,
    StyledCard,
    Separator,
)
from widgets.hex_viewer import HexViewer
from widgets.log_console import LogConsole


class AttackSimPage(QWidget):
    """Interactive attack simulation panel."""

    def __init__(self, bridge: BackendBridge, parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._attack_cards: dict[str, _AttackCard] = {}
        self._build_ui()
        self._connect_signals()

    # ── UI ───────────────────────────────────────────────────────────

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(24, 20, 24, 20)
        root.setSpacing(16)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet(Theme.stylesheet.SCROLL_AREA)
        scroll.setFrameShape(QFrame.NoFrame)

        container = QWidget()
        lay = QVBoxLayout(container)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(16)

        # Header
        hdr = GlowLabel(
            "Attack Simulation Lab",
            color=Theme.color.RED,
            font_size=Theme.font.SIZE_H1,
            bold=True,
        )
        lay.addWidget(hdr)

        sub = QLabel(
            "Test SecOC resilience against common automotive network attacks. "
            "Each attack first authenticates a valid PDU, then tampers with it, "
            "and verifies that the modification is detected."
        )
        sub.setWordWrap(True)
        sub.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
        )
        lay.addWidget(sub)

        # Mode row
        mode_row = QHBoxLayout()
        mode_lbl = QLabel("Auth Mode:")
        mode_lbl.setStyleSheet(f"color:{Theme.color.TEXT_SECONDARY};")
        mode_row.addWidget(mode_lbl)
        self._toggle = AnimatedToggle()
        mode_row.addWidget(self._toggle)

        self._btn_all = IconButton("Run All Attacks", "\U0001F9EA", "danger")
        mode_row.addStretch()
        mode_row.addWidget(self._btn_all)
        lay.addLayout(mode_row)

        # ── Attack cards grid ────────────────────────────────────────
        grid = QGridLayout()
        grid.setSpacing(16)

        for i, (name, info) in enumerate(Settings.ATTACK_TYPES.items()):
            ac = _AttackCard(name, info)
            self._attack_cards[name] = ac
            grid.addWidget(ac, i // 2, i % 2)

        lay.addLayout(grid)

        # Hex comparison
        hex_row = QHBoxLayout()
        hex_row.setSpacing(16)
        self._hex_before = HexViewer("Before Attack")
        self._hex_after = HexViewer("After Attack")
        hex_row.addWidget(self._hex_before)
        hex_row.addWidget(self._hex_after)
        lay.addLayout(hex_row)

        # Log
        self._log = LogConsole("Attack Simulation Log")
        lay.addWidget(self._log)

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    # ── Signals ──────────────────────────────────────────────────────

    def _connect_signals(self):
        self._btn_all.clicked.connect(self._run_all)
        for name, card in self._attack_cards.items():
            card.run_clicked.connect(lambda n=name: self._run_attack(n))

    def _mode(self) -> AuthMode:
        return AuthMode.PQC if self._toggle.is_pqc else AuthMode.CLASSIC

    # ── Attack execution ─────────────────────────────────────────────

    def _run_attack(self, name: str):
        mode = self._mode()
        mode_str = "PQC" if mode == AuthMode.PQC else "Classic"
        card = self._attack_cards[name]
        data = bytes.fromhex("01A34F00FF221188")

        self._log.log(f"\u25B6 {name} [{mode_str}]", "warning")

        # 1) Authenticate
        status, _ = self._bridge.authenticate(0, data, mode)
        if status != "E_OK":
            self._log.log(f"  Auth failed: {status}", "error")
            card.set_result(False)
            return

        # Get hex before attack
        hex_before, len_before = self._bridge.get_secured_pdu(0)
        self._hex_before.set_data(hex_before, len_before)

        # 2) Transmit + receive
        self._bridge.transmit(0)
        self._bridge.receive()

        # 3) Tamper
        info = Settings.ATTACK_TYPES[name]
        target = info["target"]
        if target == "freshness":
            self._bridge.alter_freshness(0)
        elif target in ("payload", "authenticator", "bitflip"):
            self._bridge.alter_authenticator(0)

        # Get hex after attack
        hex_after, _, sec_len = self._bridge.get_secured_rx_pdu(0)
        self._hex_after.set_data(hex_after, sec_len)

        # 4) Verify
        vr = self._bridge.verify(0, mode)
        detected = not vr.is_ok
        card.set_result(detected)

        if detected:
            self._log.log(
                f"  \u2713 DETECTED \u2014 {vr.raw_message} ({vr.elapsed_us:.0f} \u00b5s)",
                "success",
            )
        else:
            self._log.log(
                f"  \u2717 NOT DETECTED \u2014 {vr.raw_message}", "error"
            )

    def _run_all(self):
        self._log.log(
            "\u2550\u2550\u2550 Running all attack simulations \u2550\u2550\u2550",
            "warning",
        )
        for name in Settings.ATTACK_TYPES:
            self._run_attack(name)
        self._log.log(
            "\u2550\u2550\u2550 All attacks complete \u2550\u2550\u2550", "info"
        )


# ── Private helper widget ────────────────────────────────────────────


class _AttackCard(StyledCard):
    """Small card representing one attack type with a Run button."""

    from PySide6.QtCore import Signal

    run_clicked = Signal()

    def __init__(self, name: str, info: dict, parent=None):
        super().__init__(accent_color=Theme.color.RED, parent=parent)
        self._name = name

        # Title row
        title_row = QHBoxLayout()
        icon = QLabel(info.get("icon", "\u26A0\uFE0F"))
        icon.setStyleSheet("font-size: 24px; border: none;")
        lbl = QLabel(name)
        lbl.setStyleSheet(f"""
            color: {Theme.color.TEXT_PRIMARY};
            font-size: {Theme.font.SIZE_H3}px;
            font-weight: bold;
            border: none;
        """)
        title_row.addWidget(icon)
        title_row.addWidget(lbl)
        title_row.addStretch()
        self.card_layout.addLayout(title_row)

        # Description
        desc = QLabel(info.get("description", ""))
        desc.setWordWrap(True)
        desc.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; "
            f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
        )
        self.card_layout.addWidget(desc)

        # Result badge + button
        bottom = QHBoxLayout()
        self._badge = StatusBadge("Not Run", "idle")
        self._btn = IconButton("Run", "\u25B6\uFE0F", "danger")
        self._btn.clicked.connect(self.run_clicked.emit)
        bottom.addWidget(self._badge)
        bottom.addStretch()
        bottom.addWidget(self._btn)
        self.card_layout.addLayout(bottom)

    def set_result(self, detected: bool):
        if detected:
            self._badge.set_preset("success", "\u2713 Detected")
        else:
            self._badge.set_preset("error", "\u2717 Bypassed")
