"""
AUTOSAR Diagnostics Monitor page.

Monitors network health and ECU status for multi-ECU gateway:
  - Network status (CAN, Ethernet, LIN)
  - ECU communication state
  - Diagnostic trouble codes (DTC)
  - Network management status

Based on AUTOSAR DCM and DEM specifications.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer, Signal
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QProgressBar,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.topology import topology
from core.app_state import state
from widgets.common import (
    IconButton,
    SectionHeader,
    StatusBadge,
    StyledCard,
    Separator,
)
from widgets.status_panel import StatusLight


class DiagnosticsPage(QWidget):
    """AUTOSAR Diagnostics and Network Monitor."""

    def __init__(self, bridge: BackendBridge, parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._build_ui()
        self._connect_signals()

        # Refresh timer
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._refresh_status)
        self._timer.start(Settings.TIMER_STATUS_REFRESH)

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

        # ── Network Health ──────────────────────────────────────────
        lay.addWidget(self._build_network_health())

        # ── ECU Status ──────────────────────────────────────────────
        lay.addWidget(self._build_ecu_status())

        # ── System Resources ────────────────────────────────────────
        lay.addWidget(self._build_system_resources())

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    def _build_header(self) -> QHBoxLayout:
        """Build the page header."""
        row = QHBoxLayout()
        row.setSpacing(12)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)

        title = QLabel("Diagnostics Monitor")
        title.setStyleSheet(f"""
            color: {Theme.color.GREEN_DARK};
            font-size: {Theme.font.SIZE_H1}px;
            font-weight: bold;  border: none;
        """)

        sub = QLabel(
            "Network Health  \u00b7  "
            "ECU Status  \u00b7  "
            "System Resources"
        )
        sub.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;  border: none;
        """)

        title_col.addWidget(title)
        title_col.addWidget(sub)
        row.addLayout(title_col, stretch=1)

        row.addWidget(StatusBadge("DCM", "info"))
        row.addWidget(StatusBadge("DEM", "info"))

        return row

    def _build_network_health(self) -> QWidget:
        """Build network health card."""
        card = StyledCard("Network Health")
        lay = card.card_layout

        # ── Status Lights Grid ──────────────────────────────────────
        grid = QGridLayout()
        grid.setSpacing(8)

        # Network status lights
        self._lights = {}

        network_items = [
            ("Ethernet Backbone", "eth_backbone", "ok"),
            ("CAN Bus", "can_bus", "idle"),
            ("LIN Sub-bus", "lin_bus", "idle"),
            ("Network Management", "nm", "ok"),
            ("PDU Router", "pdur", "ok"),
            ("SecOC Engine", "secoc", "pqc"),
        ]

        for i, (name, key, preset) in enumerate(network_items):
            row = i // 3
            col = i % 3

            light = StatusLight(name, preset)
            self._lights[key] = light
            grid.addWidget(light, row, col)

        lay.addLayout(grid)

        lay.addWidget(Separator())

        # ── Network Statistics ──────────────────────────────────────
        stats_row = QHBoxLayout()
        stats_row.setSpacing(24)

        stats = [
            ("Messages TX", "0", Theme.color.CYAN),
            ("Messages RX", "0", Theme.color.CYAN_DARK),
            ("TX Errors", "0", Theme.color.RED),
            ("RX Errors", "0", Theme.color.RED),
        ]

        for label, value, color in stats:
            stat = QVBoxLayout()
            stat.setSpacing(2)

            v = QLabel(value)
            v.setStyleSheet(f"""
                color: {color};
                font-size: {Theme.font.SIZE_H2}px;
                font-weight: bold;
                border: none;
            """)
            setattr(self, f"_stat_{label.lower().replace(' ', '_')}", v)

            l = QLabel(label)
            l.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px;
                border: none;
            """)

            stat.addWidget(v, alignment=Qt.AlignCenter)
            stat.addWidget(l, alignment=Qt.AlignCenter)
            stats_row.addLayout(stat)

        stats_row.addStretch()
        lay.addLayout(stats_row)

        return card

    def _build_ecu_status(self) -> QWidget:
        """Build ECU status card."""
        card = StyledCard("ECU Communication Status")
        lay = card.card_layout

        # ── Summary ─────────────────────────────────────────────────
        summary_row = QHBoxLayout()
        summary_row.setSpacing(16)

        for label, value, color in [
            ("Total ECUs", "0", Theme.color.TEXT_PRIMARY),
            ("Online", "0", Theme.color.GREEN),
            ("Simulated", "0", Theme.color.CYAN),
            ("Offline", "0", Theme.color.RED),
        ]:
            stat = QVBoxLayout()
            stat.setSpacing(2)

            v = QLabel(value)
            v.setStyleSheet(f"""
                color: {color};
                font-size: {Theme.font.SIZE_H2}px;
                font-weight: bold;
                border: none;
            """)
            setattr(self, f"_ecu_{label.lower().replace(' ', '_')}", v)

            l = QLabel(label)
            l.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px;
                border: none;
            """)

            stat.addWidget(v, alignment=Qt.AlignCenter)
            stat.addWidget(l, alignment=Qt.AlignCenter)
            summary_row.addLayout(stat)

        summary_row.addStretch()
        lay.addLayout(summary_row)

        lay.addWidget(Separator())

        # ── ECU List ────────────────────────────────────────────────
        self._ecu_list_container = QWidget()
        self._ecu_list_lay = QVBoxLayout(self._ecu_list_container)
        self._ecu_list_lay.setContentsMargins(0, 0, 0, 0)
        self._ecu_list_lay.setSpacing(4)

        lay.addWidget(self._ecu_list_container)
        self._refresh_ecu_list()

        return card

    def _build_system_resources(self) -> QWidget:
        """Build system resources card."""
        card = StyledCard("Gateway Resources")
        lay = card.card_layout

        # ── Resource Bars ───────────────────────────────────────────
        resources = [
            ("CPU Load", 25, Theme.color.CYAN),
            ("Memory Usage", 42, Theme.color.GREEN),
            ("Network Buffer", 15, Theme.color.ORANGE),
        ]

        for name, value, color in resources:
            row = QHBoxLayout()
            row.setSpacing(8)

            lbl = QLabel(name)
            lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_SECONDARY};
                font-size: {Theme.font.SIZE_SMALL}px;
                border: none;
            """)
            lbl.setFixedWidth(120)
            row.addWidget(lbl)

            bar = QProgressBar()
            bar.setRange(0, 100)
            bar.setValue(value)
            bar.setTextVisible(True)
            bar.setFormat(f"{value}%")
            bar.setFixedHeight(20)
            bar.setStyleSheet(f"""
                QProgressBar {{
                    background: {Theme.color.BG_ELEVATED};
                    border: 1px solid {Theme.color.BORDER};
                    border-radius: 4px;
                    text-align: center;
                }}
                QProgressBar::chunk {{
                    background: {color};
                    border-radius: 3px;
                }}
            """)
            setattr(self, f"_bar_{name.lower().replace(' ', '_')}", bar)
            row.addWidget(bar, stretch=1)

            lay.addLayout(row)

        return card

    def _refresh_ecu_list(self):
        """Refresh ECU list based on topology."""
        while self._ecu_list_lay.count():
            item = self._ecu_list_lay.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        zones = topology.zones
        total = 0
        online = 0
        simulated = 0
        offline = 0

        if not zones:
            empty = QLabel("No ECUs configured. Add zones in Architecture page.")
            empty.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_SMALL}px;
                font-style: italic;
                padding: 8px;
                border: none;
            """)
            self._ecu_list_lay.addWidget(empty)
        else:
            for zone in zones:
                # Zone header
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

                    # Determine status
                    if ecu.status == "Connected":
                        online += 1
                        status_color = Theme.color.GREEN
                        status_text = "\u2713 Online"
                    elif ecu.status == "Simulated":
                        simulated += 1
                        status_color = Theme.color.CYAN
                        status_text = "\u25CB Sim"
                    else:
                        offline += 1
                        status_color = Theme.color.RED
                        status_text = "\u2716 Offline"

                    ecu_row = QHBoxLayout()
                    ecu_row.setSpacing(8)

                    ecu_lbl = QLabel(f"  \u2022 {ecu.name}")
                    ecu_lbl.setStyleSheet(f"""
                        color: {Theme.color.TEXT_SECONDARY};
                        font-size: {Theme.font.SIZE_SMALL}px;
                        border: none;
                    """)
                    ecu_row.addWidget(ecu_lbl)

                    # Domain badge
                    domain_badge = QLabel(ecu.domain[:3])
                    domain_badge.setStyleSheet(f"""
                        color: {Theme.color.TEXT_MUTED};
                        font-size: {Theme.font.SIZE_TINY}px;
                        background: {Theme.color.BG_ELEVATED};
                        border-radius: 3px;
                        padding: 1px 4px;
                        border: none;
                    """)
                    ecu_row.addWidget(domain_badge)

                    ecu_row.addStretch()

                    status_lbl = QLabel(status_text)
                    status_lbl.setStyleSheet(f"""
                        color: {status_color};
                        font-size: {Theme.font.SIZE_TINY}px;
                        font-weight: bold;
                        border: none;
                    """)
                    ecu_row.addWidget(status_lbl)

                    self._ecu_list_lay.addLayout(ecu_row)

        # Update stats
        self._ecu_total_ecus.setText(str(total))
        self._ecu_online.setText(str(online))
        self._ecu_simulated.setText(str(simulated))
        self._ecu_offline.setText(str(offline))

    def _refresh_status(self):
        """Refresh status from state."""
        c = state.counters
        self._stat_messages_tx.setText(f"{c.tx_total:,}")
        self._stat_messages_rx.setText(f"{c.rx_total:,}")
        self._stat_tx_errors.setText(f"{c.verify_fail:,}")
        self._stat_rx_errors.setText("0")

    # ── Signals ────────────────────────────────────────────────────────

    def _connect_signals(self):
        """Connect to topology and state changes."""
        topology.changed.connect(self._refresh_ecu_list)
        state.changed.connect(self._refresh_status)
