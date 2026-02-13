"""
SecOC Authentication & Verification page.

Interactive panel where the user can:
  1. Select a PDU preset or enter custom hex data
  2. Choose Classic (HMAC) or PQC (ML-DSA) mode
  3. Authenticate → inspect Secured PDU hex dump
  4. Copy TX → RX   → Verify
  5. View verification result
"""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QScrollArea,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.data_models import AuthMode, VerificationStatus
from widgets.common import (
    AnimatedToggle,
    IconButton,
    SectionHeader,
    StatusBadge,
    StyledCard,
    Separator,
)
from widgets.hex_viewer import HexViewer
from widgets.log_console import LogConsole


class SecOCAuthPage(QWidget):
    """Interactive SecOC Authenticate → Verify workflow."""

    # Emitted whenever an operation completes so other pages can react
    operation_done = Signal(str, float)  # (operation_name, elapsed_us)

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

        # ── Control strip ────────────────────────────────────────────
        lay.addWidget(self._build_controls())

        # ── TX / RX split ────────────────────────────────────────────
        split = QHBoxLayout()
        split.setSpacing(16)
        split.addWidget(self._build_tx_panel(), stretch=1)
        split.addWidget(self._build_rx_panel(), stretch=1)
        lay.addLayout(split)

        # ── Log console ──────────────────────────────────────────────
        self._log = LogConsole("Operation Log")
        lay.addWidget(self._log)

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    # ── Control strip ────────────────────────────────────────────────

    def _build_controls(self) -> QWidget:
        card = StyledCard("Configuration")

        row = QHBoxLayout()
        row.setSpacing(16)

        # Preset selector
        lbl_preset = QLabel("Preset:")
        lbl_preset.setStyleSheet(f"color: {Theme.color.TEXT_SECONDARY}; border:none;")
        self._combo_preset = QComboBox()
        self._combo_preset.setStyleSheet(Theme.stylesheet.COMBO)
        self._combo_preset.addItem("\u2014 Custom \u2014")
        for name in Settings.PDU_PRESETS:
            self._combo_preset.addItem(name)

        # Config ID
        lbl_id = QLabel("Config ID:")
        lbl_id.setStyleSheet(f"color: {Theme.color.TEXT_SECONDARY}; border:none;")
        self._spin_id = QSpinBox()
        self._spin_id.setRange(0, 7)
        self._spin_id.setStyleSheet(Theme.stylesheet.INPUT)
        self._spin_id.setFixedWidth(70)

        # Hex data input
        lbl_data = QLabel("Hex Data:")
        lbl_data.setStyleSheet(f"color: {Theme.color.TEXT_SECONDARY}; border:none;")
        self._input_data = QLineEdit("01 A3 4F 00 FF 22 11 88")
        self._input_data.setStyleSheet(Theme.stylesheet.INPUT)
        self._input_data.setPlaceholderText("e.g. 01 A3 4F 00 FF 22 11 88")

        # Mode toggle
        lbl_mode = QLabel("Mode:")
        lbl_mode.setStyleSheet(f"color: {Theme.color.TEXT_SECONDARY}; border:none;")
        self._toggle = AnimatedToggle()

        for w in [
            lbl_preset, self._combo_preset,
            lbl_id, self._spin_id,
            lbl_data, self._input_data,
            lbl_mode, self._toggle,
        ]:
            row.addWidget(w)

        card.card_layout.addLayout(row)
        return card

    # ── TX panel ─────────────────────────────────────────────────────

    def _build_tx_panel(self) -> QWidget:
        card = StyledCard("Transmit (TX)", accent_color=Theme.color.CYAN)

        # Buttons
        btn_row = QHBoxLayout()
        self._btn_auth = IconButton("Authenticate", "\U0001F50F", "primary")
        self._btn_get_secured = IconButton("Get Secured PDU", "\U0001F4E6", "outline")
        btn_row.addWidget(self._btn_auth)
        btn_row.addWidget(self._btn_get_secured)
        card.card_layout.addLayout(btn_row)

        # Status
        self._tx_status = StatusBadge("Idle", "idle")
        card.card_layout.addWidget(self._tx_status, alignment=Qt.AlignLeft)

        # Hex viewer
        self._hex_tx = HexViewer("Secured PDU (TX)")
        card.card_layout.addWidget(self._hex_tx)

        return card

    # ── RX panel ─────────────────────────────────────────────────────

    def _build_rx_panel(self) -> QWidget:
        card = StyledCard("Receive (RX)", accent_color=Theme.color.GREEN)

        btn_row = QHBoxLayout()
        self._btn_copy_tx_rx = IconButton("Copy TX \u2192 RX", "\U0001F4CB", "outline")
        self._btn_verify = IconButton("Verify", "\u2705", "primary")
        btn_row.addWidget(self._btn_copy_tx_rx)
        btn_row.addWidget(self._btn_verify)
        card.card_layout.addLayout(btn_row)

        self._rx_status = StatusBadge("Idle", "idle")
        card.card_layout.addWidget(self._rx_status, alignment=Qt.AlignLeft)

        self._hex_rx = HexViewer("Secured PDU (RX)")
        card.card_layout.addWidget(self._hex_rx)

        # Verified data output
        self._hex_auth = HexViewer("Authenticated PDU (output)")
        card.card_layout.addWidget(self._hex_auth)

        return card

    # ── Signal wiring ────────────────────────────────────────────────

    def _connect_signals(self):
        self._combo_preset.currentIndexChanged.connect(self._on_preset_changed)
        self._btn_auth.clicked.connect(self._on_authenticate)
        self._btn_get_secured.clicked.connect(self._on_get_secured)
        self._btn_copy_tx_rx.clicked.connect(self._on_copy_tx_rx)
        self._btn_verify.clicked.connect(self._on_verify)

    # ── Slots ────────────────────────────────────────────────────────

    def _on_preset_changed(self, index: int):
        if index <= 0:
            return
        name = self._combo_preset.currentText()
        preset = Settings.PDU_PRESETS.get(name, {})
        self._spin_id.setValue(preset.get("config_id", 0))
        self._input_data.setText(preset.get("data", ""))

    def _current_mode(self) -> AuthMode:
        return AuthMode.PQC if self._toggle.is_pqc else AuthMode.CLASSIC

    def _parse_hex(self) -> bytes:
        raw = self._input_data.text().strip().replace("-", " ")
        tokens = raw.split()
        return bytes(int(t, 16) for t in tokens if t)

    def _on_authenticate(self):
        try:
            data = self._parse_hex()
            cfg = self._spin_id.value()
            mode = self._current_mode()
            mode_str = "PQC (ML-DSA-65)" if mode == AuthMode.PQC else "Classic (HMAC)"

            self._log.log(
                f"Authenticating {len(data)} bytes  [config={cfg}, mode={mode_str}]",
                "info",
            )

            status, elapsed = self._bridge.authenticate(cfg, data, mode)

            if status == "E_OK":
                self._tx_status.set_preset(
                    "success", f"\u2713 {status} ({elapsed:.0f} \u00b5s)"
                )
                self._log.log(
                    f"Authentication OK \u2014 {elapsed:.1f} \u00b5s", "success"
                )
            else:
                self._tx_status.set_preset("error", status)
                self._log.log(f"Authentication failed: {status}", "error")

            self.operation_done.emit(f"Auth ({mode_str})", elapsed)

        except Exception as e:
            self._tx_status.set_preset("error", "Parse Error")
            self._log.log(f"Error: {e}", "error")

    def _on_get_secured(self):
        try:
            cfg = self._spin_id.value()
            hex_str, length = self._bridge.get_secured_pdu(cfg)
            self._hex_tx.set_data(hex_str, length)
            self._log.log(f"Secured PDU: {length} bytes", "info")
        except Exception as e:
            self._log.log(f"Error getting secured PDU: {e}", "error")

    def _on_copy_tx_rx(self):
        """Transmit the secured PDU from TX → RX via Ethernet loopback."""
        try:
            cfg = self._spin_id.value()
            status = self._bridge.transmit(cfg)
            self._log.log(f"Transmit \u2192 {status}", "info")

            recv_status, rx_id, rx_len = self._bridge.receive()
            self._log.log(
                f"Receive  \u2190 {recv_status}  [rx_id={rx_id}, len={rx_len}]", "info"
            )

            hex_str, str_len, sec_len = self._bridge.get_secured_rx_pdu(rx_id)
            self._hex_rx.set_data(hex_str, sec_len)
            self._rx_status.set_preset("info", f"Received ({sec_len} B)")

        except Exception as e:
            self._log.log(f"TX\u2192RX error: {e}", "error")

    def _on_verify(self):
        try:
            cfg = self._spin_id.value()
            mode = self._current_mode()
            mode_str = "PQC" if mode == AuthMode.PQC else "Classic"

            vr = self._bridge.verify(cfg, mode)
            self._log.log(
                f"Verify [{mode_str}]: {vr.raw_message} \u2014 {vr.elapsed_us:.1f} \u00b5s",
                "success" if vr.is_ok else "error",
            )

            if vr.is_ok:
                self._rx_status.set_preset(
                    "success",
                    f"\u2713 Authentic ({vr.elapsed_us:.0f} \u00b5s)",
                )
                # Show authenticated PDU
                auth_bytes, auth_len = self._bridge.get_auth_pdu(cfg)
                if auth_len > 0:
                    self._hex_auth.set_raw_bytes(auth_bytes)
            else:
                self._rx_status.set_preset("error", vr.raw_message)

            self.operation_done.emit(f"Verify ({mode_str})", vr.elapsed_us)

        except Exception as e:
            self._log.log(f"Verify error: {e}", "error")
