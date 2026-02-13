"""
Ethernet Gateway simulation page.

Visualises the full ECU → Gateway → ECU flow:
  1. ECU-A authenticates a PDU (PQC / Classic)
  2. Gateway receives, verifies, re-authenticates, and forwards
  3. ECU-B receives and verifies

Each step is shown as an animated card with live hex output.
Light professional theme.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.data_models import AuthMode
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


class GatewaySimPage(QWidget):
    """Ethernet Gateway ECU-A → GW → ECU-B simulation."""

    def __init__(self, bridge: BackendBridge, parent=None):
        super().__init__(parent)
        self._bridge = bridge
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
            "Ethernet Gateway Simulation",
            color=Theme.color.ORANGE,
            font_size=Theme.font.SIZE_H1,
            bold=True,
        )
        lay.addWidget(hdr)

        sub = QLabel(
            "ECU-A  \u2192  Ethernet Gateway  \u2192  ECU-B   (SecOC-protected)"
        )
        sub.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_H3}px;"
        )
        lay.addWidget(sub)

        # Mode toggle
        mode_row = QHBoxLayout()
        mode_lbl = QLabel("Auth Mode:")
        mode_lbl.setStyleSheet(f"color:{Theme.color.TEXT_SECONDARY};")
        mode_row.addWidget(mode_lbl)
        self._toggle = AnimatedToggle()
        mode_row.addWidget(self._toggle)
        mode_row.addStretch()
        lay.addLayout(mode_row)

        # ── Three-column ECU diagram ─────────────────────────────────
        cols = QHBoxLayout()
        cols.setSpacing(12)

        self._ecu_a = self._build_node_card(
            "ECU-A (Sender)", Theme.color.CYAN, "\U0001F697"
        )
        self._gateway = self._build_node_card(
            "Ethernet Gateway", Theme.color.ORANGE, "\U0001F310"
        )
        self._ecu_b = self._build_node_card(
            "ECU-B (Receiver)", Theme.color.GREEN, "\U0001F699"
        )

        cols.addWidget(self._ecu_a, stretch=1)
        cols.addWidget(self._build_flow_arrow("\u2192"), stretch=0)
        cols.addWidget(self._gateway, stretch=1)
        cols.addWidget(self._build_flow_arrow("\u2192"), stretch=0)
        cols.addWidget(self._ecu_b, stretch=1)
        lay.addLayout(cols)

        # ── Action buttons ───────────────────────────────────────────
        btn_row = QHBoxLayout()
        self._btn_full = IconButton("Run Full Flow", "\u25B6\uFE0F", "primary")
        self._btn_step = IconButton("Step-by-Step", "\u23ED\uFE0F", "outline")
        self._btn_reset = IconButton("Reset", "\U0001F504", "danger")
        btn_row.addWidget(self._btn_full)
        btn_row.addWidget(self._btn_step)
        btn_row.addStretch()
        btn_row.addWidget(self._btn_reset)
        lay.addLayout(btn_row)

        # Log
        self._log = LogConsole("Gateway Simulation Log")
        lay.addWidget(self._log)

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    def _build_node_card(self, title: str, color: str, icon: str) -> StyledCard:
        card = StyledCard(title, accent_color=color)
        icon_lbl = QLabel(icon)
        icon_lbl.setStyleSheet("font-size: 32px; border:none;")
        icon_lbl.setAlignment(Qt.AlignCenter)
        card.card_layout.addWidget(icon_lbl)

        status = StatusBadge("IDLE", "idle")
        card._status = status
        card.card_layout.addWidget(status, alignment=Qt.AlignCenter)

        hex_view = HexViewer("PDU Data")
        card._hex = hex_view
        card.card_layout.addWidget(hex_view)
        return card

    def _build_flow_arrow(self, symbol: str) -> QWidget:
        w = QWidget()
        w.setFixedWidth(40)
        lay = QVBoxLayout(w)
        lay.setAlignment(Qt.AlignCenter)
        lbl = QLabel(symbol)
        lbl.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: 24px;
            font-weight: bold;
            border: none;
        """)
        lbl.setAlignment(Qt.AlignCenter)
        lay.addWidget(lbl)
        return w

    # ── Signals ──────────────────────────────────────────────────────

    def _connect_signals(self):
        self._btn_full.clicked.connect(self._run_full_flow)
        self._btn_reset.clicked.connect(self._reset)

    def _mode(self) -> AuthMode:
        return AuthMode.PQC if self._toggle.is_pqc else AuthMode.CLASSIC

    # ── Full flow ────────────────────────────────────────────────────

    def _run_full_flow(self):
        mode = self._mode()
        mode_str = "PQC" if mode == AuthMode.PQC else "Classic"
        self._log.log(f"=== Full Gateway Flow [{mode_str}] ===", "info")

        # Step 1 – ECU-A authenticates
        self._ecu_a._status.set_preset("info", "SIGNING\u2026")
        data = bytes.fromhex("10203040506070809A0B0C0D0E0F")
        status, elapsed = self._bridge.authenticate(0, data, mode)
        self._log.log(
            f"ECU-A auth: {status} ({elapsed:.0f} \u00b5s)",
            "success" if status == "E_OK" else "error",
        )
        hex_str, length = self._bridge.get_secured_pdu(0)
        self._ecu_a._hex.set_data(hex_str, length)
        self._ecu_a._status.set_preset("success", f"SIGNED ({elapsed:.0f}\u00b5s)")

        # Step 2 – Transmit to Gateway
        self._gateway._status.set_preset("info", "RECEIVING\u2026")
        tx_status = self._bridge.transmit(0)
        recv_status, rx_id, rx_len = self._bridge.receive()
        self._log.log(
            f"Gateway receive: {recv_status} [id={rx_id}, len={rx_len}]", "info"
        )
        rx_hex, _, sec_len = self._bridge.get_secured_rx_pdu(rx_id)
        self._gateway._hex.set_data(rx_hex, sec_len)

        # Step 3 – Gateway verifies
        vr = self._bridge.verify(rx_id, mode)
        if vr.is_ok:
            self._gateway._status.set_preset(
                "success", f"VERIFIED ({vr.elapsed_us:.0f}\u00b5s)"
            )
            self._log.log(
                f"Gateway verify: OK ({vr.elapsed_us:.0f} \u00b5s)", "success"
            )
        else:
            self._gateway._status.set_preset("error", vr.raw_message)
            self._log.log(f"Gateway verify FAILED: {vr.raw_message}", "error")
            return

        # Step 4 – Gateway re-authenticates + forwards
        auth_bytes, auth_len = self._bridge.get_auth_pdu(rx_id)
        if auth_len > 0:
            status2, elapsed2 = self._bridge.authenticate(
                1, auth_bytes[:auth_len], mode
            )
            self._log.log(
                f"Gateway re-auth: {status2} ({elapsed2:.0f} \u00b5s)", "info"
            )

        # Step 5 – ECU-B receives
        self._ecu_b._status.set_preset("success", "RECEIVED \u2713")
        self._ecu_b._hex.set_raw_bytes(
            auth_bytes[:auth_len] if auth_len > 0 else data
        )
        self._log.log("ECU-B received authenticated payload", "success")
        self._log.log("=== Flow Complete ===", "success")

    def _reset(self):
        for card in [self._ecu_a, self._gateway, self._ecu_b]:
            card._status.set_preset("idle", "IDLE")
            card._hex.clear()
        self._log.log("Simulation reset", "warning")
