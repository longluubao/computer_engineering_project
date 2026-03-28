"""
Hardware Configuration page — central gateway and zone communication setup.

Shows both the Central Zone Gateway (HPCU) and each Zone Controller.
"""

from __future__ import annotations

import csv
from datetime import datetime
from pathlib import Path

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QComboBox,
    QFileDialog,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from core.app_state import state
from core.hw_configurator import hw_configurator
from core.signal_hub import hub
from core.topology import (
    ADAPTER_WIRING,
    BACKBONE_OPTIONS,
    CAN_ADAPTER_OPTIONS,
    MCU_PERIPHERALS,
    ZONE_HARDWARE_OPTIONS,
    GatewayConfig,
    ZoneController,
    topology,
)
from widgets.common import IconButton, StatusBadge, StyledCard
from widgets.hw_dialogs import CANConfigDialog, IPConfigDialog


# ── Sentinel to distinguish gateway from zone controllers ────────────
_GATEWAY_KEY = "__gateway__"


class HardwareConfigPage(QWidget):
    """Central communication setup and ECU peripheral details."""

    def __init__(self, bridge=None, parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._run_seconds = 0
        self._last_event = "Idle"
        self._baseline_snapshot: dict[str, float] | None = None
        # combo index → zone uid or _GATEWAY_KEY
        self._combo_keys: list[str] = []
        self._build_ui()
        self._connect_signals()
        self._ticker = QTimer(self)
        self._ticker.timeout.connect(self._tick)
        self._ticker.start(1000)

    # ── UI construction ──────────────────────────────────────────────

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet(Theme.stylesheet.SCROLL_AREA)

        container = QWidget()
        self._outer_lay = QVBoxLayout(container)
        self._outer_lay.setContentsMargins(28, 24, 28, 24)
        self._outer_lay.setSpacing(18)

        self._outer_lay.addLayout(self._build_header())

        self._content = QWidget()
        self._outer_lay.addWidget(self._content, stretch=1)

        scroll.setWidget(container)
        root.addWidget(scroll)

    def _build_header(self) -> QHBoxLayout:
        row = QHBoxLayout()
        row.setSpacing(14)

        title_col = QVBoxLayout()
        title_col.setSpacing(2)
        title = QLabel("Central Computing Dashboard")
        title.setStyleSheet(
            f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_H1}px;"
            f"font-weight:bold; border:none;"
        )
        subtitle = QLabel("Gateway Health \u00b7 Routing \u00b7 Security \u00b7 Diagnostics")
        subtitle.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none;"
        )
        title_col.addWidget(title)
        title_col.addWidget(subtitle)
        row.addLayout(title_col)
        row.addStretch()

        lbl = QLabel("Device:")
        lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
            f"font-weight:600; border:none;"
        )
        row.addWidget(lbl, alignment=Qt.AlignVCenter)

        self._device_combo = QComboBox()
        self._device_combo.setStyleSheet(Theme.stylesheet.COMBO)
        self._device_combo.setMinimumWidth(260)
        row.addWidget(self._device_combo, alignment=Qt.AlignVCenter)

        self._hw_badge = StatusBadge("No zones", "idle")
        row.addWidget(self._hw_badge, alignment=Qt.AlignVCenter)

        return row

    # ── Signal wiring ────────────────────────────────────────────────

    def _connect_signals(self):
        topology.changed.connect(self._refresh_combo)
        self._device_combo.currentIndexChanged.connect(self._on_device_changed)
        hw_configurator.operation_completed.connect(self._on_hw_operation)
        state.changed.connect(self._refresh_live_view)
        hub.attack_result.connect(self._on_attack_result)
        hub.operation_error.connect(self._on_operation_error)
        hub.auth_completed.connect(self._on_auth_completed)
        hub.verify_completed.connect(self._on_verify_completed)
        self._refresh_combo()

    def _tick(self):
        self._run_seconds += 1
        self._refresh_live_view()

    def _on_hw_operation(self, op: str, success: bool, msg: str):
        preset = "ok" if success else "error"
        text = f"{op}: {'OK' if success else 'Fail'}"
        self._hw_badge.set_preset(preset, text)
        self._last_event = f"{op}: {msg}"
        self._refresh_live_view()

    def _on_attack_result(self, attack_name: str, detected: bool, detail: str):
        tag = "detected" if detected else "not-detected"
        self._last_event = f"Attack {attack_name}: {tag} ({detail})"
        self._refresh_live_view()

    def _on_operation_error(self, operation: str, error: str):
        self._last_event = f"Error {operation}: {error}"
        self._refresh_live_view()

    def _on_auth_completed(self, operation_name: str, elapsed_us: float):
        self._last_event = f"Auth {operation_name} in {elapsed_us:.1f}us"
        self._refresh_live_view()

    def _on_verify_completed(self, operation: str, elapsed_us: float, success: bool):
        status = "OK" if success else "FAIL"
        self._last_event = f"Verify {operation}: {status} in {elapsed_us:.1f}us"
        self._refresh_live_view()

    def _refresh_live_view(self):
        idx = self._device_combo.currentIndex()
        if 0 <= idx < len(self._combo_keys):
            self._on_device_changed(idx)

    def _refresh_combo(self):
        self._device_combo.blockSignals(True)
        prev_key = ""
        idx = self._device_combo.currentIndex()
        if 0 <= idx < len(self._combo_keys):
            prev_key = self._combo_keys[idx]

        self._device_combo.clear()
        self._combo_keys.clear()

        # Always show gateway first
        gw = topology.gateway
        self._device_combo.addItem(
            f"\u2b24  {gw.name}  \u2014  {gw.hardware}"
        )
        self._combo_keys.append(_GATEWAY_KEY)

        # Then zone controllers
        for z in topology.zones:
            self._device_combo.addItem(f"    {z.name}  \u2014  {z.hardware}")
            self._combo_keys.append(z.uid)

        # Restore selection
        restore_idx = 0
        if prev_key:
            for i, k in enumerate(self._combo_keys):
                if k == prev_key:
                    restore_idx = i
                    break

        self._device_combo.setCurrentIndex(restore_idx)
        self._device_combo.blockSignals(False)
        self._on_device_changed(restore_idx)

    # ── Device change handler ────────────────────────────────────────

    def _on_device_changed(self, index: int):
        if index < 0 or index >= len(self._combo_keys):
            self._set_content(QWidget())
            return

        key = self._combo_keys[index]

        if key == _GATEWAY_KEY:
            gw = topology.gateway
            self._hw_badge.set_preset("info", f"{gw.hardware}  \u00b7  {gw.role}")
            self._set_content(self._build_gateway_content(gw))
        else:
            zone = topology.zone_by_uid(key)
            if not zone:
                self._set_content(QWidget())
                return
            if zone.hardware == "Simulated (Virtual)":
                self._hw_badge.set_preset("idle", zone.hardware)
            else:
                self._hw_badge.set_preset("info", zone.hardware)
            self._set_content(self._build_zone_content(zone))

    def _set_content(self, new: QWidget):
        idx = self._outer_lay.indexOf(self._content)
        self._outer_lay.removeWidget(self._content)
        self._content.setParent(None)
        self._content = new
        self._outer_lay.insertWidget(idx, new, stretch=1)

    def _format_runtime(self) -> str:
        s = self._run_seconds
        h = s // 3600
        m = (s % 3600) // 60
        sec = s % 60
        return f"{h:02d}:{m:02d}:{sec:02d}"

    def _latency_percentiles_us(self) -> tuple[float, float]:
        samples = [float(s.elapsed_us) for s in state.perf_history if s.elapsed_us > 0]
        if not samples:
            return 0.0, 0.0
        samples.sort()
        n = len(samples)
        p50_idx = max(0, min(n - 1, int(round(0.50 * (n - 1)))))
        p95_idx = max(0, min(n - 1, int(round(0.95 * (n - 1)))))
        return samples[p50_idx], samples[p95_idx]

    def _export_evidence_csv(self, node_name: str, role: str, p50: float, p95: float):
        default_name = f"thesis_evidence_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        out_path, _ = QFileDialog.getSaveFileName(
            self,
            "Export Thesis Evidence CSV",
            str(Path.cwd() / default_name),
            "CSV Files (*.csv)",
        )
        if not out_path:
            return

        path = Path(out_path)
        with path.open("w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["section", "metric", "value"])
            writer.writerow(["summary", "node", node_name])
            writer.writerow(["summary", "role", role])
            writer.writerow(["summary", "runtime", self._format_runtime()])
            writer.writerow(["summary", "auth_mode", state.auth_mode.name])
            writer.writerow(["summary", "success_rate_pct", f"{state.success_rate:.3f}"])
            writer.writerow(["summary", "avg_latency_us", f"{state.avg_latency_us:.3f}"])
            writer.writerow(["summary", "p50_latency_us", f"{p50:.3f}"])
            writer.writerow(["summary", "p95_latency_us", f"{p95:.3f}"])
            writer.writerow(["summary", "verify_ok", state.counters.verify_ok])
            writer.writerow(["summary", "verify_fail", state.counters.verify_fail])
            writer.writerow(["summary", "attacks_detected", state.counters.attacks_detected])
            writer.writerow(["summary", "attacks_total", state.counters.attacks_total])
            writer.writerow(["summary", "tx_total", state.counters.tx_total])
            writer.writerow(["summary", "rx_total", state.counters.rx_total])
            writer.writerow(["summary", "key_exchanges", state.counters.key_exchanges])
            writer.writerow(["summary", "last_event", self._last_event])

            if self._baseline_snapshot:
                b = self._baseline_snapshot
                writer.writerow(["baseline", "mode", "PQC" if b["mode_is_pqc"] > 0 else "CLASSIC"])
                writer.writerow(["baseline", "avg_latency_us", f"{b['avg_latency_us']:.3f}"])
                writer.writerow(["baseline", "p50_latency_us", f"{b['p50_latency_us']:.3f}"])
                writer.writerow(["baseline", "p95_latency_us", f"{b['p95_latency_us']:.3f}"])
                writer.writerow(["baseline", "success_rate_pct", f"{b['success_rate']:.3f}"])

            writer.writerow([])
            writer.writerow(["perf_history", "timestamp", "operation", "elapsed_us", "data_size", "throughput_kbps"])
            for s in state.perf_history:
                writer.writerow(
                    [
                        "perf_history",
                        datetime.fromtimestamp(s.timestamp).isoformat(timespec="seconds"),
                        s.operation,
                        f"{s.elapsed_us:.3f}",
                        s.data_size,
                        f"{s.throughput_kbps:.3f}",
                    ]
                )

        self._last_event = f"Evidence exported: {path.name}"
        self._refresh_live_view()

    # ══════════════════════════════════════════════════════════════════
    # GATEWAY content
    # ══════════════════════════════════════════════════════════════════

    def _build_gateway_content(self, gw: GatewayConfig) -> QWidget:
        w = QWidget()
        lay = QVBoxLayout(w)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(18)

        lay.addWidget(self._make_gateway_config_card(gw))
        lay.addWidget(self._make_runtime_health_card(gw.name, gw.ip_address))
        lay.addWidget(self._make_zonal_connection_card())
        lay.addWidget(self._make_pqc_focus_card(gw.name, is_gateway=True))
        lay.addWidget(self._make_routing_control_card(gw.name, True))
        lay.addWidget(self._make_security_posture_card(gw.name))
        lay.addWidget(self._make_thesis_scorecard_card(gw.name))
        lay.addWidget(self._make_diagnostics_card(gw.name, True))
        lay.addWidget(self._make_interface_status_card("Gateway", gw.ip_address))
        if hw_configurator.simulation_mode:
            lay.addWidget(self._make_simulation_banner())
        lay.addWidget(self._make_adapter_card(gw.can_adapter))
        lay.addStretch()
        return w

    def _make_gateway_config_card(self, gw: GatewayConfig) -> QWidget:
        card = StyledCard(
            "ZONE GATEWAY CONFIGURATION (HPCU)",
            accent_color=Theme.color.CYAN_DARK,
        )

        # Description
        desc = QLabel(gw.description)
        desc.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
            f"border:none; padding-bottom:8px;"
        )
        card.card_layout.addWidget(desc)

        # Config form — 2-column grid
        form = QGridLayout()
        form.setSpacing(10)
        form.setContentsMargins(0, 0, 0, 0)

        row = 0

        # Hardware
        form.addWidget(self._form_label("Board"), row, 0)
        hw_combo = QComboBox()
        hw_combo.setStyleSheet(Theme.stylesheet.COMBO)
        hw_combo.addItems(ZONE_HARDWARE_OPTIONS)
        hw_combo.setCurrentText(gw.hardware)
        hw_combo.currentTextChanged.connect(
            lambda val: topology.update_gateway(hardware=val)
        )
        form.addWidget(hw_combo, row, 1)

        role_badge = StatusBadge("HPCU", "info")
        form.addWidget(role_badge, row, 2)
        row += 1

        # IP Address
        form.addWidget(self._form_label("IP Address"), row, 0)
        ip_edit = QLineEdit(gw.ip_address)
        ip_edit.setStyleSheet(Theme.stylesheet.INPUT)
        ip_edit.setPlaceholderText("e.g. 192.168.1.1")
        ip_edit.editingFinished.connect(
            lambda: topology.update_gateway(ip_address=ip_edit.text())
        )
        form.addWidget(ip_edit, row, 1)
        row += 1

        # Backbone
        form.addWidget(self._form_label("Backbone"), row, 0)
        bb_combo = QComboBox()
        bb_combo.setStyleSheet(Theme.stylesheet.COMBO)
        bb_combo.addItems(BACKBONE_OPTIONS)
        bb_combo.setCurrentText(gw.backbone)
        bb_combo.currentTextChanged.connect(
            lambda val: topology.update_gateway(backbone=val)
        )
        form.addWidget(bb_combo, row, 1)
        row += 1

        # CAN Adapter
        form.addWidget(self._form_label("CAN Adapter"), row, 0)
        can_combo = QComboBox()
        can_combo.setStyleSheet(Theme.stylesheet.COMBO)
        can_combo.addItems(CAN_ADAPTER_OPTIONS)
        can_combo.setCurrentText(gw.can_adapter)
        can_combo.currentTextChanged.connect(
            lambda val: topology.update_gateway(can_adapter=val)
        )
        form.addWidget(can_combo, row, 1)
        row += 1

        card.card_layout.addLayout(form)
        return card

    # ══════════════════════════════════════════════════════════════════
    # ZONE CONTROLLER content
    # ══════════════════════════════════════════════════════════════════

    def _build_zone_content(self, zone: ZoneController) -> QWidget:
        w = QWidget()
        lay = QVBoxLayout(w)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(18)

        lay.addWidget(self._make_zone_config_card(zone))
        lay.addWidget(self._make_runtime_health_card(zone.name, zone.ip_address))
        lay.addWidget(self._make_zone_to_gateway_card(zone))
        lay.addWidget(self._make_pqc_focus_card(zone.name, is_gateway=False))
        lay.addWidget(self._make_routing_control_card(zone.name, False))
        lay.addWidget(self._make_security_posture_card(zone.name))
        lay.addWidget(self._make_thesis_scorecard_card(zone.name))
        lay.addWidget(self._make_diagnostics_card(zone.name, False))
        lay.addWidget(self._make_interface_status_card(zone.name, zone.ip_address))
        if hw_configurator.simulation_mode:
            lay.addWidget(self._make_simulation_banner())
        lay.addWidget(self._make_adapter_card(zone.can_adapter))
        lay.addWidget(self._make_peripheral_card(zone))
        lay.addWidget(self._make_ecu_matrix_card(zone))
        lay.addStretch()
        return w

    def _make_simulation_banner(self) -> QWidget:
        card = StyledCard("SIMULATION MODE", accent_color=Theme.color.YELLOW)
        msg = QLabel("Running in simulation mode — commands are logged but not executed.")
        msg.setStyleSheet(
            f"color:{Theme.color.YELLOW_DARK}; font-size:{Theme.font.SIZE_BODY}px;"
            f"font-weight:600; border:none; padding:6px 0;"
        )
        card.card_layout.addWidget(msg)
        return card

    def _make_runtime_health_card(self, node_name: str, fallback_ip: str) -> QWidget:
        card = StyledCard("EXPERIMENT CONTEXT", accent_color=Theme.color.CYAN_DARK)

        net = hw_configurator.network_info("eth0")
        ip_txt = net.get("ip", "N/A")
        if ip_txt == "N/A" and fallback_ip:
            ip_txt = fallback_ip

        scenario = "Replay Attack Drill" if state.counters.attacks_total > 0 else "Baseline Secure Routing"
        mode_txt = "PQC" if state.auth_mode.name == "PQC" else "CLASSIC"
        intro = QLabel(
            f"Thesis focus: central compute at {node_name} enforces secure cross-domain routing."
        )
        intro.setWordWrap(True)
        intro.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none; padding-bottom:2px;"
        )
        card.card_layout.addWidget(intro)

        grid = QGridLayout()
        grid.setContentsMargins(0, 0, 0, 0)
        grid.setHorizontalSpacing(24)
        grid.setVerticalSpacing(8)
        metrics = [
            ("Scenario", scenario),
            ("Run Time", self._format_runtime()),
            ("Security Mode", mode_txt),
            ("Node", node_name),
            ("Gateway IP", ip_txt),
            ("Topology", f"{topology.zone_count} zones / {topology.total_ecus} ECUs"),
        ]
        for i, (k, v) in enumerate(metrics):
            key = QLabel(k)
            key.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none;"
            )
            val = QLabel(v)
            val.setStyleSheet(
                f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_BODY}px;"
                f"font-weight:600; border:none;"
            )
            row = i // 3
            col = (i % 3) * 2
            grid.addWidget(key, row * 2, col)
            grid.addWidget(val, row * 2 + 1, col)
        card.card_layout.addLayout(grid)
        return card

    def _make_zonal_connection_card(self) -> QWidget:
        card = StyledCard("ZONAL CONNECTION VIEW (REF: CH1.1 / CH2.1 / CH2.2)", accent_color=Theme.color.CYAN)

        desc = QLabel(
            "How each Zone Controller connects to Central Computing and which secured transport path is active."
        )
        desc.setWordWrap(True)
        desc.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px; border:none;"
        )
        card.card_layout.addWidget(desc)

        if not topology.zones:
            msg = QLabel("No zones added yet. Add zones from Architecture to visualize central connectivity.")
            msg.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_SMALL}px;"
                f"font-style:italic; border:none; padding:8px 0;"
            )
            card.card_layout.addWidget(msg)
            return card

        header = QGridLayout()
        header.setHorizontalSpacing(8)
        cols = ["Zone", "Zone IP", "Backbone", "Adapter", "Path to Central", "Security"]
        for i, h in enumerate(cols):
            lbl = QLabel(h)
            lbl.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none; background:{Theme.color.BG_ELEVATED};"
                f"padding:4px 8px; border-radius:4px;"
            )
            header.addWidget(lbl, 0, i)
        card.card_layout.addLayout(header)

        grid = QGridLayout()
        grid.setHorizontalSpacing(8)
        grid.setVerticalSpacing(4)
        for r, z in enumerate(topology.zones, start=1):
            if z.hardware == "Simulated (Virtual)":
                link_state = "SIM"
            elif z.ip_address:
                link_state = "UP"
            else:
                link_state = "DOWN"

            sec = "PQC" if any("PQC" in ecu.security for ecu in z.ecus) else "Classic"
            path = f"{z.name} -> HPCU -> Ethernet Backbone"
            vals = [
                z.name,
                z.ip_address or "N/A",
                z.backbone,
                z.can_adapter,
                path,
                f"{sec} ({link_state})",
            ]
            for c, val in enumerate(vals):
                cell = QLabel(val)
                cell.setWordWrap(c in {2, 4})
                color = Theme.color.PQC_PURPLE if (c == 5 and "PQC" in val) else Theme.color.TEXT_PRIMARY
                cell.setStyleSheet(
                    f"color:{color}; font-size:{Theme.font.SIZE_SMALL}px;"
                    f"border:none; padding:2px 4px;"
                )
                grid.addWidget(cell, r, c)
        card.card_layout.addLayout(grid)
        return card

    def _make_zone_to_gateway_card(self, zone: ZoneController) -> QWidget:
        card = StyledCard("ZONE -> CENTRAL CONNECTION (REF: CH2.1 / CH2.2)", accent_color=Theme.color.CYAN)
        status = "UP" if zone.ip_address else ("SIM" if zone.hardware == "Simulated (Virtual)" else "DOWN")
        sec_mode = "PQC" if any("PQC" in ecu.security for ecu in zone.ecus) else "Classic"
        line1 = QLabel(f"{zone.name} -> Zone Gateway (HPCU) -> Backend Ethernet")
        line2 = QLabel(
            f"Backbone: {zone.backbone} | Adapter: {zone.can_adapter} | Link: {status} | Security Path: {sec_mode}"
        )
        line1.setStyleSheet(
            f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_BODY}px; font-weight:600; border:none;"
        )
        line2.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px; border:none;"
        )
        card.card_layout.addWidget(line1)
        card.card_layout.addWidget(line2)
        return card

    def _make_pqc_focus_card(self, node_name: str, is_gateway: bool) -> QWidget:
        card = StyledCard("PQC THESIS FOCUS (REF: CH4 / CH6 / CH8)", accent_color=Theme.color.PQC_PURPLE)

        pqc_ecus = topology.pqc_secured_ecus
        total = topology.total_ecus
        coverage = (pqc_ecus / total * 100.0) if total else 0.0
        mode = state.auth_mode.name
        p50, p95 = self._latency_percentiles_us()

        intro = QLabel(
            "Main thesis objective: prove quantum-resistant secure communication is feasible at central gateway level."
        )
        intro.setWordWrap(True)
        intro.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px; border:none;"
        )
        card.card_layout.addWidget(intro)

        row = QHBoxLayout()
        row.setSpacing(8)
        row.addWidget(StatusBadge(f"Auth Mode: {mode}", "pqc" if mode == "PQC" else "idle"))
        row.addWidget(StatusBadge(f"PQC ECUs: {pqc_ecus}/{total}", "pqc" if pqc_ecus else "idle"))
        row.addWidget(StatusBadge(f"PQC Coverage: {coverage:.1f}%", "info"))
        row.addWidget(StatusBadge(f"P95: {p95:.1f} us", "warning" if p95 > 0 else "idle"))
        row.addStretch()
        card.card_layout.addLayout(row)

        detail = QLabel(
            f"{node_name} currently reports p50={p50:.1f}us, p95={p95:.1f}us, success={state.success_rate:.1f}%."
        )
        detail.setStyleSheet(
            f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px; border:none; padding-top:2px;"
        )
        card.card_layout.addWidget(detail)

        if is_gateway:
            note = QLabel("Central node signs/verifies secure Ethernet path while preserving CAN interoperability.")
            note.setStyleSheet(
                f"color:{Theme.color.PQC_PURPLE}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-style:italic; border:none; padding-top:2px;"
            )
            card.card_layout.addWidget(note)
        return card

    def _make_routing_control_card(self, node_name: str, is_gateway: bool) -> QWidget:
        card = StyledCard("CROSS-DOMAIN ROUTING EVIDENCE", accent_color=Theme.color.GREEN)

        blurb = QLabel(
            "Evidence for thesis: central node computes and enforces secure route paths."
        )
        blurb.setWordWrap(True)
        blurb.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none;"
        )
        card.card_layout.addWidget(blurb)

        row = QHBoxLayout()
        row.setSpacing(10)
        route_badge = StatusBadge("Global Router Active" if is_gateway else "Zone Routing Client", "info")
        row.addWidget(route_badge)
        sec_mode = "PQC-first" if state.auth_mode.name == "PQC" else "Classic"
        sec_badge = StatusBadge(
            f"Security Mode: {sec_mode}",
            "pqc" if sec_mode == "PQC-first" else "idle",
        )
        row.addWidget(sec_badge)
        row.addStretch()
        recalc_btn = IconButton("Recompute Routes", icon_text="\u21bb", style="outline")
        recalc_btn.clicked.connect(lambda: topology.auto_generate_routes())
        row.addWidget(recalc_btn)
        card.card_layout.addLayout(row)

        route_count = len(topology.routes)
        if route_count == 0 and topology.total_ecus >= 2:
            route_count = topology.total_ecus - 1
        summary = QLabel(
            f"Node: {node_name} \u00b7 Active routes: {route_count} \u00b7 "
            f"Cross-domain paths: {len(topology.active_domains)} \u00b7 Backbone: {topology.backbone_status}"
        )
        summary.setStyleSheet(
            f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none; padding-top:4px;"
        )
        card.card_layout.addWidget(summary)
        return card

    def _make_security_posture_card(self, node_name: str) -> QWidget:
        card = StyledCard("SECOC / PQC SECURITY EVIDENCE", accent_color=Theme.color.PQC_PURPLE)

        desc = QLabel(
            "Measured communication protection evidence from authentication, verification and attack counters."
        )
        desc.setWordWrap(True)
        desc.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none;"
        )
        card.card_layout.addWidget(desc)

        row = QHBoxLayout()
        row.setSpacing(10)
        row.addWidget(StatusBadge(f"Verify OK: {state.counters.verify_ok}", "ok"))
        row.addWidget(StatusBadge(f"Verify Fail: {state.counters.verify_fail}", "error" if state.counters.verify_fail else "idle"))
        row.addWidget(StatusBadge(f"Attacks Detected: {state.counters.attacks_detected}/{state.counters.attacks_total}", "warning" if state.counters.attacks_total else "idle"))
        row.addWidget(StatusBadge(f"Key Exchanges: {state.counters.key_exchanges}", "info"))
        row.addStretch()
        apply_btn = IconButton("Rotate Security Scenario", icon_text="\u2713", style="outline")
        apply_btn.setEnabled(False)
        apply_btn.setToolTip("Scenario orchestration hooks can be connected in next phase.")
        row.addWidget(apply_btn)
        card.card_layout.addLayout(row)

        note = QLabel(
            f"{node_name} trust boundary \u00b7 success rate: {state.success_rate:.1f}% \u00b7 auth mode: {state.auth_mode.name}"
        )
        note.setStyleSheet(
            f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
            f"font-style:italic; border:none; padding-top:4px;"
        )
        card.card_layout.addWidget(note)
        return card

    def _capture_baseline_snapshot(self):
        p50, p95 = self._latency_percentiles_us()
        self._baseline_snapshot = {
            "mode_is_pqc": 1.0 if state.auth_mode.name == "PQC" else 0.0,
            "avg_latency_us": float(state.avg_latency_us),
            "p50_latency_us": float(p50),
            "p95_latency_us": float(p95),
            "success_rate": float(state.success_rate),
            "verify_fail": float(state.counters.verify_fail),
            "attacks_detected": float(state.counters.attacks_detected),
            "tx_total": float(state.counters.tx_total),
            "rx_total": float(state.counters.rx_total),
        }
        self._last_event = "Baseline snapshot captured"
        self._refresh_live_view()

    def _make_thesis_scorecard_card(self, node_name: str) -> QWidget:
        card = StyledCard("THESIS SCORECARD (BASELINE VS CURRENT)", accent_color=Theme.color.CYAN)

        p50, p95 = self._latency_percentiles_us()
        curr = {
            "mode": state.auth_mode.name,
            "avg_latency_us": float(state.avg_latency_us),
            "p50_latency_us": float(p50),
            "p95_latency_us": float(p95),
            "success_rate": float(state.success_rate),
            "verify_fail": float(state.counters.verify_fail),
            "attacks_detected": float(state.counters.attacks_detected),
        }

        if self._baseline_snapshot is None:
            hint = QLabel("No baseline captured yet. Capture CLASSIC baseline first, then switch to PQC and compare.")
            hint.setWordWrap(True)
            hint.setStyleSheet(
                f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
                f"border:none;"
            )
            card.card_layout.addWidget(hint)
        else:
            b = self._baseline_snapshot
            delta_avg = curr["avg_latency_us"] - b["avg_latency_us"]
            delta_p95 = curr["p95_latency_us"] - b["p95_latency_us"]
            delta_sr = curr["success_rate"] - b["success_rate"]

            grid = QGridLayout()
            grid.setContentsMargins(0, 0, 0, 0)
            grid.setHorizontalSpacing(16)
            grid.setVerticalSpacing(4)
            rows = [
                ("Baseline Mode", "PQC" if b["mode_is_pqc"] > 0 else "CLASSIC"),
                ("Current Mode", curr["mode"]),
                ("Avg Latency (us)", f"{b['avg_latency_us']:.1f} -> {curr['avg_latency_us']:.1f} (d={delta_avg:+.1f})"),
                ("P95 Latency (us)", f"{b['p95_latency_us']:.1f} -> {curr['p95_latency_us']:.1f} (d={delta_p95:+.1f})"),
                ("Success Rate (%)", f"{b['success_rate']:.1f} -> {curr['success_rate']:.1f} (d={delta_sr:+.1f})"),
                ("Verify Fail Count", f"{int(b['verify_fail'])} -> {int(curr['verify_fail'])}"),
                ("Attacks Detected", f"{int(b['attacks_detected'])} -> {int(curr['attacks_detected'])}"),
            ]
            for i, (k, v) in enumerate(rows):
                kl = QLabel(k)
                kl.setStyleSheet(
                    f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                    f"font-weight:bold; border:none;"
                )
                vl = QLabel(v)
                vl.setStyleSheet(
                    f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_SMALL}px;"
                    f"font-family:{Theme.font.FAMILY_MONO}; border:none;"
                )
                grid.addWidget(kl, i, 0)
                grid.addWidget(vl, i, 1)
            card.card_layout.addLayout(grid)

        btn_row = QHBoxLayout()
        btn_row.setSpacing(8)
        cap_btn = IconButton("Capture Baseline", icon_text="\u25cf", style="outline")
        cap_btn.setToolTip("Capture current counters/latency as baseline for thesis comparison.")
        cap_btn.clicked.connect(self._capture_baseline_snapshot)
        btn_row.addWidget(cap_btn)
        btn_row.addStretch()
        card.card_layout.addLayout(btn_row)

        node_lbl = QLabel(f"Node: {node_name}")
        node_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
            f"font-style:italic; border:none; padding-top:2px;"
        )
        card.card_layout.addWidget(node_lbl)
        return card

    def _make_diagnostics_card(self, node_name: str, is_gateway: bool) -> QWidget:
        card = StyledCard("PERFORMANCE & DIAGNOSTICS EVIDENCE", accent_color=Theme.color.ORANGE)
        net = hw_configurator.network_info("eth0")
        can = hw_configurator.can_status("can0")
        role = "Central Gateway" if is_gateway else "Zone Controller"
        p50, p95 = self._latency_percentiles_us()

        diag_grid = QGridLayout()
        diag_grid.setContentsMargins(0, 0, 0, 0)
        diag_grid.setHorizontalSpacing(18)
        diag_grid.setVerticalSpacing(6)
        items = [
            ("Role", role),
            ("Node", node_name),
            ("Avg Verify Latency", f"{state.avg_latency_us:.1f} us"),
            ("P50 Verify Latency", f"{p50:.1f} us"),
            ("P95 Verify Latency", f"{p95:.1f} us"),
            ("Success Rate", f"{state.success_rate:.1f} %"),
            ("Tx / Rx Total", f"{state.counters.tx_total} / {state.counters.rx_total}"),
            ("ETH State", str(net.get("state", "unknown")).upper()),
            ("ETH IP", str(net.get("ip", "N/A"))),
            ("CAN State", str(can.get("state", "unknown")).upper()),
            ("CAN Bitrate", f"{int(can.get('bitrate', 0))} bps"),
            ("Last Event", self._last_event),
        ]
        for i, (k, v) in enumerate(items):
            k_lbl = QLabel(k)
            k_lbl.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none;"
            )
            v_lbl = QLabel(v)
            if k == "Last Event":
                v_lbl.setWordWrap(True)
            v_lbl.setStyleSheet(
                f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_SMALL}px;"
                f"font-family:{Theme.font.FAMILY_MONO}; border:none;"
            )
            diag_grid.addWidget(k_lbl, i, 0)
            diag_grid.addWidget(v_lbl, i, 1)
        card.card_layout.addLayout(diag_grid)

        actions = QHBoxLayout()
        actions.setSpacing(8)
        export_btn = IconButton("Export Evidence CSV", icon_text="\u2b07", style="outline")
        export_btn.clicked.connect(lambda: self._export_evidence_csv(node_name, role, p50, p95))
        actions.addWidget(export_btn)
        actions.addStretch()
        card.card_layout.addLayout(actions)
        return card

    def _make_interface_status_card(self, title_hint: str, fallback_ip: str) -> QWidget:
        card = StyledCard("INTERFACE STATUS PANEL", accent_color=Theme.color.GREEN)

        hint = QLabel(f"{title_hint} real-time interface state")
        hint.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_SMALL}px;"
            f"border:none; padding-bottom:4px;"
        )
        card.card_layout.addWidget(hint)

        can_info = hw_configurator.can_status("can0")
        net_info = hw_configurator.network_info("eth0")
        ip_text = net_info.get("ip", "N/A")
        if ip_text == "N/A" and fallback_ip:
            ip_text = fallback_ip

        spi_up = hw_configurator.spi_enabled()
        i2c_up = hw_configurator.i2c_enabled()
        can_up = can_info.get("state") == "up"
        can_bitrate = can_info.get("bitrate", 0)
        eth_up = net_info.get("state") == "up"

        rows = QVBoxLayout()
        rows.setSpacing(8)
        rows.addWidget(
            self._make_status_row(
                "SPI",
                "Enabled" if spi_up else "Disabled",
                "ok" if spi_up else "warning",
                "Disable" if spi_up else "Enable",
                hw_configurator.disable_spi if spi_up else hw_configurator.enable_spi,
            )
        )
        rows.addWidget(
            self._make_status_row(
                "I2C",
                "Enabled" if i2c_up else "Disabled",
                "ok" if i2c_up else "warning",
                "Disable" if i2c_up else "Enable",
                hw_configurator.disable_i2c if i2c_up else hw_configurator.enable_i2c,
            )
        )
        can_label = "UP" if can_up else "DOWN"
        if can_bitrate:
            can_label = f"{can_label} {can_bitrate // 1000}kbps"
        rows.addWidget(
            self._make_status_row(
                "CAN0",
                can_label,
                "ok" if can_up else "warning",
                "Configure...",
                self._open_can_dialog,
            )
        )
        rows.addWidget(
            self._make_status_row(
                "ETH",
                ip_text if eth_up else f"DOWN ({ip_text})",
                "ok" if eth_up else "warning",
                "Configure...",
                self._open_ip_dialog,
            )
        )
        card.card_layout.addLayout(rows)
        return card

    def _make_status_row(
        self,
        name: str,
        text: str,
        preset: str,
        button_text: str,
        callback,
    ) -> QWidget:
        row_w = QWidget()
        row = QHBoxLayout(row_w)
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(10)

        name_lbl = QLabel(f"{name}:")
        name_lbl.setMinimumWidth(46)
        name_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
            f"font-weight:600; border:none;"
        )
        row.addWidget(name_lbl)

        color_map = {
            "ok": Theme.color.GREEN,
            "warning": Theme.color.YELLOW_DARK,
            "error": Theme.color.RED,
            "idle": Theme.color.TEXT_MUTED,
            "info": Theme.color.CYAN_DARK,
        }
        dot_color = color_map.get(preset, Theme.color.TEXT_MUTED)

        dot = QLabel()
        dot.setFixedSize(10, 10)
        dot.setStyleSheet(f"background:{dot_color}; border:none; border-radius:5px;")
        row.addWidget(dot)

        state_lbl = QLabel(text)
        state_lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_BODY}px;"
            f"font-weight:600; border:none; background:transparent;"
        )
        row.addWidget(state_lbl)
        row.addStretch()

        btn = IconButton(button_text, style="outline")
        btn.setMinimumHeight(30)
        btn.setMinimumWidth(120)
        btn.clicked.connect(callback)
        row.addWidget(btn)
        return row_w

    def _open_can_dialog(self):
        dlg = CANConfigDialog(self, iface="can0")
        dlg.exec()

    def _open_ip_dialog(self):
        dlg = IPConfigDialog(self, iface="eth0")
        dlg.exec()

    def _make_zone_config_card(self, zone: ZoneController) -> QWidget:
        card = StyledCard(
            "ZONE CONTROLLER CONFIGURATION",
            accent_color=Theme.color.CYAN,
        )

        if zone.description:
            desc = QLabel(zone.description)
            desc.setStyleSheet(
                f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
                f"border:none; padding-bottom:8px;"
            )
            card.card_layout.addWidget(desc)

        form = QGridLayout()
        form.setSpacing(10)
        form.setContentsMargins(0, 0, 0, 0)

        row = 0

        # Hardware
        form.addWidget(self._form_label("Board"), row, 0)
        hw_combo = QComboBox()
        hw_combo.setStyleSheet(Theme.stylesheet.COMBO)
        hw_combo.addItems(ZONE_HARDWARE_OPTIONS)
        hw_combo.setCurrentText(zone.hardware)
        uid = zone.uid
        hw_combo.currentTextChanged.connect(
            lambda val, u=uid: topology.update_zone(u, hardware=val)
        )
        form.addWidget(hw_combo, row, 1)

        zone_badge = StatusBadge("Zone Controller", "idle")
        form.addWidget(zone_badge, row, 2)
        row += 1

        # IP Address
        form.addWidget(self._form_label("IP Address"), row, 0)
        ip_edit = QLineEdit(zone.ip_address)
        ip_edit.setStyleSheet(Theme.stylesheet.INPUT)
        ip_edit.setPlaceholderText("e.g. 192.168.1.11")
        ip_edit.editingFinished.connect(
            lambda u=uid: topology.update_zone(u, ip_address=ip_edit.text())
        )
        form.addWidget(ip_edit, row, 1)
        row += 1

        # Backbone
        form.addWidget(self._form_label("Backbone"), row, 0)
        bb_combo = QComboBox()
        bb_combo.setStyleSheet(Theme.stylesheet.COMBO)
        bb_combo.addItems(BACKBONE_OPTIONS)
        bb_combo.setCurrentText(zone.backbone)
        bb_combo.currentTextChanged.connect(
            lambda val, u=uid: topology.update_zone(u, backbone=val)
        )
        form.addWidget(bb_combo, row, 1)
        row += 1

        # CAN Adapter
        form.addWidget(self._form_label("CAN Adapter"), row, 0)
        can_combo = QComboBox()
        can_combo.setStyleSheet(Theme.stylesheet.COMBO)
        can_combo.addItems(CAN_ADAPTER_OPTIONS)
        can_combo.setCurrentText(zone.can_adapter)
        can_combo.currentTextChanged.connect(
            lambda val, u=uid: topology.update_zone(u, can_adapter=val)
        )
        form.addWidget(can_combo, row, 1)
        row += 1

        card.card_layout.addLayout(form)
        return card

    # ══════════════════════════════════════════════════════════════════
    # SHARED section builders
    # ══════════════════════════════════════════════════════════════════

    # ── CAN Adapter Wiring card ──────────────────────────────────────

    def _make_adapter_card(self, can_adapter: str) -> QWidget:
        wiring = ADAPTER_WIRING.get(can_adapter, {})

        card = StyledCard("EDGE BUS ADAPTER PROFILE", accent_color=Theme.color.ORANGE)

        info = QHBoxLayout()
        info.setSpacing(16)
        for label, value in [
            ("Adapter", can_adapter),
            ("Interface", wiring.get("interface", "N/A")),
            ("Transceiver", wiring.get("transceiver", "N/A")),
        ]:
            col = QVBoxLayout()
            col.setSpacing(1)
            h = QLabel(label)
            h.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none;"
            )
            v = QLabel(value)
            v.setStyleSheet(
                f"color:{Theme.color.TEXT_PRIMARY}; font-size:{Theme.font.SIZE_BODY}px;"
                f"font-weight:600; border:none;"
            )
            col.addWidget(h)
            col.addWidget(v)
            info.addLayout(col)
        info.addStretch()
        card.card_layout.addLayout(info)

        pins = wiring.get("pins", [])
        if not pins:
            if wiring.get("interface") == "USB":
                msg = QLabel(
                    "USB adapter \u2014 connects via USB port, no GPIO wiring needed."
                )
            else:
                msg = QLabel("No physical wiring for this adapter configuration.")
            msg.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_BODY}px;"
                f"font-style:italic; padding:12px 0; border:none;"
            )
            card.card_layout.addWidget(msg)
        else:
            headers = [
                "Adapter Pin", "Function", "RPi GPIO", "Physical Pin", "Notes",
            ]
            grid = QGridLayout()
            grid.setSpacing(1)
            for ci, h in enumerate(headers):
                lbl = QLabel(h)
                lbl.setStyleSheet(
                    f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                    f"font-weight:bold; padding:6px 8px; border:none;"
                    f"background:{Theme.color.BG_ELEVATED}; border-radius:4px;"
                )
                grid.addWidget(lbl, 0, ci)

            for ri, p in enumerate(pins, start=1):
                vals = [
                    p["adapter"],
                    p["function"],
                    f"GPIO{p['gpio']}" if p["gpio"] is not None else "\u2014",
                    str(p["pin"]),
                    p.get("notes", ""),
                ]
                for ci, val in enumerate(vals):
                    cell = QLabel(val)
                    cell.setStyleSheet(
                        f"color:{Theme.color.TEXT_PRIMARY};"
                        f"font-size:{Theme.font.SIZE_SMALL}px;"
                        f"padding:4px 8px; border:none;"
                        f"font-family:{Theme.font.FAMILY_MONO};"
                    )
                    grid.addWidget(cell, ri, ci)

            card.card_layout.addLayout(grid)

            tip = QLabel(
                "Tip: Enable SPI via raspi-config \u2192 Interface Options \u2192 SPI"
            )
            tip.setStyleSheet(
                f"color:{Theme.color.CYAN_DARK}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-style:italic; padding:8px 0 0 0; border:none;"
            )
            card.card_layout.addWidget(tip)

        return card

    # ── Peripheral Summary card (zone only) ──────────────────────────

    def _make_peripheral_card(self, zone: ZoneController) -> QWidget:
        card = StyledCard("PERIPHERAL SUMMARY", accent_color=Theme.color.GREEN)

        bus_info: dict[str, list[str]] = {}
        for ecu in zone.ecus:
            bus_info.setdefault(ecu.local_bus, []).append(ecu.name)

        if not bus_info:
            msg = QLabel("No ECUs configured in this zone.")
            msg.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_BODY}px;"
                f"font-style:italic; padding:12px 0; border:none;"
            )
            card.card_layout.addWidget(msg)
        else:
            row = QHBoxLayout()
            row.setSpacing(12)
            for bus, ecus in bus_info.items():
                mini = QFrame()
                mini.setStyleSheet(
                    f"QFrame {{ background:{Theme.color.BG_ELEVATED};"
                    f"border:1px solid {Theme.color.BORDER}; border-radius:10px;"
                    f"padding:10px; }}"
                )
                ml = QVBoxLayout(mini)
                ml.setSpacing(4)

                bus_lbl = QLabel(bus)
                bus_lbl.setStyleSheet(
                    f"color:{Theme.color.CYAN_DARK};"
                    f"font-size:{Theme.font.SIZE_BODY}px;"
                    f"font-weight:bold; border:none;"
                )
                ml.addWidget(bus_lbl)

                count_lbl = QLabel(
                    f"{len(ecus)} ECU{'s' if len(ecus) > 1 else ''}"
                )
                count_lbl.setStyleSheet(
                    f"color:{Theme.color.TEXT_SECONDARY};"
                    f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
                )
                ml.addWidget(count_lbl)

                for name in ecus:
                    e_lbl = QLabel(f"\u2022 {name}")
                    e_lbl.setStyleSheet(
                        f"color:{Theme.color.TEXT_MUTED};"
                        f"font-size:{Theme.font.SIZE_TINY}px; border:none;"
                    )
                    ml.addWidget(e_lbl)

                row.addWidget(mini)
            row.addStretch()
            card.card_layout.addLayout(row)

        return card

    # ── ECU Interface Matrix card (zone only) ────────────────────────

    def _make_ecu_matrix_card(self, zone: ZoneController) -> QWidget:
        card = StyledCard(
            "ECU INTERFACE MATRIX", accent_color=Theme.color.PQC_PURPLE,
        )

        if not zone.ecus:
            msg = QLabel("No ECUs in this zone.")
            msg.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_BODY}px;"
                f"font-style:italic; padding:12px 0; border:none;"
            )
            card.card_layout.addWidget(msg)
            return card

        headers = [
            "ECU Name", "MCU", "Bus", "Node Addr", "Interface Pins", "Status",
        ]
        grid = QGridLayout()
        grid.setSpacing(1)

        for ci, h in enumerate(headers):
            lbl = QLabel(h)
            lbl.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; padding:6px 8px; border:none;"
                f"background:{Theme.color.BG_ELEVATED}; border-radius:4px;"
            )
            grid.addWidget(lbl, 0, ci)

        for ri, ecu in enumerate(zone.ecus, start=1):
            periph = MCU_PERIPHERALS.get(ecu.mcu_model, {})
            bus_key = ecu.local_bus.lower().replace("-", "").replace(" ", "")
            if "can" in bus_key:
                iface_pins = periph.get("can", "N/A")
            elif "ethernet" in bus_key:
                iface_pins = periph.get("ethernet", "N/A")
            elif "lin" in bus_key:
                iface_pins = periph.get("uart", "N/A")
            elif "flexray" in bus_key:
                iface_pins = periph.get("can", "N/A")
            else:
                iface_pins = "N/A"

            status_colors = {
                "Connected": Theme.color.GREEN,
                "Simulated": Theme.color.TEXT_MUTED,
                "Disconnected": Theme.color.RED,
            }
            dot_color = status_colors.get(ecu.status, Theme.color.TEXT_MUTED)

            vals = [
                ecu.name, ecu.mcu_model, ecu.local_bus,
                ecu.node_address, iface_pins,
            ]
            for ci, val in enumerate(vals):
                cell = QLabel(val)
                cell.setWordWrap(True)
                cell.setStyleSheet(
                    f"color:{Theme.color.TEXT_PRIMARY};"
                    f"font-size:{Theme.font.SIZE_SMALL}px;"
                    f"padding:4px 8px; border:none;"
                    f"font-family:{Theme.font.FAMILY_MONO};"
                )
                grid.addWidget(cell, ri, ci)

            # Status column
            sf = QFrame()
            sf.setStyleSheet("border:none; background:transparent;")
            sl = QHBoxLayout(sf)
            sl.setContentsMargins(8, 4, 8, 4)
            sl.setSpacing(4)
            sdot = QLabel()
            sdot.setFixedSize(8, 8)
            sdot.setStyleSheet(
                f"background:{dot_color}; border-radius:4px; border:none;"
            )
            sl.addWidget(sdot)
            stxt = QLabel(ecu.status)
            stxt.setStyleSheet(
                f"color:{Theme.color.TEXT_SECONDARY};"
                f"font-size:{Theme.font.SIZE_SMALL}px; border:none;"
            )
            sl.addWidget(stxt)
            sl.addStretch()
            grid.addWidget(sf, ri, 5)

        card.card_layout.addLayout(grid)
        return card

    # ── Helpers ──────────────────────────────────────────────────────

    @staticmethod
    def _form_label(text: str) -> QLabel:
        lbl = QLabel(text)
        lbl.setFixedWidth(100)
        lbl.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
            f"font-weight:600; border:none;"
        )
        return lbl
