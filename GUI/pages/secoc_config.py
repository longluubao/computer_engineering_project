"""
SecOC Configuration page.

Configures AUTOSAR SecOC (Secure Onboard Communication) parameters:
  - Global authentication settings
  - Per-ECU security configuration
  - Freshness management
  - Key status overview

Based on AUTOSAR R21-11 SecOC specification.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QSpinBox,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.topology import topology, ECU_DOMAINS
from widgets.common import (
    IconButton,
    SectionHeader,
    StatusBadge,
    StyledCard,
    Separator,
)


# ── SecOC algorithm options ─────────────────────────────────────────────

AUTH_ALGORITHMS = {
    "PQC (ML-DSA-65)": {
        "desc": "NIST FIPS 204 - Module-Lattice-Based Digital Signatures",
        "level": "NIST Level 3 (192-bit quantum security)",
        "mac_bytes": 64,
        "pqc": True,
    },
    "Classic MAC (HMAC-SHA256)": {
        "desc": "Traditional HMAC with SHA-256",
        "level": "Classical security (256-bit)",
        "mac_bytes": 32,
        "pqc": False,
    },
    "CMAC (AES-128)": {
        "desc": "Cipher-based MAC with AES-128",
        "level": "Classical security (128-bit)",
        "mac_bytes": 16,
        "pqc": False,
    },
}

FRESHNESS_TYPES = {
    "Counter (Sync)": {
        "desc": "Synchronized counter across all ECUs",
        "bits": 16,
    },
    "Timestamp": {
        "desc": "Real-time clock based freshness",
        "bits": 32,
    },
    "Data-Derived": {
        "desc": "Freshness from data content",
        "bits": 8,
    },
}


# ── Helper functions ────────────────────────────────────────────────────

def _kv_row(key: str, value: str, accent: str = "") -> QHBoxLayout:
    """Create a key-value row for configuration display."""
    accent = accent or Theme.color.TEXT_PRIMARY
    row = QHBoxLayout()
    row.setSpacing(4)
    row.setContentsMargins(0, 2, 0, 2)
    lbl = QLabel(key)
    lbl.setStyleSheet(
        f"color:{Theme.color.TEXT_MUTED};"
        f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
    )
    val = QLabel(value)
    val.setStyleSheet(
        f"color:{accent}; font-size:{Theme.font.SIZE_SMALL}px;"
        f"font-weight:600; border:none;"
    )
    row.addWidget(lbl)
    row.addStretch()
    row.addWidget(val)
    return row


class SecOCConfigPage(QWidget):
    """SecOC configuration panel for the zonal gateway."""

    config_changed = Signal()

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

        # ── Header ──────────────────────────────────────────────────
        lay.addLayout(self._build_header())

        # ── Global Configuration ────────────────────────────────────
        lay.addWidget(self._build_global_config())

        # ── ECU Security Overview ───────────────────────────────────
        lay.addWidget(self._build_ecu_overview())

        # ── Key Status ──────────────────────────────────────────────
        lay.addWidget(self._build_key_status())

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    def _build_header(self) -> QHBoxLayout:
        """Build the page header with title and badges."""
        row = QHBoxLayout()
        row.setSpacing(12)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)

        title = QLabel("SecOC Configuration")
        title.setStyleSheet(f"""
            color: {Theme.color.PQC_PURPLE};
            font-size: {Theme.font.SIZE_H1}px;
            font-weight: bold;  border: none;
        """)

        sub = QLabel(
            "AUTOSAR Secure Onboard Communication  \u00b7  "
            "R21-11 Specification"
        )
        sub.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;  border: none;
        """)

        title_col.addWidget(title)
        title_col.addWidget(sub)
        row.addLayout(title_col, stretch=1)

        row.addWidget(StatusBadge("FIPS 203/204", "pqc"))
        row.addWidget(StatusBadge("AUTOSAR R21-11", "info"))

        return row

    def _build_global_config(self) -> QWidget:
        """Build the global SecOC configuration card."""
        card = StyledCard("Global SecOC Settings")

        lay = QVBoxLayout()
        lay.setSpacing(12)

        # ── Authentication Algorithm ───────────────────────────────
        algo_row = QHBoxLayout()
        algo_row.setSpacing(8)

        algo_lbl = QLabel("Authentication Algorithm:")
        algo_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; border:none;"
        )
        algo_row.addWidget(algo_lbl)

        self._algo_combo = QComboBox()
        self._algo_combo.addItems(AUTH_ALGORITHMS.keys())
        self._algo_combo.setStyleSheet(Theme.stylesheet.INPUT)
        self._algo_combo.setFixedWidth(220)
        self._algo_combo.currentTextChanged.connect(self._on_algo_changed)
        algo_row.addWidget(self._algo_combo)

        algo_row.addStretch()
        lay.addLayout(algo_row)

        # ── Algorithm Info ─────────────────────────────────────────
        self._algo_info = QLabel()
        self._algo_info.setWordWrap(True)
        self._algo_info.setStyleSheet(
            f"color:{Theme.color.TEXT_MUTED};"
            f"font-size:{Theme.font.SIZE_SMALL}px;"
            f"padding:8px 12px;"
            f"background:{Theme.color.BG_ELEVATED};"
            f"border-radius:6px; border:none;"
        )
        self._update_algo_info()
        lay.addWidget(self._algo_info)

        lay.addWidget(Separator())

        # ── Freshness Configuration ────────────────────────────────
        fresh_row = QHBoxLayout()
        fresh_row.setSpacing(8)

        fresh_lbl = QLabel("Freshness Type:")
        fresh_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; border:none;"
        )
        fresh_row.addWidget(fresh_lbl)

        self._fresh_combo = QComboBox()
        self._fresh_combo.addItems(FRESHNESS_TYPES.keys())
        self._fresh_combo.setStyleSheet(Theme.stylesheet.INPUT)
        self._fresh_combo.setFixedWidth(180)
        fresh_row.addWidget(self._fresh_combo)

        fresh_row.addStretch()
        lay.addLayout(fresh_row)

        # ── MAC Length ─────────────────────────────────────────────
        mac_row = QHBoxLayout()
        mac_row.setSpacing(8)

        mac_lbl = QLabel("MAC Truncation (bits):")
        mac_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; border:none;"
        )
        mac_row.addWidget(mac_lbl)

        self._mac_spin = QSpinBox()
        self._mac_spin.setRange(8, 128)
        self._mac_spin.setValue(64)
        self._mac_spin.setSingleStep(8)
        self._mac_spin.setStyleSheet(Theme.stylesheet.INPUT)
        self._mac_spin.setFixedWidth(80)
        mac_row.addWidget(self._mac_spin)

        mac_hint = QLabel("(Truncated from full MAC for CAN payload)")
        mac_hint.setStyleSheet(
            f"color:{Theme.color.TEXT_MUTED};"
            f"font-size:{Theme.font.SIZE_TINY}px; border:none;"
        )
        mac_row.addWidget(mac_hint)

        mac_row.addStretch()
        lay.addLayout(mac_row)

        card.setContentLayout(lay)
        return card

    def _build_ecu_overview(self) -> QWidget:
        """Build the ECU security overview card."""
        card = StyledCard("ECU Security Overview")

        lay = QVBoxLayout()
        lay.setSpacing(8)

        # ── Summary Stats ──────────────────────────────────────────
        stats_row = QHBoxLayout()
        stats_row.setSpacing(16)

        self._total_ecus = QLabel("0")
        self._pqc_ecus = QLabel("0")
        self._classic_ecus = QLabel("0")
        self._no_sec_ecus = QLabel("0")

        for label, value, color in [
            ("Total ECUs", self._total_ecus, Theme.color.TEXT_PRIMARY),
            ("PQC Secured", self._pqc_ecus, Theme.color.PQC_PURPLE),
            ("Classic MAC", self._classic_ecus, Theme.color.CYAN),
            ("No Security", self._no_sec_ecus, Theme.color.TEXT_MUTED),
        ]:
            stat = QVBoxLayout()
            stat.setSpacing(2)
            v = QLabel(value.text())
            v.setStyleSheet(
                f"color:{color}; font-size:{Theme.font.SIZE_H2}px;"
                f"font-weight:bold; border:none;"
            )
            setattr(self, f"_{label.lower().replace(' ', '_')}_value", v)
            l = QLabel(label)
            l.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED};"
                f"font-size:{Theme.font.SIZE_TINY}px; border:none;"
            )
            stat.addWidget(v, alignment=Qt.AlignCenter)
            stat.addWidget(l, alignment=Qt.AlignCenter)
            stats_row.addLayout(stat)

        stats_row.addStretch()
        lay.addLayout(stats_row)

        lay.addWidget(Separator())

        # ── ECU List ───────────────────────────────────────────────
        self._ecu_list_container = QWidget()
        self._ecu_list_lay = QVBoxLayout(self._ecu_list_container)
        self._ecu_list_lay.setContentsMargins(0, 0, 0, 0)
        self._ecu_list_lay.setSpacing(4)

        lay.addWidget(self._ecu_list_container)
        self._refresh_ecu_list()

        card.setContentLayout(lay)
        return card

    def _build_key_status(self) -> QWidget:
        """Build the key status card."""
        card = StyledCard("Key Management Status")

        lay = QVBoxLayout()
        lay.setSpacing(8)

        # ── Key Generation Status ──────────────────────────────────
        grid = QGridLayout()
        grid.setSpacing(8)

        self._key_labels = {}
        key_items = [
            ("ML-KEM-768 Key Pair", "Generated", Theme.color.GREEN),
            ("ML-DSA-65 Signing Key", "Generated", Theme.color.GREEN),
            ("Symmetric Session Key", "Active", Theme.color.CYAN),
            ("Freshness Master", "Synchronized", Theme.color.GREEN),
        ]

        for i, (name, status, color) in enumerate(key_items):
            row = i // 2
            col = i % 2

            item = QVBoxLayout()
            item.setSpacing(2)

            n = QLabel(name)
            n.setStyleSheet(
                f"color:{Theme.color.TEXT_SECONDARY};"
                f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
            )
            s = QLabel(f"\u2713 {status}")
            s.setStyleSheet(
                f"color:{color};"
                f"font-size:{Theme.font.SIZE_SMALL}px;"
                f"font-weight:600; border:none;"
            )
            self._key_labels[name] = s

            item.addWidget(n)
            item.addWidget(s)
            grid.addLayout(item, row, col)

        lay.addLayout(grid)

        lay.addWidget(Separator())

        # ── Key Actions ────────────────────────────────────────────
        action_row = QHBoxLayout()
        action_row.setSpacing(8)

        self._btn_rotate = IconButton("Rotate Keys", "\U0001F511", "outline")
        self._btn_export = IconButton("Export Config", "\U0001F4BE", "outline")

        action_row.addWidget(self._btn_rotate)
        action_row.addWidget(self._btn_export)
        action_row.addStretch()

        lay.addLayout(action_row)

        card.setContentLayout(lay)
        return card

    # ── Event handlers ────────────────────────────────────────────────

    def _on_algo_changed(self, algo_name: str):
        """Update algorithm info when selection changes."""
        self._update_algo_info()
        self.config_changed.emit()

    def _update_algo_info(self):
        """Update the algorithm info label."""
        algo = self._algo_combo.currentText()
        info = AUTH_ALGORITHMS.get(algo, {})
        desc = info.get("desc", "")
        level = info.get("level", "")
        mac_bytes = info.get("mac_bytes", 0)

        self._algo_info.setText(
            f"{desc}\n"
            f"Security: {level}  \u00b7  "
            f"Full MAC: {mac_bytes} bytes"
        )

    def _refresh_ecu_list(self):
        """Refresh the ECU list based on current topology."""
        # Clear existing widgets
        while self._ecu_list_lay.count():
            item = self._ecu_list_lay.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        zones = topology.zones
        total = 0
        pqc = 0
        classic = 0
        none = 0

        for zone in zones:
            zone_header = QLabel(f"\u25A0 {zone.name}")
            zone_header.setStyleSheet(f"""
                color: {Theme.color.CYAN_DARK};
                font-size: {Theme.font.SIZE_SMALL}px;
                font-weight: bold;
                padding: 6px 0 2px 0;
                border: none;
            """)
            self._ecu_list_lay.addWidget(zone_header)

            for ecu in zone.ecus:
                total += 1
                if "PQC" in ecu.security:
                    pqc += 1
                    color = Theme.color.PQC_PURPLE
                    badge = "PQC"
                elif "MAC" in ecu.security or "HMAC" in ecu.security:
                    classic += 1
                    color = Theme.color.CYAN
                    badge = "HMAC"
                else:
                    none += 1
                    color = Theme.color.TEXT_MUTED
                    badge = "None"

                ecu_row = QHBoxLayout()
                ecu_row.setSpacing(8)

                ecu_lbl = QLabel(f"  \u2022 {ecu.name}")
                ecu_lbl.setStyleSheet(
                    f"color:{Theme.color.TEXT_SECONDARY};"
                    f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
                )
                ecu_row.addWidget(ecu_lbl)

                ecu_row.addStretch()

                sec_badge = QLabel(badge)
                sec_badge.setStyleSheet(f"""
                    color: {color};
                    font-size: {Theme.font.SIZE_TINY}px;
                    font-weight: bold;
                    background: {color}20;
                    border-radius: 4px;
                    padding: 1px 6px;
                    border: none;
                """)
                ecu_row.addWidget(sec_badge)

                self._ecu_list_lay.addLayout(ecu_row)

        # Update stats
        self._total_ecus_value.setText(str(total))
        self._pqc_secured_value.setText(str(pqc))
        self._classic_mac_value.setText(str(classic))
        self._no_security_value.setText(str(none))

        if total == 0:
            empty_lbl = QLabel(
                "No ECUs configured. Add zones in the Architecture page."
            )
            empty_lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_SMALL}px;
                font-style: italic;
                padding: 12px;
                border: none;
            """)
            self._ecu_list_lay.addWidget(empty_lbl)

    # ── Signals ────────────────────────────────────────────────────────

    def _connect_signals(self):
        """Connect to topology changes."""
        topology.changed.connect(self._refresh_ecu_list)
