"""
Dashboard page — Zonal Ethernet Gateway overview.

Interactive two-column layout:
  LEFT  (60 %): Interactive Zonal Gateway topology (drag, click-to-config)
  RIGHT (40 %): Security gauges, gateway config, system health
  BOTTOM:       Unified metrics bar

Zone interactions:
  • Left-click  a zone → opens ZoneConfigDialog
  • Drag        a zone → reposition on canvas
  • Right-click        → context menus for add/remove/configure
  • "Reset" button     → clears to 0 zones (empty canvas)
  • "Load Preset"      → loads standard 6-zone automotive layout
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
    QGraphicsDropShadowEffect,
    QSizePolicy,
)

from config.theme import Theme
from config.settings import Settings
from core.app_state import state
from core.signal_hub import hub
from core.topology import topology, ECU_DOMAINS
from widgets.common import StatusBadge, SectionHeader, Separator
from widgets.status_panel import StatusLight, MetricsBar
from widgets.arch_diagram import ArchDiagram
from widgets.gauges import RadialGauge
from widgets.zone_config_dialog import ZoneConfigDialog


# ── helpers ───────────────────────────────────────────────────────────

def _shadow(w: QWidget, blur: int = 24, dy: int = 4, alpha: int = 14):
    s = QGraphicsDropShadowEffect(w)
    s.setBlurRadius(blur)
    s.setOffset(0, dy)
    s.setColor(QColor(0, 0, 0, alpha))
    return s


def _card(parent: QWidget | None = None) -> QFrame:
    f = QFrame(parent)
    f.setStyleSheet(f"""
        QFrame {{
            background: {Theme.color.BG_CARD};
            border: none;
            border-radius: {Theme.radius.CARD}px;
        }}
    """)
    f.setGraphicsEffect(_shadow(f))
    return f


def _kv_row(key: str, value: str, accent: str = "") -> QHBoxLayout:
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


# ── Toolbar button style ──────────────────────────────────────────────

_BTN_STYLE = f"""
    QPushButton {{
        background: {Theme.color.BG_ELEVATED};
        color: {Theme.color.TEXT_SECONDARY};
        border: 1px solid {Theme.color.BORDER};
        border-radius: 8px;
        padding: 4px 14px;
        font-size: {Theme.font.SIZE_SMALL}px;
        font-weight: 600;
    }}
    QPushButton:hover {{
        background: {Theme.color.BG_SIDEBAR_HOVER};
        color: {Theme.color.CYAN_DARK};
        border-color: {Theme.color.CYAN};
    }}
"""


# ── DashboardPage ─────────────────────────────────────────────────────

class DashboardPage(QWidget):
    """First page — interactive topology overview at a glance."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._build_ui()
        self._connect_signals()

        self._timer = QTimer(self)
        self._timer.timeout.connect(self._refresh)
        self._timer.start(Settings.TIMER_STATUS_REFRESH)

    # ── Layout ────────────────────────────────────────────────────────

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(28, 18, 28, 18)
        root.setSpacing(16)

        root.addLayout(self._header())

        mid = QHBoxLayout()
        mid.setSpacing(16)
        mid.addWidget(self._diagram_card(), stretch=3)
        mid.addWidget(self._right_panel(), stretch=2)
        root.addLayout(mid, stretch=1)

        root.addWidget(self._metrics_row())

    # ── 1  Header ─────────────────────────────────────────────────────

    def _header(self) -> QHBoxLayout:
        row = QHBoxLayout()
        row.setSpacing(14)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)

        title = QLabel("Ethernet Gateway — PQC Secure Key Management")
        title.setStyleSheet(f"""
            color: {Theme.color.CYAN_DARK};
            font-size: {Theme.font.SIZE_H1}px;
            font-weight: bold;  border: none;
        """)

        sub = QLabel(
            "AUTOSAR SecOC  ·  Zonal E/E Architecture  ·  ML-KEM-768 + ML-DSA-65"
        )
        sub.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;  border: none;
        """)

        title_col.addWidget(title)
        title_col.addWidget(sub)
        row.addLayout(title_col, stretch=1)

        for text, preset in [
            ("FIPS 203/204", "pqc"),
            ("R21-11", "info"),
            ("Quantum-Resistant", "success"),
            ("Zonal E/E", "info"),
        ]:
            row.addWidget(StatusBadge(text, preset))

        return row

    # ── 2a  Diagram card ──────────────────────────────────────────────

    def _diagram_card(self) -> QWidget:
        card = _card()

        lay = QVBoxLayout(card)
        lay.setContentsMargins(20, 12, 20, 10)
        lay.setSpacing(6)

        # ── Toolbar: header + actions + live topology stats ────────────
        toolbar = QHBoxLayout()
        toolbar.setSpacing(8)

        toolbar.addWidget(SectionHeader("Zonal E/E Architecture"))
        toolbar.addStretch()

        # Topology summary chip
        self._topo_label = QLabel()
        self._topo_label.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_TINY}px;
            background: {Theme.color.BG_ELEVATED};
            border: 1px solid {Theme.color.BORDER};
            border-radius: 10px;
            padding: 3px 10px;
        """)
        self._update_topo_label()
        toolbar.addWidget(self._topo_label)

        # Add Zone button
        btn_add = QPushButton("+ Add Zone")
        btn_add.setCursor(Qt.PointingHandCursor)
        btn_add.setStyleSheet(_BTN_STYLE)
        btn_add.clicked.connect(self._on_add_zone)
        toolbar.addWidget(btn_add)

        # Load 1-Zone Demo button
        btn_1zone = QPushButton("1-Zone Demo")
        btn_1zone.setCursor(Qt.PointingHandCursor)
        btn_1zone.setStyleSheet(_BTN_STYLE)
        btn_1zone.setToolTip("Load single demo zone for thesis demonstration")
        btn_1zone.clicked.connect(self._on_load_1zone)
        toolbar.addWidget(btn_1zone)

        # Load 6-Zone Preset button
        btn_preset = QPushButton("6-Zone Preset")
        btn_preset.setCursor(Qt.PointingHandCursor)
        btn_preset.setStyleSheet(_BTN_STYLE)
        btn_preset.setToolTip("Load the standard 6-zone automotive layout")
        btn_preset.clicked.connect(self._on_load_preset)
        toolbar.addWidget(btn_preset)

        # Reset button (clears to 0 zones)
        btn_reset = QPushButton("Reset")
        btn_reset.setCursor(Qt.PointingHandCursor)
        btn_reset.setStyleSheet(_BTN_STYLE)
        btn_reset.setToolTip("Clear all zones — start from scratch")
        btn_reset.clicked.connect(self._on_reset_topology)
        toolbar.addWidget(btn_reset)

        lay.addLayout(toolbar)

        # ── Hint text ──────────────────────────────────────────────────
        hint = QLabel(
            "Click a zone to configure  ·  Drag to reposition  ·"
            "  Right-click for more options"
        )
        hint.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            border: none;
            font-style: italic;
        """)
        lay.addWidget(hint)

        # ── Architecture diagram ───────────────────────────────────────
        self._arch = ArchDiagram()
        self._arch.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._arch.topology_changed.connect(self._update_topo_label)
        self._arch.zone_selected.connect(self._on_zone_selected)
        lay.addWidget(self._arch, stretch=1)

        # ── State legend ───────────────────────────────────────────────
        legend = QHBoxLayout()
        legend.addStretch()
        for label, clr in [
            ("Active", Theme.color.CYAN),
            ("PQC", Theme.color.PQC_PURPLE),
            ("Error", Theme.color.RED),
            ("Idle", Theme.color.TEXT_MUTED),
        ]:
            d = QLabel(f"● {label}")
            d.setStyleSheet(
                f"color:{clr}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none; padding-left:6px;"
            )
            legend.addWidget(d)
        legend.addStretch()
        lay.addLayout(legend)

        return card

    # ── 2b  Right panel ───────────────────────────────────────────────

    def _right_panel(self) -> QWidget:
        card = _card()

        lay = QVBoxLayout(card)
        lay.setContentsMargins(22, 16, 22, 16)
        lay.setSpacing(8)

        # ── Gauges ─────────────────────────────────────────────────────
        lay.addWidget(SectionHeader("Security Metrics"))

        gauges = QHBoxLayout()
        gauges.setSpacing(4)

        self._g_success = RadialGauge(
            label="Success", unit="%", max_val=100,
            color=Theme.color.GREEN, size=100,
        )
        self._g_success.set_value(100)

        self._g_latency = RadialGauge(
            label="Latency", unit="µs", max_val=500,
            color=Theme.color.CYAN, size=100,
        )
        self._g_latency.set_value(0)

        gauges.addWidget(self._g_success, alignment=Qt.AlignCenter)
        gauges.addWidget(self._g_latency, alignment=Qt.AlignCenter)
        lay.addLayout(gauges)

        lay.addWidget(Separator())

        # ── Gateway Configuration ──────────────────────────────────────
        lay.addWidget(SectionHeader("Gateway Configuration"))

        configs = [
            ("Authentication", "ML-DSA-65",    Theme.color.PQC_PURPLE),
            ("Key Exchange",   "ML-KEM-768",   Theme.color.CYAN),
            ("Security Level", "NIST Level 3", Theme.color.GREEN),
            ("FIPS Standard",  "203 / 204",    Theme.color.TEXT_PRIMARY),
            ("Gateway Mode",   "Zonal E/E",    Theme.color.CYAN_DARK),
            ("PDU Transport",  "TP Mode",      Theme.color.TEXT_PRIMARY),
        ]
        for key, val, accent in configs:
            lay.addLayout(_kv_row(key, val, accent))

        lay.addWidget(Separator())

        # ── System Status ──────────────────────────────────────────────
        lay.addWidget(SectionHeader("System Status"))

        self._light_backend = StatusLight("Backend", "idle")
        self._light_secoc   = StatusLight("SecOC", "idle")
        self._light_pqc     = StatusLight("PQC Engine", "idle")
        self._light_keys    = StatusLight("Key Exchange", "idle")
        self._light_net     = StatusLight("Ethernet", "idle")

        grid = QGridLayout()
        grid.setSpacing(2)
        grid.setContentsMargins(0, 0, 0, 0)
        grid.addWidget(self._light_backend, 0, 0)
        grid.addWidget(self._light_secoc,   0, 1)
        grid.addWidget(self._light_pqc,     1, 0)
        grid.addWidget(self._light_keys,    1, 1)
        grid.addWidget(self._light_net,     2, 0)
        lay.addLayout(grid)

        lay.addStretch()
        return card

    # ── 3  Metrics bar ────────────────────────────────────────────────

    def _metrics_row(self) -> MetricsBar:
        self._mbar = MetricsBar([
            ("Zones",           "0",    Theme.color.CYAN),
            ("ECUs",            "0",    Theme.color.CYAN_DARK),
            ("PQC Secured",     "0",    Theme.color.PQC_PURPLE),
            ("Domains",         "0",    Theme.color.GREEN),
            ("Backbone",        "Offline", Theme.color.TEXT_MUTED),
        ])
        return self._mbar

    # ── Topology toolbar actions ──────────────────────────────────────

    def _on_add_zone(self):
        self._arch._do_add_zone()

    def _on_load_preset(self):
        """Load the standard 6-zone automotive layout."""
        self._arch.reset_all()
        topology.load_6zone_preset()

    def _on_load_1zone(self):
        """Load the 1-zone demo layout for thesis demonstration."""
        self._arch.reset_all()
        topology.load_1zone_demo()

    def _on_reset_topology(self):
        """Clear to 0 zones — empty canvas."""
        self._arch.reset_all()
        topology.clear()

    def _on_zone_selected(self, zone_uid: str):
        """Open the zone configuration dialog."""
        dlg = ZoneConfigDialog(zone_uid, parent=self)
        dlg.exec()

    def _update_topo_label(self):
        n_zones = topology.zone_count
        n_ecus = topology.total_ecus
        self._topo_label.setText(
            f"{n_zones} Zone{'s' if n_zones != 1 else ''}  ·  "
            f"{n_ecus} ECU{'s' if n_ecus != 1 else ''}"
        )

    # ── Signals ───────────────────────────────────────────────────────

    def _connect_signals(self):
        state.changed.connect(self._refresh)
        state.init_changed.connect(self._on_backend)
        hub.auth_completed.connect(self._on_auth)
        hub.verify_completed.connect(self._on_verify)
        topology.changed.connect(self._update_topo_label)

    def _on_backend(self, ready: bool):
        p = "ok" if ready else "idle"
        self._light_backend.set_preset(p, "Backend")
        if ready:
            self._light_secoc.set_preset("ok", "SecOC")
            self._light_pqc.set_preset("pqc", "PQC Engine")
        self._arch.set_node_state("gateway", 3 if ready else 0)

    def _on_auth(self, msg: str, elapsed: float):
        self._arch.set_node_state("gateway", 1)
        zones = topology.zones
        if zones:
            self._arch.set_node_state(zones[0].uid, 1)
            self._arch.animate_flow()
            uid = zones[0].uid
            QTimer.singleShot(2000, lambda: self._arch.set_node_state(uid, 0))
        QTimer.singleShot(2000, lambda: self._arch.set_node_state("gateway", 0))

    def _on_verify(self, result):
        ok = getattr(result, "is_ok", True)
        ns = 1 if ok else 2
        zones = topology.zones
        if zones:
            uid = zones[-1].uid
            self._arch.set_node_state(uid, ns)
            QTimer.singleShot(2000, lambda: self._arch.set_node_state(uid, 0))

    # ── Periodic refresh ──────────────────────────────────────────────

    def _refresh(self):
        # Update topology-based metrics
        self._mbar.set_value(0, str(topology.zone_count))
        self._mbar.set_value(1, str(topology.total_ecus))
        self._mbar.set_value(2, str(topology.pqc_secured_ecus))
        self._mbar.set_value(3, str(len(topology.active_domains)))

        backbone = topology.backbone_status
        color = Theme.color.GREEN if backbone == "Online" else (
            Theme.color.CYAN if backbone == "Simulated" else Theme.color.TEXT_MUTED
        )
        self._mbar.set_value(4, backbone)

        # Update gauges with security stats
        self._g_success.set_value(state.success_rate)
        self._g_latency.set_value(state.avg_latency_us)
