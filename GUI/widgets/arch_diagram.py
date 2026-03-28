"""
Interactive Zonal Gateway topology diagram — drag & click-to-configure.

Based on automotive Zonal E/E Architecture research:
  • Central Zone Gateway (HPCU) — always present, Ethernet backbone hub
  • Zone Controllers — physical-location clusters, each hosting ECUs
  • ECU domains: Powertrain, Body, Chassis, ADAS, Infotainment

Interactive features:
  • Left-click zone  → opens ZoneConfigDialog
  • Drag zone        → reposition on canvas
  • Right-click zone → Configure / Add ECU / Rename / Remove
  • Right-click ECU  → view info / Remove
  • Right-click empty → Add Zone / Load Preset / Clear
  • Zones auto-layout with drag-position override
  • Live state animation (idle / active / PQC / error)
"""

from __future__ import annotations

import math
from PySide6.QtCore import Qt, QRectF, QPointF, QTimer, Signal
from PySide6.QtGui import (
    QColor,
    QFont,
    QMouseEvent,
    QPainter,
    QPen,
    QPainterPath,
    QLinearGradient,
    QAction,
)
from PySide6.QtWidgets import (
    QWidget,
    QMenu,
    QInputDialog,
)

from config.theme import Theme
from core.topology import topology, ECU_DOMAINS, ZoneController, ECUConfig


# ── Visual state enum ─────────────────────────────────────────────────

class _NodeState:
    IDLE   = 0
    ACTIVE = 1
    ERROR  = 2
    PQC    = 3


# ── Drag threshold ────────────────────────────────────────────────────

_DRAG_THRESHOLD = 5  # pixels of movement before a press becomes a drag


# ── Interactive Architecture Diagram ──────────────────────────────────

class ArchDiagram(QWidget):
    """
    Custom-painted interactive zonal gateway architecture.

    The topology is driven by ``core.topology.topology`` model.

    - **Left-click** a zone → emits ``zone_selected(uid)`` (opens config).
    - **Drag** a zone → reposition it freely on the canvas.
    - **Right-click** → context menus to add/remove zones and ECUs.
    """

    zone_selected = Signal(str)        # zone UID — opens config dialog
    topology_changed = Signal()        # re-emitted after model mutation

    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent)
        self.setMinimumHeight(380)
        self.setMinimumWidth(640)
        self.setMouseTracking(True)

        # Visual states for gateway + dynamic zones
        self._node_states: dict[str, int] = {"gateway": _NodeState.IDLE}

        # Hit-test rectangles (rebuilt every paintEvent)
        self._zone_rects: dict[str, QRectF] = {}      # zone uid → painted rect
        self._ecu_circles: dict[str, QRectF] = {}     # "zone:ecu" → bounding circle
        self._ecu_map: dict[str, tuple[str, str]] = {} # "zone:ecu" → (zone_uid, ecu_uid)
        self._gateway_rect = QRectF()

        # Hover tracking
        self._hover_uid: str = ""

        # ── Drag state ────────────────────────────────────────────────
        self._drag_uid: str = ""
        self._drag_start: QPointF = QPointF()
        self._drag_offset: QPointF = QPointF()
        self._is_dragging: bool = False
        self._zone_positions: dict[str, QPointF] = {}  # uid → custom center

        # Pulse animation
        self._pulse = 0.0
        self._pulse_timer = QTimer(self)
        self._pulse_timer.timeout.connect(self._tick_pulse)
        self._pulse_timer.start(40)

        # Flow animation
        self._anim_progress = 0.0
        self._flow_timer: QTimer | None = None

        # Listen to topology changes
        topology.changed.connect(self._on_topology_changed)

    # ── Public API ─────────────────────────────────────────────────────

    def set_node_state(self, node_id: str, s: int) -> None:
        self._node_states[node_id] = s
        self.update()

    def reset_all(self) -> None:
        self._node_states = {"gateway": _NodeState.IDLE}
        self._zone_positions.clear()
        self.update()

    def animate_flow(self) -> None:
        self._anim_progress = 0.0
        if self._flow_timer is None:
            self._flow_timer = QTimer(self)
            self._flow_timer.timeout.connect(self._tick_flow)
        self._flow_timer.start(30)

    # ── Topology listener ──────────────────────────────────────────────

    def _on_topology_changed(self):
        # Purge custom positions for removed zones
        alive = {z.uid for z in topology.zones}
        self._zone_positions = {
            k: v for k, v in self._zone_positions.items() if k in alive
        }
        self.update()
        self.topology_changed.emit()

    # ── Timers ─────────────────────────────────────────────────────────

    def _tick_pulse(self):
        self._pulse = (self._pulse + 0.05) % (2 * math.pi)
        if any(s != _NodeState.IDLE for s in self._node_states.values()):
            self.update()

    def _tick_flow(self):
        self._anim_progress += 0.015
        if self._anim_progress >= 1.0:
            self._anim_progress = 0.0
            if self._flow_timer:
                self._flow_timer.stop()
        self.update()

    # ── Colour helpers ─────────────────────────────────────────────────

    def _state_color(self, s: int) -> QColor:
        m = {
            _NodeState.IDLE:   Theme.color.BORDER,
            _NodeState.ACTIVE: Theme.color.CYAN,
            _NodeState.ERROR:  Theme.color.RED,
            _NodeState.PQC:    Theme.color.PQC_PURPLE,
        }
        c = QColor(m.get(s, Theme.color.BORDER))
        if s != _NodeState.IDLE:
            f = 0.90 + 0.10 * (math.sin(self._pulse) + 1) / 2
            c = c.lighter(int(100 * f))
        return c

    @staticmethod
    def _state_fill(s: int) -> QColor:
        if s == _NodeState.ACTIVE:
            return QColor(Theme.color.CYAN_LIGHT)
        if s == _NodeState.ERROR:
            return QColor(Theme.color.RED_LIGHT)
        if s == _NodeState.PQC:
            return QColor(Theme.color.PQC_PURPLE_LIGHT)
        return QColor(Theme.color.BG_ELEVATED)

    @staticmethod
    def _ecu_color(domain: str) -> QColor:
        info = ECU_DOMAINS.get(domain, {})
        attr = info.get("color_attr", "TEXT_MUTED")
        return QColor(getattr(Theme.color, attr, Theme.color.TEXT_MUTED))

    @staticmethod
    def _font(size: int = 9, bold: bool = False) -> QFont:
        f = QFont(Theme.font.FAMILY_PRIMARY.split(",")[0].strip("'"), size)
        f.setBold(bold)
        return f

    # ── Layout calculation ─────────────────────────────────────────────

    def _calc_layout(self, w: float, h: float):
        """
        Calculate zone positions — auto-layout with drag overrides.

        Default: top/bottom grid around the gateway.
        Zones dragged to custom positions keep their position.
        """
        zones = topology.zones
        n = len(zones)

        cx, cy = w / 2, h / 2
        zone_w, zone_h = 130, 54
        gw_w, gw_h = 190, 54

        # Gateway always centred
        self._gateway_rect = QRectF(cx - gw_w / 2, cy - gw_h / 2, gw_w, gw_h)

        if n == 0:
            return cx, cy, zone_w, zone_h, gw_w, gw_h, []

        # Auto-layout defaults
        top_n = math.ceil(n / 2)
        bot_n = n - top_n
        margin_x = 30
        margin_y = 40

        defaults: list[QPointF] = []

        # Top row
        top_y = margin_y + zone_h / 2 + 8
        if top_n > 0:
            usable_w = w - 2 * margin_x
            spacing = usable_w / top_n
            for i in range(top_n):
                x = margin_x + spacing * (i + 0.5)
                defaults.append(QPointF(x, top_y))

        # Bottom row
        bot_y = h - margin_y - zone_h / 2 - 24
        if bot_n > 0:
            usable_w = w - 2 * margin_x
            spacing = usable_w / bot_n
            for i in range(bot_n):
                x = margin_x + spacing * (i + 0.5)
                defaults.append(QPointF(x, bot_y))

        # Merge: custom drag positions override auto-layout
        positions: list[tuple[ZoneController, QPointF]] = []
        for idx, zone in enumerate(zones):
            if zone.uid in self._zone_positions:
                pos = self._zone_positions[zone.uid]
            elif idx < len(defaults):
                pos = defaults[idx]
            else:
                pos = QPointF(cx, cy)
            positions.append((zone, pos))

        return cx, cy, zone_w, zone_h, gw_w, gw_h, positions

    # ── Paint ──────────────────────────────────────────────────────────

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        p.setRenderHint(QPainter.TextAntialiasing)

        w, h = self.width(), self.height()
        cx, cy, zone_w, zone_h, gw_w, gw_h, positions = self._calc_layout(w, h)

        gw_center = QPointF(cx, cy)

        # Reset hit-test maps
        self._zone_rects.clear()
        self._ecu_circles.clear()
        self._ecu_map.clear()

        # ── Car silhouette ─────────────────────────────────────────────
        self._draw_car_silhouette(p, cx, cy, w, h)

        # ── Connection lines (behind nodes) ────────────────────────────
        for zone, pt in positions:
            s = self._node_states.get(zone.uid, _NodeState.IDLE)
            lc = (
                self._state_color(s)
                if s != _NodeState.IDLE
                else QColor(Theme.color.TEXT_MUTED)
            )
            lc.setAlpha(100 if s == _NodeState.IDLE else 180)
            pen = QPen(lc, 2.0)
            pen.setCapStyle(Qt.RoundCap)
            p.setPen(pen)
            p.drawLine(gw_center, pt)

        # ── Flow animation dots ────────────────────────────────────────
        if self._anim_progress > 0:
            for _, pt in positions:
                t = self._anim_progress
                dot = QPointF(
                    gw_center.x() + (pt.x() - gw_center.x()) * t,
                    gw_center.y() + (pt.y() - gw_center.y()) * t,
                )
                p.setPen(Qt.NoPen)
                dc = QColor(Theme.color.CYAN)
                dc.setAlpha(200)
                p.setBrush(dc)
                p.drawEllipse(dot, 4, 4)
                for tr in range(1, 3):
                    tt = max(0, t - tr * 0.06)
                    trail = QPointF(
                        gw_center.x() + (pt.x() - gw_center.x()) * tt,
                        gw_center.y() + (pt.y() - gw_center.y()) * tt,
                    )
                    tc = QColor(Theme.color.CYAN)
                    tc.setAlpha(80 - tr * 25)
                    p.setBrush(tc)
                    p.drawEllipse(trail, 3, 3)

        # ── Central Gateway node ───────────────────────────────────────
        self._draw_gateway_node(p, gw_center, gw_w, gw_h)

        # ── Zone Controller nodes ──────────────────────────────────────
        for zone, pt in positions:
            s = self._node_states.get(zone.uid, _NodeState.IDLE)
            hovered = zone.uid == self._hover_uid
            dragging = zone.uid == self._drag_uid and self._is_dragging
            self._draw_zone_node(p, pt, zone_w, zone_h, zone, s, hovered, dragging)

        # ── Legend ─────────────────────────────────────────────────────
        self._draw_legend(p, w, h)

        # ── Empty-state hint ───────────────────────────────────────────
        if not positions:
            p.setPen(QColor(Theme.color.TEXT_MUTED))
            p.setFont(self._font(11))
            p.drawText(
                QRectF(0, cy + gw_h, w, 60),
                Qt.AlignCenter,
                "Right-click to add a Zone Controller",
            )

        p.end()

    # ── Sub-draw: car silhouette ───────────────────────────────────────

    def _draw_car_silhouette(self, p: QPainter, cx, cy, w, h):
        path = QPainterPath()
        car_w = w * 0.70
        car_h = h * 0.82
        car_r = QRectF(cx - car_w / 2, cy - car_h / 2, car_w, car_h)
        path.addRoundedRect(car_r, car_w * 0.28, car_h * 0.14)
        p.setPen(Qt.NoPen)
        fill = QColor(Theme.color.TEXT_MUTED)
        fill.setAlpha(12)
        p.setBrush(fill)
        p.drawPath(path)

        # Windshield
        ws = QRectF(
            cx - car_w * 0.22, cy - car_h * 0.38,
            car_w * 0.44, car_h * 0.08,
        )
        ws_c = QColor(Theme.color.TEXT_MUTED)
        ws_c.setAlpha(10)
        p.setBrush(ws_c)
        p.drawRoundedRect(ws, 8, 8)

        # Rear window
        rw = QRectF(
            cx - car_w * 0.18, cy + car_h * 0.30,
            car_w * 0.36, car_h * 0.06,
        )
        p.drawRoundedRect(rw, 6, 6)

    # ── Sub-draw: gateway ──────────────────────────────────────────────

    def _draw_gateway_node(self, p: QPainter, center: QPointF, w, h):
        gw_state = self._node_states.get("gateway", _NodeState.IDLE)
        rect = QRectF(center.x() - w / 2, center.y() - h / 2, w, h)

        # Glow
        glow_c = (
            self._state_color(gw_state)
            if gw_state != _NodeState.IDLE
            else QColor(Theme.color.CYAN)
        )
        glow_c.setAlpha(25)
        p.setPen(Qt.NoPen)
        p.setBrush(glow_c)
        p.drawRoundedRect(rect.adjusted(-6, -6, 6, 6), 18, 18)

        # Fill
        grad = QLinearGradient(rect.topLeft(), rect.bottomRight())
        grad.setColorAt(0, QColor("#3B82F6"))
        grad.setColorAt(1, QColor("#2563EB"))
        p.setBrush(grad)
        p.setPen(Qt.NoPen)
        p.drawRoundedRect(rect, 14, 14)

        # Text — two lines: role + hardware
        p.setPen(QColor("#FFFFFF"))
        p.setFont(self._font(10, bold=True))
        top_text = QRectF(rect.left(), rect.top() + 4, rect.width(), rect.height() * 0.55)
        p.drawText(top_text, Qt.AlignCenter, "Zone Gateway (HPCU)")

        p.setFont(self._font(8))
        p.setPen(QColor(255, 255, 255, 180))
        bot_text = QRectF(rect.left(), rect.top() + rect.height() * 0.50, rect.width(), rect.height() * 0.45)
        p.drawText(bot_text, Qt.AlignCenter, "Raspberry Pi 4  \u00b7  ARM Cortex-A72")

        # Badge
        badge_w, badge_h = 96, 18
        badge_r = QRectF(
            center.x() - badge_w / 2, rect.bottom() + 4,
            badge_w, badge_h,
        )
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(Theme.color.PQC_PURPLE_LIGHT))
        p.drawRoundedRect(badge_r, 9, 9)
        p.setPen(QColor(Theme.color.PQC_PURPLE))
        p.setFont(self._font(8, bold=True))
        p.drawText(badge_r, Qt.AlignCenter, "SecOC + PQC")

    # ── Sub-draw: zone controller ──────────────────────────────────────

    def _draw_zone_node(
        self,
        p: QPainter,
        center: QPointF,
        w: float,
        h: float,
        zone: ZoneController,
        state: int,
        is_hovered: bool,
        is_dragging: bool = False,
    ):
        rect = QRectF(center.x() - w / 2, center.y() - h / 2, w, h)
        self._zone_rects[zone.uid] = rect

        # Shadow (deeper when dragging)
        if is_dragging:
            s_r = rect.adjusted(2, 5, 2, 5)
            p.setPen(Qt.NoPen)
            p.setBrush(QColor(0, 0, 0, 22))
            p.drawRoundedRect(s_r, 12, 12)
        else:
            s_r = rect.adjusted(1, 2, 1, 2)
            p.setPen(Qt.NoPen)
            p.setBrush(QColor(0, 0, 0, 10))
            p.drawRoundedRect(s_r, 12, 12)

        # Fill
        fill = self._state_fill(state)
        if is_hovered or is_dragging:
            fill = fill.darker(105)
        p.setBrush(fill)

        # Border
        border_c = (
            self._state_color(state)
            if state != _NodeState.IDLE
            else QColor(Theme.color.BORDER)
        )
        if is_hovered or is_dragging:
            border_c = QColor(Theme.color.CYAN)
        pen_w = 2.0 if (is_hovered or is_dragging) else 1.5
        p.setPen(QPen(border_c, pen_w))
        p.drawRoundedRect(rect, 12, 12)

        # Label — zone name + hardware abbreviation
        p.setPen(QColor(Theme.color.TEXT_PRIMARY))
        p.setFont(self._font(8, bold=True))
        name_rect = QRectF(
            rect.left() + 4, rect.top() + 3,
            rect.width() - 8, rect.height() * 0.38,
        )
        p.drawText(name_rect, Qt.AlignCenter, zone.name)

        # Hardware hint (abbreviated)
        hw_short = zone.hardware.replace(" Model B", "").replace(" Model B+", "")
        if hw_short.startswith("Simulated"):
            hw_short = "Simulated"
        p.setPen(QColor(Theme.color.TEXT_MUTED))
        p.setFont(self._font(7))
        hw_rect = QRectF(
            rect.left() + 4, rect.top() + rect.height() * 0.34,
            rect.width() - 8, rect.height() * 0.24,
        )
        p.drawText(hw_rect, Qt.AlignCenter, hw_short)

        # ECU dots — each ecu is an ECUConfig object
        if zone.ecus:
            dot_r = 5
            total_w = len(zone.ecus) * (dot_r * 2 + 4) - 4
            start_x = center.x() - total_w / 2 + dot_r
            dot_y = rect.bottom() - dot_r - 4

            for i, ecu in enumerate(zone.ecus):
                dx = start_x + i * (dot_r * 2 + 4)
                p.setPen(Qt.NoPen)
                p.setBrush(self._ecu_color(ecu.domain))
                p.drawEllipse(QPointF(dx, dot_y), dot_r, dot_r)

                # Hit-test
                key = f"{zone.uid}:{ecu.uid}"
                self._ecu_circles[key] = QRectF(
                    dx - dot_r, dot_y - dot_r,
                    dot_r * 2, dot_r * 2,
                )
                self._ecu_map[key] = (zone.uid, ecu.uid)
        else:
            p.setPen(QColor(Theme.color.TEXT_MUTED))
            p.setFont(self._font(7))
            p.drawText(
                QRectF(rect.left(), rect.bottom() - 16, rect.width(), 14),
                Qt.AlignCenter,
                "no ECUs",
            )

    # ── Sub-draw: legend ───────────────────────────────────────────────

    def _draw_legend(self, p: QPainter, w: float, h: float):
        p.setFont(self._font(8))
        legend_y = h - 12
        items = list(ECU_DOMAINS.items())
        item_w = 110
        total_w = len(items) * item_w
        start_x = (w - total_w) / 2

        for i, (name, _info) in enumerate(items):
            x = start_x + i * item_w
            p.setPen(Qt.NoPen)
            p.setBrush(self._ecu_color(name))
            p.drawEllipse(QPointF(x + 5, legend_y), 5, 5)
            p.setPen(QColor(Theme.color.TEXT_SECONDARY))
            p.drawText(
                QRectF(x + 14, legend_y - 8, item_w - 16, 16),
                Qt.AlignVCenter | Qt.AlignLeft,
                f": {name}",
            )

    # ── Mouse: press / move / release  (drag + click + hover) ─────────

    def mousePressEvent(self, ev: QMouseEvent):
        if ev.button() == Qt.LeftButton:
            pos = ev.position()
            for uid, rect in self._zone_rects.items():
                if rect.contains(pos):
                    self._drag_uid = uid
                    self._drag_start = QPointF(pos)
                    self._drag_offset = QPointF(
                        rect.center().x() - pos.x(),
                        rect.center().y() - pos.y(),
                    )
                    self._is_dragging = False
                    return
        super().mousePressEvent(ev)

    def mouseMoveEvent(self, ev: QMouseEvent):
        pos = ev.position()

        # ── Active drag operation ──────────────────────────────────────
        if self._drag_uid:
            delta = pos - self._drag_start
            if not self._is_dragging and (
                abs(delta.x()) + abs(delta.y())
            ) > _DRAG_THRESHOLD:
                self._is_dragging = True
                self.setCursor(Qt.ClosedHandCursor)
            if self._is_dragging:
                new_center = QPointF(
                    pos.x() + self._drag_offset.x(),
                    pos.y() + self._drag_offset.y(),
                )
                self._zone_positions[self._drag_uid] = new_center
                self.update()
            return

        # ── Hover tracking (no drag in progress) ──────────────────────
        old_hover = self._hover_uid
        self._hover_uid = ""
        for uid, rect in self._zone_rects.items():
            if rect.contains(pos):
                self._hover_uid = uid
                break
        if self._hover_uid != old_hover:
            self.setCursor(
                Qt.PointingHandCursor if self._hover_uid else Qt.ArrowCursor
            )
            self.update()

    def mouseReleaseEvent(self, ev: QMouseEvent):
        if ev.button() == Qt.LeftButton and self._drag_uid:
            uid = self._drag_uid
            self._drag_uid = ""
            if not self._is_dragging:
                # Short click — open config dialog
                self.zone_selected.emit(uid)
            self._is_dragging = False
            self.setCursor(
                Qt.PointingHandCursor if self._hover_uid else Qt.ArrowCursor
            )
            return
        super().mouseReleaseEvent(ev)

    def contextMenuEvent(self, ev):
        pos = ev.pos()

        # Check ECU dots first (they sit on top of zone rects)
        for key, circ in self._ecu_circles.items():
            if circ.contains(QPointF(pos)):
                zone_uid, ecu_uid = self._ecu_map[key]
                self._show_ecu_menu(ev.globalPos(), zone_uid, ecu_uid)
                return

        # Check zones
        for uid, rect in self._zone_rects.items():
            if rect.contains(QPointF(pos)):
                self._show_zone_menu(ev.globalPos(), uid)
                return

        # Empty space
        self._show_canvas_menu(ev.globalPos())

    # ── Context menus ──────────────────────────────────────────────────

    def _show_canvas_menu(self, global_pos):
        menu = QMenu(self)
        menu.setStyleSheet(self._menu_style())

        add_action = menu.addAction("Add Zone Controller")
        preset_action = menu.addAction("Load 6-Zone Preset")
        menu.addSeparator()
        clear_action = menu.addAction("Clear All Zones")
        reset_pos = menu.addAction("Reset All Positions")

        action = menu.exec_(global_pos)
        if action == add_action:
            self._do_add_zone()
        elif action == preset_action:
            self._zone_positions.clear()
            topology.load_6zone_preset()
        elif action == clear_action:
            self._zone_positions.clear()
            topology.clear()
        elif action == reset_pos:
            self._zone_positions.clear()
            self.update()

    def _show_zone_menu(self, global_pos, zone_uid: str):
        zone = topology.zone_by_uid(zone_uid)
        if not zone:
            return

        menu = QMenu(self)
        menu.setStyleSheet(self._menu_style())

        # Info header
        info = menu.addAction(f"{zone.name}  \u00b7  {zone.hardware}")
        info.setEnabled(False)
        if zone.ip_address:
            ip_info = menu.addAction(f"IP: {zone.ip_address}  \u00b7  {zone.backbone}")
            ip_info.setEnabled(False)

        menu.addSeparator()
        config_action = menu.addAction("Configure Zone\u2026")
        menu.addSeparator()

        # Quick-add ECU by domain
        existing_domains = {ecu.domain for ecu in zone.ecus}
        ecu_menu = menu.addMenu("Quick-Add ECU")
        ecu_actions: dict[QAction, str] = {}
        for domain in ECU_DOMAINS:
            if domain not in existing_domains:
                a = ecu_menu.addAction(domain)
                ecu_actions[a] = domain
        if not ecu_actions:
            ecu_menu.addAction("(all domains present)").setEnabled(False)

        menu.addSeparator()
        rename_action = menu.addAction("Rename Zone")
        reset_pos_action = menu.addAction("Reset Position")
        menu.addSeparator()
        remove_action = menu.addAction("Remove Zone")

        action = menu.exec_(global_pos)
        if action == config_action:
            self.zone_selected.emit(zone_uid)
        elif action in ecu_actions:
            domain = ecu_actions[action]
            topology.add_ecu(zone_uid, name=f"{domain} ECU", domain=domain)
        elif action == rename_action:
            self._do_rename_zone(zone_uid, zone.name)
        elif action == reset_pos_action:
            self._zone_positions.pop(zone_uid, None)
            self.update()
        elif action == remove_action:
            topology.remove_zone(zone_uid)

    def _show_ecu_menu(self, global_pos, zone_uid: str, ecu_uid: str):
        zone = topology.zone_by_uid(zone_uid)
        if not zone:
            return
        ecu = next((e for e in zone.ecus if e.uid == ecu_uid), None)
        if not ecu:
            return

        menu = QMenu(self)
        menu.setStyleSheet(self._menu_style())

        # Informational header
        header = menu.addAction(f"{ecu.name}")
        header.setEnabled(False)
        detail1 = menu.addAction(
            f"{ecu.domain}  \u00b7  {ecu.local_bus}  \u00b7  {ecu.security}"
        )
        detail1.setEnabled(False)
        detail2 = menu.addAction(
            f"MCU: {ecu.mcu_model}  \u00b7  {ecu.asil}  \u00b7  Addr: {ecu.node_address}"
        )
        detail2.setEnabled(False)

        menu.addSeparator()
        config_action = menu.addAction("Open Zone Config\u2026")
        menu.addSeparator()
        remove_action = menu.addAction("Remove ECU")

        action = menu.exec_(global_pos)
        if action == config_action:
            self.zone_selected.emit(zone_uid)
        elif action == remove_action:
            topology.remove_ecu(zone_uid, ecu_uid)

    # ── Dialogs ────────────────────────────────────────────────────────

    def _do_add_zone(self):
        name, ok = QInputDialog.getText(
            self,
            "Add Zone Controller",
            "Zone name (e.g. 'Front Center', 'Trunk'):",
        )
        if ok and name.strip():
            topology.add_zone(name.strip())

    def _do_rename_zone(self, uid: str, current_name: str):
        name, ok = QInputDialog.getText(
            self,
            "Rename Zone Controller",
            "New name:",
            text=current_name,
        )
        if ok and name.strip():
            topology.rename_zone(uid, name.strip())

    # ── Menu styling ───────────────────────────────────────────────────

    @staticmethod
    def _menu_style() -> str:
        return f"""
            QMenu {{
                background: {Theme.color.BG_CARD};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 8px;
                padding: 6px 2px;
                font-size: {Theme.font.SIZE_BODY}px;
                color: {Theme.color.TEXT_PRIMARY};
            }}
            QMenu::item {{
                padding: 6px 20px 6px 12px;
                border-radius: 4px;
                margin: 1px 4px;
            }}
            QMenu::item:selected {{
                background: {Theme.color.BG_SIDEBAR_HOVER};
                color: {Theme.color.CYAN_DARK};
            }}
            QMenu::item:disabled {{
                color: {Theme.color.TEXT_MUTED};
            }}
            QMenu::separator {{
                height: 1px;
                background: {Theme.color.BORDER};
                margin: 4px 8px;
            }}
        """
