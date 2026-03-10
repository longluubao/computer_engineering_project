"""
AUTOSAR Gateway Routing Configuration page.

Configures PDU Router (PduR) gateway for multi-ECU communication:
  - PDU routing table with auto-generation from topology
  - SecOC protection visualization (PQC / Classic / None)
  - Bus distribution and interface summary
  - Mode tabs: PDU Router, Signal Gateway, TP Gateway

Based on AUTOSAR R22-11 Gateway specification.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QFrame,
    QHBoxLayout,
    QHeaderView,
    QLabel,
    QScrollArea,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from core.backend_bridge import BackendBridge
from core.topology import (
    topology,
    RouteConfig,
    LOCAL_BUS_OPTIONS,
    SECURITY_OPTIONS,
)
from widgets.common import (
    IconButton,
    SectionHeader,
    StatusBadge,
    StyledCard,
    Separator,
)


# ── Helpers ───────────────────────────────────────────────────────────

_SECOC_COLORS = {
    "PQC (ML-DSA-65)": Theme.color.PQC_PURPLE,
    "Classic MAC (HMAC)": Theme.color.CLASSIC_BLUE,
    "None": Theme.color.TEXT_MUTED,
}

_BUS_COLORS = {
    "CAN": Theme.color.CYAN,
    "CAN-FD": Theme.color.CYAN_DARK,
    "Ethernet": Theme.color.GREEN,
    "LIN": Theme.color.ORANGE,
    "FlexRay": Theme.color.PQC_PURPLE,
}


def _secoc_short(secoc: str) -> str:
    if "PQC" in secoc:
        return "PQC"
    if "HMAC" in secoc or "Classic" in secoc:
        return "HMAC"
    return "None"


# ── Add Route Dialog ──────────────────────────────────────────────────

class _AddRouteDialog(QDialog):
    """Dialog to manually add a single route."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Add PDU Route")
        self.setMinimumWidth(420)

        lay = QFormLayout(self)
        lay.setSpacing(10)

        # Collect ECUs from topology
        self._ecu_map: dict[str, tuple] = {}  # display -> (ecu, zone_name)
        for zone in topology.zones:
            for ecu in zone.ecus:
                key = f"{ecu.name} ({zone.name})"
                self._ecu_map[key] = (ecu, zone.name)

        ecu_names = sorted(self._ecu_map.keys())

        self._src_combo = QComboBox()
        self._src_combo.addItems(ecu_names)
        self._src_combo.setStyleSheet(Theme.stylesheet.COMBO)
        lay.addRow("Source ECU:", self._src_combo)

        self._dst_combo = QComboBox()
        self._dst_combo.addItems(ecu_names)
        if len(ecu_names) > 1:
            self._dst_combo.setCurrentIndex(1)
        self._dst_combo.setStyleSheet(Theme.stylesheet.COMBO)
        lay.addRow("Destination ECU:", self._dst_combo)

        self._secoc_combo = QComboBox()
        self._secoc_combo.addItems(SECURITY_OPTIONS)
        self._secoc_combo.setStyleSheet(Theme.stylesheet.COMBO)
        lay.addRow("SecOC:", self._secoc_combo)

        self._mode_combo = QComboBox()
        self._mode_combo.addItems(["IF", "TP"])
        self._mode_combo.setStyleSheet(Theme.stylesheet.COMBO)
        lay.addRow("Mode:", self._mode_combo)

        btns = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        lay.addRow(btns)

    def get_route(self, pdu_id: str) -> RouteConfig | None:
        src_key = self._src_combo.currentText()
        dst_key = self._dst_combo.currentText()
        if src_key not in self._ecu_map or dst_key not in self._ecu_map:
            return None
        src_ecu, src_zone = self._ecu_map[src_key]
        dst_ecu, dst_zone = self._ecu_map[dst_key]
        return RouteConfig(
            source_ecu_uid=src_ecu.uid,
            source_ecu_name=src_ecu.name,
            source_zone=src_zone,
            source_bus=src_ecu.local_bus,
            dest_ecu_uid=dst_ecu.uid,
            dest_ecu_name=dst_ecu.name,
            dest_zone=dst_zone,
            dest_bus=dst_ecu.local_bus,
            pdu_id=pdu_id,
            pdu_name=f"{src_ecu.name}_to_{dst_ecu.name}",
            mode=self._mode_combo.currentText(),
            secoc=self._secoc_combo.currentText(),
            direction="Bidirectional",
            status="Configured",
        )


# ── RoutingPage ───────────────────────────────────────────────────────

class RoutingPage(QWidget):
    """AUTOSAR Gateway routing configuration panel."""

    config_changed = Signal()

    def __init__(self, bridge: BackendBridge, parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._active_tab = 0  # 0=PDU Router, 1=Signal GW, 2=TP GW
        self._build_ui()
        self._connect_signals()
        self._refresh()

    # ── UI Construction ───────────────────────────────────────────────

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

        lay.addLayout(self._build_header())
        lay.addWidget(self._build_overview())
        lay.addWidget(self._build_routing_table())
        lay.addWidget(self._build_interface_summary())
        lay.addStretch()

        scroll.setWidget(container)
        root.addWidget(scroll)

    # ── Section 1: Header + Mode Tabs ─────────────────────────────────

    def _build_header(self) -> QHBoxLayout:
        row = QHBoxLayout()
        row.setSpacing(12)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)

        title = QLabel("Gateway Routing")
        title.setStyleSheet(f"""
            color: {Theme.color.CYAN_DARK};
            font-size: {Theme.font.SIZE_H1}px;
            font-weight: bold; border: none;
        """)
        sub = QLabel("AUTOSAR PDU Router \u00b7 R22-11 Specification")
        sub.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px; border: none;
        """)
        title_col.addWidget(title)
        title_col.addWidget(sub)
        row.addLayout(title_col, stretch=1)

        # Tab buttons
        self._tab_btns: list[IconButton] = []
        for i, label in enumerate(["PDU Router", "Signal GW", "TP GW"]):
            btn = IconButton(label, "", "primary" if i == 0 else "outline")
            btn.clicked.connect(lambda checked, idx=i: self._switch_tab(idx))
            self._tab_btns.append(btn)
            row.addWidget(btn)

        return row

    def _switch_tab(self, idx: int):
        self._active_tab = idx
        for i, btn in enumerate(self._tab_btns):
            style = "primary" if i == idx else "outline"
            qss = Theme.stylesheet.BTN_PRIMARY if style == "primary" else Theme.stylesheet.BTN_OUTLINE
            btn.setStyleSheet(qss)

    # ── Section 2: Routing Overview ───────────────────────────────────

    def _build_overview(self) -> QWidget:
        card = StyledCard("Routing Overview")
        lay = card.card_layout

        # Stats row
        stats_row = QHBoxLayout()
        stats_row.setSpacing(20)

        self._stat_labels: dict[str, QLabel] = {}
        stat_defs = [
            ("total", "Total Routes", Theme.color.CYAN),
            ("pqc", "PQC Secured", Theme.color.PQC_PURPLE),
            ("hmac", "Classic MAC", Theme.color.CLASSIC_BLUE),
            ("none", "Unprotected", Theme.color.TEXT_MUTED),
            ("if_mode", "IF Mode", Theme.color.CYAN_DARK),
            ("tp_mode", "TP Mode", Theme.color.GREEN),
        ]

        for key, label, color in stat_defs:
            col = QVBoxLayout()
            col.setSpacing(2)
            val = QLabel("0")
            val.setStyleSheet(f"""
                color: {color}; font-size: {Theme.font.SIZE_H2}px;
                font-weight: bold; border: none;
            """)
            val.setAlignment(Qt.AlignCenter)
            self._stat_labels[key] = val

            lbl = QLabel(label)
            lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px; border: none;
            """)
            lbl.setAlignment(Qt.AlignCenter)

            col.addWidget(val)
            col.addWidget(lbl)
            stats_row.addLayout(col)

        stats_row.addStretch()
        lay.addLayout(stats_row)

        # Bus distribution bar
        self._bus_bar_container = QWidget()
        self._bus_bar_lay = QHBoxLayout(self._bus_bar_container)
        self._bus_bar_lay.setContentsMargins(0, 4, 0, 0)
        self._bus_bar_lay.setSpacing(0)
        lay.addWidget(self._bus_bar_container)

        # Bus legend
        self._bus_legend = QLabel("")
        self._bus_legend.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_TINY}px; border: none;
        """)
        lay.addWidget(self._bus_legend)

        lay.addWidget(Separator())

        # Coverage text
        self._coverage_label = QLabel("No routes configured")
        self._coverage_label.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px; border: none;
        """)
        lay.addWidget(self._coverage_label)

        return card

    # ── Section 3: PDU Routing Table ──────────────────────────────────

    def _build_routing_table(self) -> QWidget:
        card = StyledCard("PDU Routing Table")
        lay = card.card_layout

        # Toolbar
        toolbar = QHBoxLayout()
        toolbar.setSpacing(8)

        self._btn_add = IconButton("Add Route", "+", "outline")
        self._btn_auto = IconButton("Auto-Generate", "\u26A1", "primary")
        self._btn_clear = IconButton("Clear All", "\u2716", "danger")
        self._btn_export = IconButton("Export ARXML", "\U0001F4BE", "outline")

        self._btn_add.clicked.connect(self._on_add_route)
        self._btn_auto.clicked.connect(self._on_auto_generate)
        self._btn_clear.clicked.connect(self._on_clear)

        toolbar.addWidget(self._btn_add)
        toolbar.addWidget(self._btn_auto)
        toolbar.addWidget(self._btn_clear)
        toolbar.addWidget(self._btn_export)
        toolbar.addStretch()

        lay.addLayout(toolbar)

        # Table
        cols = ["#", "Source ECU", "Src Zone", "Src Bus", "PDU ID",
                "PDU Name", "Dest ECU", "Dst Zone", "Dst Bus", "Mode", "SecOC", "Status"]
        self._table = QTableWidget()
        self._table.setColumnCount(len(cols))
        self._table.setHorizontalHeaderLabels(cols)
        self._table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(4, QHeaderView.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(9, QHeaderView.ResizeToContents)
        self._table.setSelectionBehavior(QTableWidget.SelectRows)
        self._table.setEditTriggers(QTableWidget.NoEditTriggers)
        self._table.setAlternatingRowColors(True)
        self._table.setMinimumHeight(300)
        self._table.setStyleSheet(f"""
            QTableWidget {{
                background: {Theme.color.BG_ELEVATED};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 6px;
                gridline-color: {Theme.color.BORDER};
            }}
            QTableWidget::item {{
                padding: 6px 8px;
                border: none;
            }}
            QTableWidget::item:selected {{
                background: rgba(0, 152, 206, 0.15);
                color: {Theme.color.TEXT_PRIMARY};
            }}
            QHeaderView::section {{
                background: {Theme.color.BG_CARD};
                color: {Theme.color.TEXT_SECONDARY};
                font-weight: bold;
                padding: 6px 8px;
                border: none;
                border-bottom: 1px solid {Theme.color.BORDER};
                font-size: {Theme.font.SIZE_TINY}px;
            }}
        """)

        lay.addWidget(self._table)

        # Hint
        hint = QLabel(
            "Click Auto-Generate to create routes from topology, "
            "or add routes manually. Right-click a row to remove."
        )
        hint.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            font-style: italic; border: none;
        """)
        lay.addWidget(hint)

        return card

    # ── Section 4: Interface Summary ──────────────────────────────────

    def _build_interface_summary(self) -> QWidget:
        card = StyledCard("Interface Summary")
        lay = card.card_layout

        self._iface_row = QHBoxLayout()
        self._iface_row.setSpacing(12)
        self._iface_cards: dict[str, QLabel] = {}

        for bus, color in _BUS_COLORS.items():
            frame = QFrame()
            frame.setStyleSheet(f"""
                QFrame {{
                    background: {Theme.color.BG_ELEVATED};
                    border: 1px solid {Theme.color.BORDER};
                    border-left: 3px solid {color};
                    border-radius: 8px;
                    padding: 10px 16px;
                }}
            """)
            fl = QVBoxLayout(frame)
            fl.setSpacing(2)

            name_lbl = QLabel(bus)
            name_lbl.setStyleSheet(f"""
                color: {color}; font-size: {Theme.font.SIZE_SMALL}px;
                font-weight: bold; border: none;
            """)
            count_lbl = QLabel("0 routes")
            count_lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px; border: none;
            """)
            self._iface_cards[bus] = count_lbl

            fl.addWidget(name_lbl)
            fl.addWidget(count_lbl)
            self._iface_row.addWidget(frame)

        self._iface_row.addStretch()
        lay.addLayout(self._iface_row)

        return card

    # ── Signals ───────────────────────────────────────────────────────

    def _connect_signals(self):
        topology.changed.connect(self._refresh)

    # ── Actions ───────────────────────────────────────────────────────

    def _on_auto_generate(self):
        topology.auto_generate_routes()

    def _on_clear(self):
        topology.clear_routes()

    def _on_add_route(self):
        if topology.total_ecus < 2:
            return
        dlg = _AddRouteDialog(self)
        if dlg.exec() == QDialog.Accepted:
            next_id = f"0x{0x100 + len(topology.routes):04X}"
            route = dlg.get_route(next_id)
            if route:
                topology.add_route(route)

    # ── Refresh ───────────────────────────────────────────────────────

    def _refresh(self):
        routes = topology.routes

        # Stats
        pqc = sum(1 for r in routes if "PQC" in r.secoc)
        hmac = sum(1 for r in routes if "HMAC" in r.secoc or "Classic" in r.secoc)
        none_count = sum(1 for r in routes if r.secoc == "None")
        if_count = sum(1 for r in routes if r.mode == "IF")
        tp_count = sum(1 for r in routes if r.mode == "TP")

        self._stat_labels["total"].setText(str(len(routes)))
        self._stat_labels["pqc"].setText(str(pqc))
        self._stat_labels["hmac"].setText(str(hmac))
        self._stat_labels["none"].setText(str(none_count))
        self._stat_labels["if_mode"].setText(str(if_count))
        self._stat_labels["tp_mode"].setText(str(tp_count))

        # Bus distribution
        bus_counts: dict[str, int] = {}
        for r in routes:
            for bus in (r.source_bus, r.dest_bus):
                bus_counts[bus] = bus_counts.get(bus, 0) + 1

        # Rebuild bus bar
        while self._bus_bar_lay.count():
            item = self._bus_bar_lay.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        total_bus = sum(bus_counts.values()) or 1
        for bus, count in sorted(bus_counts.items()):
            seg = QFrame()
            color = _BUS_COLORS.get(bus, Theme.color.TEXT_MUTED)
            seg.setStyleSheet(f"background: {color}; border: none; border-radius: 3px;")
            seg.setFixedHeight(8)
            self._bus_bar_lay.addWidget(seg, stretch=count)

        if not bus_counts:
            empty = QFrame()
            empty.setStyleSheet(f"background: {Theme.color.BORDER}; border: none; border-radius: 3px;")
            empty.setFixedHeight(8)
            self._bus_bar_lay.addWidget(empty)

        legend_parts = [f"{bus}: {count}" for bus, count in sorted(bus_counts.items())]
        self._bus_legend.setText("  \u00b7  ".join(legend_parts) if legend_parts else "No routes")

        # Coverage
        ecu_uids = set()
        for zone in topology.zones:
            for ecu in zone.ecus:
                ecu_uids.add(ecu.uid)
        n_ecus = len(ecu_uids)
        max_pairs = n_ecus * (n_ecus - 1) // 2 if n_ecus > 1 else 0
        self._coverage_label.setText(
            f"Coverage: {len(routes)} of {max_pairs} ECU pairs routed"
            if max_pairs else "Add zones with ECUs to generate routes"
        )

        # Interface summary cards
        for bus, lbl in self._iface_cards.items():
            c = bus_counts.get(bus, 0)
            lbl.setText(f"{c} route{'s' if c != 1 else ''}")

        # Table
        self._table.setRowCount(len(routes))
        for row, r in enumerate(routes):
            secoc_color = _SECOC_COLORS.get(r.secoc, Theme.color.TEXT_MUTED)
            items = [
                str(row + 1),
                r.source_ecu_name,
                r.source_zone,
                r.source_bus,
                r.pdu_id,
                r.pdu_name,
                r.dest_ecu_name,
                r.dest_zone,
                r.dest_bus,
                r.mode,
                _secoc_short(r.secoc),
                r.status,
            ]
            for col, text in enumerate(items):
                item = QTableWidgetItem(text)
                item.setTextAlignment(Qt.AlignCenter)
                # Color the SecOC column
                if col == 10:
                    item.setForeground(QColor(secoc_color))
                self._table.setItem(row, col, item)
