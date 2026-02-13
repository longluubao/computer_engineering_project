"""
Zone Controller configuration dialog — automotive-grade.

Opens when the user clicks a zone on the architecture diagram.
Full configuration of:
  • Zone hardware (Raspberry Pi model, IP address, CAN adapter)
  • Backbone Ethernet link type
  • ECU list with two-line rows:
      Line 1: domain dot · name · domain · security · remove
      Line 2: MCU model · local bus · node address · ASIL level
  • Domain-organised ECU preset library
"""

from __future__ import annotations

import copy
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QComboBox,
    QDialog,
    QFrame,
    QGraphicsDropShadowEffect,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMenu,
    QPushButton,
    QScrollArea,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from core.topology import (
    ASIL_LEVELS,
    BACKBONE_OPTIONS,
    CAN_ADAPTER_OPTIONS,
    ECU_DOMAINS,
    ECU_PRESETS,
    ECU_STATUS_OPTIONS,
    ECUConfig,
    LOCAL_BUS_OPTIONS,
    MCU_CATALOGUE,
    SECURITY_OPTIONS,
    ZONE_HARDWARE_OPTIONS,
    topology,
)


# ── Helpers ───────────────────────────────────────────────────────────

def _section_label(text: str) -> QLabel:
    lbl = QLabel(text)
    lbl.setStyleSheet(f"""
        color: {Theme.color.TEXT_SECONDARY};
        font-size: {Theme.font.SIZE_TINY}px;
        font-weight: bold;
        letter-spacing: 1px;
        border: none;
        padding-top: 8px;
    """)
    return lbl


def _field_label(text: str) -> QLabel:
    lbl = QLabel(text)
    lbl.setStyleSheet(f"""
        color: {Theme.color.TEXT_MUTED};
        font-size: {Theme.font.SIZE_SMALL}px;
        border: none;
        min-width: 90px;
    """)
    return lbl


def _sep() -> QFrame:
    f = QFrame()
    f.setFrameShape(QFrame.HLine)
    f.setStyleSheet(f"background: {Theme.color.BORDER}; border: none; max-height: 1px;")
    f.setFixedHeight(1)
    return f


def _domain_color(domain: str) -> str:
    info = ECU_DOMAINS.get(domain, {})
    attr = info.get("color_attr", "TEXT_MUTED")
    return getattr(Theme.color, attr, Theme.color.TEXT_MUTED)


def _asil_color(asil: str) -> str:
    """Return accent colour based on ASIL level."""
    mapping = {
        "ASIL-D": Theme.color.RED,
        "ASIL-C": Theme.color.ORANGE,
        "ASIL-B": Theme.color.YELLOW,
        "ASIL-A": Theme.color.GREEN,
        "QM": Theme.color.TEXT_MUTED,
    }
    return mapping.get(asil, Theme.color.TEXT_MUTED)


# ── ECU Row Widget (two-line layout) ─────────────────────────────────

class _ECURow(QFrame):
    """
    Single ECU configuration card — two-line layout.

    Line 1: ● domain dot | name input | domain combo | security combo | ✕
    Line 2:               | MCU model  | local bus    | node address   | ASIL
    """

    def __init__(self, ecu: ECUConfig, on_remove, parent=None):
        super().__init__(parent)
        self._ecu = ecu
        self._on_remove = on_remove

        self.setStyleSheet(f"""
            _ECURow {{
                background: {Theme.color.BG_ELEVATED};
                border: none;
                border-radius: 10px;
                padding: 0px;
            }}
        """)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(10, 8, 8, 8)
        outer.setSpacing(4)

        # ── Line 1: identity + security ───────────────────────────────
        row1 = QHBoxLayout()
        row1.setSpacing(6)

        # Domain colour dot
        self._dot = QLabel("●")
        self._dot.setFixedWidth(14)
        self._dot.setStyleSheet(
            f"color: {_domain_color(ecu.domain)}; "
            f"font-size: 14px; border: none;"
        )
        row1.addWidget(self._dot)

        # Name
        self._name = QLineEdit(ecu.name)
        self._name.setPlaceholderText("ECU name")
        self._name.setMinimumWidth(130)
        self._name.setStyleSheet(self._le_style())
        row1.addWidget(self._name, stretch=1)

        # Domain combo
        self._domain = QComboBox()
        self._domain.addItems(list(ECU_DOMAINS.keys()))
        self._domain.setCurrentText(ecu.domain)
        self._domain.currentTextChanged.connect(self._on_domain_changed)
        self._style_combo(self._domain, min_w=90)
        row1.addWidget(self._domain)

        # Security combo
        self._sec = QComboBox()
        self._sec.addItems(SECURITY_OPTIONS)
        self._sec.setCurrentText(ecu.security)
        self._style_combo(self._sec, min_w=110)
        row1.addWidget(self._sec)

        # ASIL badge
        self._asil_badge = QLabel(ecu.asil)
        self._asil_badge.setFixedWidth(52)
        self._asil_badge.setAlignment(Qt.AlignCenter)
        self._update_asil_badge(ecu.asil)
        row1.addWidget(self._asil_badge)

        # Remove button
        btn_rm = QPushButton("✕")
        btn_rm.setFixedSize(24, 24)
        btn_rm.setCursor(Qt.PointingHandCursor)
        btn_rm.setToolTip("Remove this ECU")
        btn_rm.setStyleSheet(f"""
            QPushButton {{
                background: transparent;
                color: {Theme.color.TEXT_MUTED};
                border: none;
                font-size: 13px;
                font-weight: bold;
                border-radius: 12px;
            }}
            QPushButton:hover {{
                background: {Theme.color.RED_LIGHT};
                color: {Theme.color.RED};
            }}
        """)
        btn_rm.clicked.connect(lambda: self._on_remove(self._ecu.uid))
        row1.addWidget(btn_rm)

        outer.addLayout(row1)

        # ── Line 2: hardware details ──────────────────────────────────
        row2 = QHBoxLayout()
        row2.setSpacing(6)
        row2.setContentsMargins(20, 0, 30, 0)  # indented past dot

        # MCU Model combo
        mcu_lbl = QLabel("MCU")
        mcu_lbl.setStyleSheet(
            f"color: {Theme.color.TEXT_MUTED}; font-size: {Theme.font.SIZE_TINY}px;"
            f"border: none; min-width: 26px;"
        )
        row2.addWidget(mcu_lbl)

        self._mcu = QComboBox()
        self._mcu.addItems(list(MCU_CATALOGUE.keys()))
        self._mcu.setCurrentText(ecu.mcu_model)
        self._mcu.currentTextChanged.connect(self._on_mcu_changed)
        self._style_combo(self._mcu, min_w=140, small=True)
        row2.addWidget(self._mcu, stretch=1)

        # Local bus combo
        bus_lbl = QLabel("Bus")
        bus_lbl.setStyleSheet(
            f"color: {Theme.color.TEXT_MUTED}; font-size: {Theme.font.SIZE_TINY}px;"
            f"border: none; min-width: 22px;"
        )
        row2.addWidget(bus_lbl)

        self._bus = QComboBox()
        self._bus.addItems(LOCAL_BUS_OPTIONS)
        self._bus.setCurrentText(ecu.local_bus)
        self._style_combo(self._bus, min_w=70, small=True)
        row2.addWidget(self._bus)

        # Node address
        addr_lbl = QLabel("Addr")
        addr_lbl.setStyleSheet(
            f"color: {Theme.color.TEXT_MUTED}; font-size: {Theme.font.SIZE_TINY}px;"
            f"border: none; min-width: 26px;"
        )
        row2.addWidget(addr_lbl)

        self._addr = QLineEdit(ecu.node_address)
        self._addr.setPlaceholderText("0x00")
        self._addr.setFixedWidth(60)
        self._addr.setStyleSheet(self._le_style(small=True, mono=True))
        row2.addWidget(self._addr)

        # ASIL combo
        asil_lbl = QLabel("ASIL")
        asil_lbl.setStyleSheet(
            f"color: {Theme.color.TEXT_MUTED}; font-size: {Theme.font.SIZE_TINY}px;"
            f"border: none; min-width: 28px;"
        )
        row2.addWidget(asil_lbl)

        self._asil = QComboBox()
        self._asil.addItems(ASIL_LEVELS)
        self._asil.setCurrentText(ecu.asil)
        self._asil.currentTextChanged.connect(self._on_asil_changed)
        self._style_combo(self._asil, min_w=68, small=True)
        row2.addWidget(self._asil)

        outer.addLayout(row2)

        # ── MCU info hint ─────────────────────────────────────────────
        self._mcu_hint = QLabel()
        self._mcu_hint.setStyleSheet(
            f"color: {Theme.color.TEXT_MUTED}; "
            f"font-size: {Theme.font.SIZE_TINY - 1}px; "
            f"border: none; padding-left: 20px;"
        )
        self._update_mcu_hint(ecu.mcu_model)
        outer.addWidget(self._mcu_hint)

    # ── Styling helpers ───────────────────────────────────────────────

    @staticmethod
    def _le_style(small: bool = False, mono: bool = False) -> str:
        sz = Theme.font.SIZE_TINY if small else Theme.font.SIZE_SMALL
        fam = Theme.font.FAMILY_MONO if mono else Theme.font.FAMILY_PRIMARY
        return f"""
            QLineEdit {{
                background: {Theme.color.BG_INPUT};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 6px;
                padding: {'3px 6px' if small else '4px 8px'};
                font-size: {sz}px;
                font-family: {fam};
            }}
            QLineEdit:focus {{ border-color: {Theme.color.CYAN}; }}
        """

    @staticmethod
    def _style_combo(cb: QComboBox, min_w: int = 80, small: bool = False):
        sz = Theme.font.SIZE_TINY if small else Theme.font.SIZE_SMALL
        cb.setStyleSheet(f"""
            QComboBox {{
                background: {Theme.color.BG_INPUT};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 6px;
                padding: {'3px 4px' if small else '4px 6px'};
                font-size: {sz}px;
                min-width: {min_w}px;
            }}
            QComboBox:hover {{ border-color: {Theme.color.CYAN}; }}
            QComboBox::drop-down {{ border: none; width: 18px; }}
            QComboBox QAbstractItemView {{
                background: {Theme.color.BG_CARD};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                selection-background-color: {Theme.color.CYAN_LIGHT};
                selection-color: {Theme.color.CYAN_DARK};
                outline: none;
            }}
        """)

    def _on_domain_changed(self, domain: str):
        self._dot.setStyleSheet(
            f"color: {_domain_color(domain)}; "
            f"font-size: 14px; border: none;"
        )

    def _on_mcu_changed(self, mcu: str):
        self._update_mcu_hint(mcu)

    def _on_asil_changed(self, asil: str):
        self._update_asil_badge(asil)

    def _update_mcu_hint(self, mcu: str):
        info = MCU_CATALOGUE.get(mcu, {})
        core = info.get("core", "")
        speed = info.get("speed_mhz", 0)
        vendor = info.get("vendor", "")
        if core and speed:
            self._mcu_hint.setText(f"{vendor}  ·  {core} @ {speed} MHz")
        elif core:
            self._mcu_hint.setText(f"{vendor}  ·  {core}")
        else:
            self._mcu_hint.setText("")

    def _update_asil_badge(self, asil: str):
        ac = _asil_color(asil)
        self._asil_badge.setText(asil)
        self._asil_badge.setStyleSheet(f"""
            color: {ac};
            font-size: {Theme.font.SIZE_TINY}px;
            font-weight: bold;
            background: transparent;
            border: 1.5px solid {ac};
            border-radius: 8px;
            padding: 2px 4px;
        """)

    def to_ecu(self) -> ECUConfig:
        """Read current values back into an ECUConfig."""
        return ECUConfig(
            uid=self._ecu.uid,
            name=self._name.text().strip() or "Unnamed ECU",
            domain=self._domain.currentText(),
            local_bus=self._bus.currentText(),
            security=self._sec.currentText(),
            mcu_model=self._mcu.currentText(),
            asil=self._asil.currentText(),
            node_address=self._addr.text().strip() or "0x00",
            status=self._ecu.status,
        )


# ── Zone Config Dialog ────────────────────────────────────────────────

class ZoneConfigDialog(QDialog):
    """
    Full configuration dialog for a Zone Controller.

    Opens when the user clicks a zone node on the architecture diagram.
    Sections:
      1. General — name, description
      2. Hardware — Raspberry Pi model, IP address, CAN adapter, backbone
      3. ECUs — scrollable list of two-line ECU config rows
    """

    def __init__(self, zone_uid: str, parent: QWidget | None = None):
        super().__init__(parent)
        self._zone_uid = zone_uid
        self._zone = topology.zone_by_uid(zone_uid)
        if not self._zone:
            self.reject()
            return

        self.setWindowTitle(f"Configure Zone: {self._zone.name}")
        self.setMinimumSize(820, 620)
        self.setModal(True)

        self._ecu_rows: list[_ECURow] = []
        self._build_ui()

    def _build_ui(self):
        self.setStyleSheet(f"""
            QDialog {{
                background: {Theme.color.BG_PAGE};
            }}
        """)

        root = QVBoxLayout(self)
        root.setContentsMargins(24, 20, 24, 20)
        root.setSpacing(0)

        # ── Title bar ──────────────────────────────────────────────────
        title_row = QHBoxLayout()
        title_row.setSpacing(12)

        title = QLabel("Configure Zone Controller")
        title.setStyleSheet(f"""
            color: {Theme.color.CYAN_DARK};
            font-size: {Theme.font.SIZE_H2}px;
            font-weight: bold;
            border: none;
        """)
        title_row.addWidget(title)
        title_row.addStretch()

        # Zone UID chip
        uid_chip = QLabel(f"UID: {self._zone.uid}")
        uid_chip.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            font-family: {Theme.font.FAMILY_MONO};
            background: {Theme.color.BG_ELEVATED};
            border: 1px solid {Theme.color.BORDER};
            border-radius: 8px;
            padding: 3px 8px;
        """)
        title_row.addWidget(uid_chip)

        root.addLayout(title_row)
        root.addSpacing(12)

        # ── General + Hardware side-by-side ────────────────────────────
        top_row = QHBoxLayout()
        top_row.setSpacing(12)

        top_row.addWidget(self._general_section(), stretch=1)
        top_row.addWidget(self._hardware_section(), stretch=1)

        root.addLayout(top_row)
        root.addSpacing(12)

        # ── ECUs section ───────────────────────────────────────────────
        root.addWidget(self._ecus_section(), stretch=1)
        root.addSpacing(16)

        # ── Buttons ────────────────────────────────────────────────────
        btn_row = QHBoxLayout()
        btn_row.addStretch()

        btn_cancel = QPushButton("Cancel")
        btn_cancel.setCursor(Qt.PointingHandCursor)
        btn_cancel.setStyleSheet(Theme.stylesheet.BTN_OUTLINE)
        btn_cancel.clicked.connect(self.reject)
        btn_row.addWidget(btn_cancel)

        btn_save = QPushButton("Save Changes")
        btn_save.setCursor(Qt.PointingHandCursor)
        btn_save.setStyleSheet(Theme.stylesheet.BTN_PRIMARY)
        btn_save.clicked.connect(self._save)
        btn_row.addWidget(btn_save)

        btn_delete = QPushButton("Delete Zone")
        btn_delete.setCursor(Qt.PointingHandCursor)
        btn_delete.setStyleSheet(Theme.stylesheet.BTN_DANGER)
        btn_delete.clicked.connect(self._delete_zone)
        btn_row.addWidget(btn_delete)

        root.addLayout(btn_row)

        # ── Populate ECU rows ──────────────────────────────────────────
        for ecu in self._zone.ecus:
            self._insert_ecu_row(copy.copy(ecu))

    # ── Section builders ───────────────────────────────────────────────

    def _general_section(self) -> QWidget:
        card = self._make_card()
        lay = QVBoxLayout(card)
        lay.setContentsMargins(16, 12, 16, 12)
        lay.setSpacing(8)

        lay.addWidget(_section_label("GENERAL"))
        lay.addWidget(_sep())

        # Zone name
        name_row = QHBoxLayout()
        name_row.addWidget(_field_label("Zone Name"))
        self._name_edit = QLineEdit(self._zone.name)
        self._name_edit.setPlaceholderText("e.g. Front Left")
        self._name_edit.setStyleSheet(self._input_style())
        name_row.addWidget(self._name_edit, stretch=1)
        lay.addLayout(name_row)

        # Description
        desc_row = QHBoxLayout()
        desc_row.addWidget(_field_label("Function"))
        self._desc_edit = QLineEdit(self._zone.description)
        self._desc_edit.setPlaceholderText("e.g. Handles headlights, ABS, front camera")
        self._desc_edit.setStyleSheet(self._input_style())
        desc_row.addWidget(self._desc_edit, stretch=1)
        lay.addLayout(desc_row)

        # Backbone
        bb_row = QHBoxLayout()
        bb_row.addWidget(_field_label("Backbone"))
        self._backbone_cb = QComboBox()
        self._backbone_cb.addItems(BACKBONE_OPTIONS)
        self._backbone_cb.setCurrentText(self._zone.backbone)
        self._backbone_cb.setStyleSheet(self._combo_style())
        bb_row.addWidget(self._backbone_cb, stretch=1)
        lay.addLayout(bb_row)

        lay.addStretch()
        return card

    def _hardware_section(self) -> QWidget:
        card = self._make_card()
        lay = QVBoxLayout(card)
        lay.setContentsMargins(16, 12, 16, 12)
        lay.setSpacing(8)

        lay.addWidget(_section_label("HARDWARE (ZONE CONTROLLER)"))
        lay.addWidget(_sep())

        # Hardware model
        hw_row = QHBoxLayout()
        hw_row.addWidget(_field_label("Board"))
        self._hw_cb = QComboBox()
        self._hw_cb.addItems(ZONE_HARDWARE_OPTIONS)
        self._hw_cb.setCurrentText(self._zone.hardware)
        self._hw_cb.setStyleSheet(self._combo_style())
        hw_row.addWidget(self._hw_cb, stretch=1)
        lay.addLayout(hw_row)

        # IP address
        ip_row = QHBoxLayout()
        ip_row.addWidget(_field_label("IP Address"))
        self._ip_edit = QLineEdit(self._zone.ip_address)
        self._ip_edit.setPlaceholderText("e.g. 192.168.1.11")
        self._ip_edit.setStyleSheet(self._input_style(mono=True))
        ip_row.addWidget(self._ip_edit, stretch=1)
        lay.addLayout(ip_row)

        # CAN adapter
        can_row = QHBoxLayout()
        can_row.addWidget(_field_label("CAN Adapter"))
        self._can_cb = QComboBox()
        self._can_cb.addItems(CAN_ADAPTER_OPTIONS)
        self._can_cb.setCurrentText(self._zone.can_adapter)
        self._can_cb.setStyleSheet(self._combo_style())
        can_row.addWidget(self._can_cb, stretch=1)
        lay.addLayout(can_row)

        lay.addStretch()
        return card

    def _ecus_section(self) -> QWidget:
        card = self._make_card()
        e_lay = QVBoxLayout(card)
        e_lay.setContentsMargins(16, 12, 16, 12)
        e_lay.setSpacing(8)

        # Header
        ecu_header = QHBoxLayout()
        self._ecu_count_label = QLabel()
        self._update_ecu_count()
        ecu_header.addWidget(_section_label("ECUs"))
        ecu_header.addWidget(self._ecu_count_label)
        ecu_header.addStretch()

        # Add ECU button
        btn_add = QPushButton("+ Add ECU")
        btn_add.setCursor(Qt.PointingHandCursor)
        btn_add.setStyleSheet(self._small_btn_style())
        btn_add.clicked.connect(self._add_blank_ecu)
        ecu_header.addWidget(btn_add)

        # Preset menu
        btn_preset = QPushButton("From Preset")
        btn_preset.setCursor(Qt.PointingHandCursor)
        btn_preset.setStyleSheet(self._small_btn_style())
        btn_preset.clicked.connect(self._show_preset_menu)
        self._btn_preset = btn_preset
        ecu_header.addWidget(btn_preset)

        e_lay.addLayout(ecu_header)
        e_lay.addWidget(_sep())

        # Scrollable ECU list
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet(Theme.stylesheet.SCROLL_AREA)
        scroll.setFrameShape(QFrame.NoFrame)

        self._ecu_container = QWidget()
        self._ecu_layout = QVBoxLayout(self._ecu_container)
        self._ecu_layout.setContentsMargins(0, 4, 0, 4)
        self._ecu_layout.setSpacing(6)
        self._ecu_layout.addStretch()

        scroll.setWidget(self._ecu_container)
        e_lay.addWidget(scroll, stretch=1)

        return card

    # ── ECU management ─────────────────────────────────────────────────

    def _insert_ecu_row(self, ecu: ECUConfig):
        row = _ECURow(ecu, self._remove_ecu_row)
        self._ecu_rows.append(row)
        idx = self._ecu_layout.count() - 1  # before the stretch
        self._ecu_layout.insertWidget(idx, row)
        self._update_ecu_count()

    def _add_blank_ecu(self):
        ecu = ECUConfig(
            name="New ECU", domain="Body", local_bus="CAN",
            security="PQC (ML-DSA-65)", mcu_model="Generic / Simulated",
            asil="QM", node_address="0x00",
        )
        self._insert_ecu_row(ecu)

    def _remove_ecu_row(self, ecu_uid: str):
        for row in self._ecu_rows:
            if row._ecu.uid == ecu_uid:
                self._ecu_rows.remove(row)
                self._ecu_layout.removeWidget(row)
                row.deleteLater()
                self._update_ecu_count()
                return

    def _show_preset_menu(self):
        menu = QMenu(self)
        menu.setStyleSheet(self._menu_style())

        for domain, presets in ECU_PRESETS.items():
            domain_menu = menu.addMenu(f"{domain}")
            for preset_data in presets:
                name, bus, sec, mcu, asil = preset_data
                action = domain_menu.addAction(f"{name}  ({mcu})")
                action.setData((name, domain, bus, sec, mcu, asil))

        action = menu.exec_(self._btn_preset.mapToGlobal(
            self._btn_preset.rect().bottomLeft()
        ))
        if action and action.data():
            name, domain, bus, sec, mcu, asil = action.data()
            ecu = ECUConfig(
                name=name, domain=domain, local_bus=bus,
                security=sec, mcu_model=mcu, asil=asil,
            )
            self._insert_ecu_row(ecu)

    def _update_ecu_count(self):
        n = len(self._ecu_rows)
        self._ecu_count_label.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            background: {Theme.color.BG_ELEVATED};
            border: 1px solid {Theme.color.BORDER};
            border-radius: 8px;
            padding: 2px 8px;
        """)
        self._ecu_count_label.setText(f"{n}")

    # ── Save / Delete ──────────────────────────────────────────────────

    def _save(self):
        """Commit changes to the topology model."""
        topology.update_zone(
            self._zone_uid,
            name=self._name_edit.text().strip() or self._zone.name,
            description=self._desc_edit.text().strip(),
            backbone=self._backbone_cb.currentText(),
            hardware=self._hw_cb.currentText(),
            ip_address=self._ip_edit.text().strip(),
            can_adapter=self._can_cb.currentText(),
        )
        ecus = [row.to_ecu() for row in self._ecu_rows]
        topology.set_zone_ecus(self._zone_uid, ecus)
        self.accept()

    def _delete_zone(self):
        topology.remove_zone(self._zone_uid)
        self.accept()

    # ── Styling helpers ────────────────────────────────────────────────

    @staticmethod
    def _make_card() -> QFrame:
        card = QFrame()
        card.setStyleSheet(f"""
            QFrame {{
                background: {Theme.color.BG_CARD};
                border: none;
                border-radius: {Theme.radius.CARD}px;
            }}
        """)
        shadow = QGraphicsDropShadowEffect(card)
        shadow.setBlurRadius(18)
        shadow.setOffset(0, 3)
        shadow.setColor(QColor(0, 0, 0, 12))
        card.setGraphicsEffect(shadow)
        return card

    @staticmethod
    def _input_style(mono: bool = False) -> str:
        fam = Theme.font.FAMILY_MONO if mono else Theme.font.FAMILY_PRIMARY
        return f"""
            QLineEdit {{
                background: {Theme.color.BG_INPUT};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 8px;
                padding: 6px 10px;
                font-size: {Theme.font.SIZE_BODY}px;
                font-family: {fam};
            }}
            QLineEdit:focus {{ border-color: {Theme.color.CYAN}; }}
        """

    @staticmethod
    def _combo_style() -> str:
        return f"""
            QComboBox {{
                background: {Theme.color.BG_INPUT};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 8px;
                padding: 6px 10px;
                font-size: {Theme.font.SIZE_BODY}px;
            }}
            QComboBox:hover {{ border-color: {Theme.color.CYAN}; }}
            QComboBox::drop-down {{ border: none; width: 24px; }}
            QComboBox QAbstractItemView {{
                background: {Theme.color.BG_CARD};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                selection-background-color: {Theme.color.CYAN_LIGHT};
                selection-color: {Theme.color.CYAN_DARK};
                outline: none;
            }}
        """

    @staticmethod
    def _small_btn_style() -> str:
        return f"""
            QPushButton {{
                background: {Theme.color.BG_ELEVATED};
                color: {Theme.color.TEXT_SECONDARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 8px;
                padding: 4px 12px;
                font-size: {Theme.font.SIZE_SMALL}px;
                font-weight: 600;
            }}
            QPushButton:hover {{
                background: {Theme.color.BG_SIDEBAR_HOVER};
                color: {Theme.color.CYAN_DARK};
                border-color: {Theme.color.CYAN};
            }}
        """

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
            QMenu::separator {{
                height: 1px;
                background: {Theme.color.BORDER};
                margin: 4px 8px;
            }}
        """
