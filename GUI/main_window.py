"""
MainWindow — the application shell.

AUTOSAR Ethernet Gateway Configurator navigation:
  ARCHITECTURE  → Zones (Zonal E/E topology)
  COMMUNICATION → Routing (PDU/Signal gateway)
  SECURITY      → SecOC (Secure onboard communication)
  DIAGNOSTICS   → Monitor (Network health & status)

Enhanced gateway card showing Raspberry Pi 4 hardware details.
"""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
    QGraphicsDropShadowEffect,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.signal_hub import hub
from core.app_state import state

from pages.dashboard import DashboardPage
from pages.routing import RoutingPage
from pages.secoc_config import SecOCConfigPage
from pages.diagnostics import DiagnosticsPage
from pages.hardware_config import HardwareConfigPage


# ── Sidebar navigation — AUTOSAR Gateway structure ──────────────────────

_SECTIONS: list[tuple[str, list[tuple[str, int]]]] = [
    ("ARCHITECTURE", [
        ("Zones", 0),
        ("Hardware", 4),
    ]),
    ("COMMUNICATION", [
        ("Routing", 1),
    ]),
    ("SECURITY", [
        ("SecOC", 2),
    ]),
    ("DIAGNOSTICS", [
        ("Monitor", 3),
    ]),
]


class MainWindow(QMainWindow):
    """Top-level window: light sidebar + light stacked pages."""

    def __init__(self, bridge: BackendBridge):
        super().__init__()
        self._bridge = bridge
        self.setWindowTitle(Settings.APP_NAME)
        self.setMinimumSize(Settings.WINDOW_MIN_W, Settings.WINDOW_MIN_H)

        self.setStyleSheet(f"""
            QMainWindow {{
                background: {Theme.color.BG_PAGE};
            }}
        """)

        self._build_ui()
        self._wire_state()

    # ── UI construction ────────────────────────────────────────────────

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QHBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        root.addWidget(self._build_sidebar())

        self._stack = QStackedWidget()
        self._stack.setStyleSheet(f"background: {Theme.color.BG_PAGE};")

        # Page order must match indices in _SECTIONS
        self._pages = [
            DashboardPage(),                # 0 - Architecture > Zones
            RoutingPage(self._bridge),      # 1 - Communication > Routing
            SecOCConfigPage(self._bridge),  # 2 - Security > SecOC
            DiagnosticsPage(self._bridge),  # 3 - Diagnostics > Monitor
            HardwareConfigPage(self._bridge),  # 4 - Architecture > Hardware
        ]
        for page in self._pages:
            self._stack.addWidget(page)

        root.addWidget(self._stack, stretch=1)
        self._select_page(0)

    # ── State wiring ───────────────────────────────────────────────────

    def _wire_state(self):
        state.init_changed.connect(self._update_gateway_card)
        state.mode_changed.connect(self._update_mode)

        if self._bridge.is_loaded:
            state.set_backend_ready(True)

    def _update_gateway_card(self, ready: bool):
        if ready:
            self._gw_dot.setStyleSheet(
                f"background:{Theme.color.GREEN}; border-radius:4px; border:none;"
            )
            self._gw_status.setText("Connected")
            self._gw_status.setStyleSheet(
                f"color:{Theme.color.GREEN}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none;"
            )
            self._gw_target.setText("Backend: SecOCLibShared")
        else:
            self._gw_dot.setStyleSheet(
                f"background:{Theme.color.TEXT_MUTED}; border-radius:4px; border:none;"
            )
            self._gw_status.setText("Simulation")
            self._gw_status.setStyleSheet(
                f"color:{Theme.color.TEXT_MUTED}; font-size:{Theme.font.SIZE_TINY}px;"
                f"font-weight:bold; border:none;"
            )
            self._gw_target.setText("No backend loaded")

    def _update_mode(self, mode: str):
        self._gw_mode.setText(f"Mode: {mode}")

    # ── Sidebar ────────────────────────────────────────────────────────

    def _build_sidebar(self) -> QWidget:
        sidebar = QFrame()
        sidebar.setFixedWidth(230)
        sidebar.setStyleSheet(f"""
            QFrame {{
                background: {Theme.color.BG_SIDEBAR};
                border-right: 1px solid {Theme.color.BORDER};
            }}
        """)

        # Subtle shadow for depth separation
        shadow = QGraphicsDropShadowEffect(sidebar)
        shadow.setBlurRadius(12)
        shadow.setOffset(1, 0)
        shadow.setColor(QColor(0, 0, 0, 12))
        sidebar.setGraphicsEffect(shadow)

        lay = QVBoxLayout(sidebar)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)

        lay.addWidget(self._brand_header())
        lay.addWidget(self._gateway_card())

        # ── Sectioned Navigation ───────────────────────────────────────
        nav = QWidget()
        nav.setStyleSheet("background: transparent; border: none;")
        nav_lay = QVBoxLayout(nav)
        nav_lay.setContentsMargins(10, 8, 10, 10)
        nav_lay.setSpacing(2)

        self._nav_buttons: list[tuple[QPushButton, int]] = []

        for section_label, items in _SECTIONS:
            sec = QLabel(section_label)
            sec.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px;
                font-weight: bold;
                letter-spacing: 1.5px;
                padding: 14px 6px 4px 6px;
                border: none;
                background: transparent;
            """)
            nav_lay.addWidget(sec)

            for label, page_idx in items:
                btn = QPushButton(label)
                btn.setCursor(Qt.PointingHandCursor)
                btn.setFixedHeight(38)
                btn.setStyleSheet(self._nav_style(False))
                btn.clicked.connect(
                    lambda checked, i=page_idx: self._select_page(i)
                )
                nav_lay.addWidget(btn)
                self._nav_buttons.append((btn, page_idx))

        nav_lay.addStretch()
        lay.addWidget(nav, stretch=1)

        lay.addWidget(self._footer())
        return sidebar

    # ── Brand header ───────────────────────────────────────────────────

    def _brand_header(self) -> QWidget:
        w = QWidget()
        w.setFixedHeight(88)
        w.setStyleSheet(f"""
            background: {Theme.color.BG_SIDEBAR};
            border-bottom: 1px solid {Theme.color.BORDER};
        """)

        lay = QVBoxLayout(w)
        lay.setContentsMargins(18, 14, 18, 10)
        lay.setSpacing(1)

        title = QLabel("AUTOSAR Gateway")
        title.setStyleSheet(f"""
            color: {Theme.color.CYAN_DARK};
            font-size: {Theme.font.SIZE_H2}px;
            font-weight: bold;
            border: none;
            background: transparent;
        """)

        subtitle = QLabel("Ethernet Gateway Configurator")
        subtitle.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;
            border: none;
            background: transparent;
        """)

        thesis_tag = QLabel(
            f"B.Sc. Thesis \u00b7 {Settings.UNIVERSITY_SHORT} {Settings.THESIS_YEAR}"
        )
        thesis_tag.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY - 1}px;
            border: none;
            background: transparent;
        """)

        lay.addWidget(title)
        lay.addWidget(subtitle)
        lay.addWidget(thesis_tag)
        return w

    # ── Gateway connection card ────────────────────────────────────────

    def _gateway_card(self) -> QWidget:
        wrapper = QWidget()
        wrapper.setStyleSheet("background: transparent; border: none;")
        wrapper_lay = QVBoxLayout(wrapper)
        wrapper_lay.setContentsMargins(12, 10, 12, 4)

        card = QFrame()
        card.setStyleSheet(f"""
            QFrame {{
                background: {Theme.color.BG_ELEVATED};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 10px;
            }}
        """)

        card_lay = QVBoxLayout(card)
        card_lay.setContentsMargins(12, 10, 12, 10)
        card_lay.setSpacing(3)

        # Row 1: status dot + label + mode
        row1 = QHBoxLayout()
        row1.setSpacing(6)

        self._gw_dot = QLabel()
        self._gw_dot.setFixedSize(8, 8)
        self._gw_dot.setStyleSheet(
            f"background:{Theme.color.TEXT_MUTED};"
            f"border-radius:4px; border:none;"
        )
        row1.addWidget(self._gw_dot)

        self._gw_status = QLabel("Simulation")
        self._gw_status.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            font-weight: bold;
            border: none;
        """)
        row1.addWidget(self._gw_status)
        row1.addStretch()

        self._gw_mode = QLabel("Mode: PQC")
        self._gw_mode.setStyleSheet(f"""
            color: {Theme.color.PQC_PURPLE};
            font-size: {Theme.font.SIZE_TINY}px;
            font-weight: bold;
            border: none;
        """)
        row1.addWidget(self._gw_mode)
        card_lay.addLayout(row1)

        # Row 2: target info
        self._gw_target = QLabel("No backend loaded")
        self._gw_target.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY - 1}px;
            border: none;
        """)
        card_lay.addWidget(self._gw_target)

        # Row 3: Hardware info
        hw_row = QHBoxLayout()
        hw_row.setSpacing(6)

        hw_label = QLabel("Raspberry Pi 4 Model B")
        hw_label.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_TINY - 1}px;
            font-weight: 600;
            border: none;
        """)
        hw_row.addWidget(hw_label)
        hw_row.addStretch()

        role_label = QLabel("HPCU")
        role_label.setToolTip("High Performance Compute Unit - Central Zone Gateway")
        role_label.setStyleSheet(f"""
            color: {Theme.color.CYAN_DARK};
            font-size: {Theme.font.SIZE_TINY - 1}px;
            font-weight: bold;
            background: {Theme.color.CYAN_LIGHT};
            border-radius: 6px;
            padding: 1px 6px;
            border: none;
        """)
        hw_row.addWidget(role_label)

        card_lay.addLayout(hw_row)

        # Row 4: ARM Cortex detail
        arm_label = QLabel("ARM Cortex-A72 \u00b7 1.5 GHz \u00b7 Ethernet Backbone")
        arm_label.setStyleSheet(f"""
            color: {Theme.color.BORDER};
            font-size: {Theme.font.SIZE_TINY - 1}px;
            font-style: italic;
            border: none;
        """)
        card_lay.addWidget(arm_label)

        wrapper_lay.addWidget(card)
        return wrapper

    # ── Footer ─────────────────────────────────────────────────────────

    def _footer(self) -> QWidget:
        footer = QLabel(
            f"{Settings.UNIVERSITY_SHORT}  \u00b7  v{Settings.APP_VERSION}"
        )
        footer.setAlignment(Qt.AlignCenter)
        footer.setToolTip(Settings.UNIVERSITY)
        footer.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            padding: 12px;
            border-top: 1px solid {Theme.color.BORDER};
            background: transparent;
        """)
        return footer

    # ── Navigation logic ───────────────────────────────────────────────

    def _select_page(self, index: int):
        self._stack.setCurrentIndex(index)
        for btn, page_idx in self._nav_buttons:
            btn.setStyleSheet(self._nav_style(page_idx == index))

    @staticmethod
    def _nav_style(active: bool) -> str:
        """Pill-style nav button - light background, clean typography."""
        C = Theme.color
        if active:
            return f"""
                QPushButton {{
                    background: rgba(0, 152, 206, 0.10);
                    color: {C.CYAN_DARK};
                    border: none;
                    border-radius: 8px;
                    text-align: left;
                    padding-left: 14px;
                    font-size: {Theme.font.SIZE_BODY}px;
                    font-weight: bold;
                }}
            """
        return f"""
            QPushButton {{
                background: transparent;
                color: {C.TEXT_SIDEBAR};
                border: none;
                border-radius: 8px;
                text-align: left;
                padding-left: 14px;
                font-size: {Theme.font.SIZE_BODY}px;
            }}
            QPushButton:hover {{
                background: {C.BG_SIDEBAR_HOVER};
                color: {C.TEXT_PRIMARY};
            }}
        """
