"""
Hardware configuration dialogs for RPi peripherals.

Provides dialogs for CAN bus, GPIO, and network configuration.
All dialogs use the central Theme and integrate with HardwareConfigurator.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QButtonGroup,
    QDialog,
    QDialogButtonBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QRadioButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from core.hw_configurator import hw_configurator
from widgets.common import IconButton, StatusBadge


class CANConfigDialog(QDialog):
    """Dialog for configuring CAN interface (bitrate, bring up/down)."""

    BITRATES = [
        ("125 kbps", 125000),
        ("250 kbps", 250000),
        ("500 kbps", 500000),
        ("1 Mbps", 1000000),
    ]

    def __init__(self, parent: QWidget | None = None, iface: str = "can0"):
        super().__init__(parent)
        self._iface = iface
        self._bitrate = 500000
        self._refresh_timer = QTimer(self)
        self._refresh_timer.timeout.connect(self._refresh_status)
        self._setup_ui()

    def _setup_ui(self):
        self.setWindowTitle(f"CAN Configuration - {self._iface}")
        self.setMinimumWidth(420)
        self.setStyleSheet(f"""
            QDialog {{
                background: {Theme.color.BG_CARD};
            }}
            QLabel {{
                color: {Theme.color.TEXT_PRIMARY};
                font-size: {Theme.font.SIZE_BODY}px;
                border: none;
            }}
        """)

        lay = QVBoxLayout(self)
        lay.setSpacing(16)
        lay.setContentsMargins(24, 20, 24, 20)

        # Status section
        status_row = QHBoxLayout()
        status_row.setSpacing(8)
        lbl = QLabel("Interface Status:")
        lbl.setStyleSheet(f"font-weight: 600;")
        status_row.addWidget(lbl)
        self._status_badge = StatusBadge("Checking...", "idle")
        status_row.addWidget(self._status_badge)
        status_row.addStretch()
        lay.addLayout(status_row)

        # Bitrate selection
        bitrate_section = QVBoxLayout()
        bitrate_section.setSpacing(8)
        bitrate_lbl = QLabel("Bitrate:")
        bitrate_lbl.setStyleSheet(f"font-weight: 600;")
        bitrate_section.addWidget(bitrate_lbl)

        bitrate_row = QHBoxLayout()
        bitrate_row.setSpacing(12)
        self._bitrate_group = QButtonGroup(self)

        for text, value in self.BITRATES:
            rb = QRadioButton(text)
            rb.setStyleSheet(f"""
                QRadioButton {{
                    color: {Theme.color.TEXT_PRIMARY};
                    font-size: {Theme.font.SIZE_BODY}px;
                    spacing: 6px;
                }}
                QRadioButton::indicator {{
                    width: 16px;
                    height: 16px;
                }}
                QRadioButton::indicator:checked {{
                    background: {Theme.color.CYAN};
                    border-radius: 8px;
                    border: none;
                }}
                QRadioButton::indicator:unchecked {{
                    background: {Theme.color.BG_ELEVATED};
                    border: 2px solid {Theme.color.BORDER};
                    border-radius: 8px;
                }}
            """)
            rb.clicked.connect(lambda checked, v=value: self._on_bitrate_changed(v))
            if value == 500000:
                rb.setChecked(True)
            self._bitrate_group.addButton(rb)
            bitrate_row.addWidget(rb)

        bitrate_row.addStretch()
        bitrate_section.addLayout(bitrate_row)
        lay.addLayout(bitrate_section)

        # MCP2515 Settings (for SPI CAN hat)
        mcp_section = QVBoxLayout()
        mcp_section.setSpacing(8)
        mcp_lbl = QLabel("MCP2515 Settings (SPI CAN HAT):")
        mcp_lbl.setStyleSheet(f"font-weight: 600;")
        mcp_section.addWidget(mcp_lbl)

        mcp_grid = QGridLayout()
        mcp_grid.setSpacing(10)

        osc_lbl = QLabel("Oscillator (Hz):")
        self._osc_spin = QSpinBox()
        self._osc_spin.setRange(1000000, 32000000)
        self._osc_spin.setValue(16000000)
        self._osc_spin.setSingleStep(1000000)
        self._osc_spin.setStyleSheet(Theme.stylesheet.INPUT)
        mcp_grid.addWidget(osc_lbl, 0, 0)
        mcp_grid.addWidget(self._osc_spin, 0, 1)

        int_lbl = QLabel("Interrupt Pin (BCM):")
        self._int_spin = QSpinBox()
        self._int_spin.setRange(0, 27)
        self._int_spin.setValue(25)
        self._int_spin.setStyleSheet(Theme.stylesheet.INPUT)
        mcp_grid.addWidget(int_lbl, 1, 0)
        mcp_grid.addWidget(self._int_spin, 1, 1)

        mcp_section.addLayout(mcp_grid)

        load_btn = IconButton("Load MCP2515 Overlay", style="outline")
        load_btn.clicked.connect(self._load_mcp2515)
        mcp_section.addWidget(load_btn)

        lay.addLayout(mcp_section)

        # Action buttons
        action_row = QHBoxLayout()
        action_row.setSpacing(10)

        self._up_btn = IconButton("Bring UP", icon_text="\u25b2", style="primary")
        self._up_btn.clicked.connect(self._bring_up)

        self._down_btn = IconButton("Bring DOWN", icon_text="\u25bc", style="danger")
        self._down_btn.clicked.connect(self._bring_down)

        action_row.addWidget(self._up_btn)
        action_row.addWidget(self._down_btn)
        action_row.addStretch()
        lay.addLayout(action_row)

        # Dialog buttons
        btn_box = QDialogButtonBox(QDialogButtonBox.Close)
        btn_box.setStyleSheet(Theme.stylesheet.BTN_OUTLINE)
        btn_box.rejected.connect(self.reject)
        lay.addWidget(btn_box)

        # Initial status refresh
        self._refresh_status()
        self._refresh_timer.start(2000)

    def _on_bitrate_changed(self, value: int):
        self._bitrate = value

    def _refresh_status(self):
        status = hw_configurator.can_status(self._iface)
        state = status.get("state", "unknown")
        bitrate = status.get("bitrate", 0)

        if state == "up":
            self._status_badge.set_preset("ok", f"UP @ {bitrate // 1000}kbps")
        elif state == "down":
            self._status_badge.set_preset("warning", "DOWN")
        else:
            self._status_badge.set_preset("idle", state.upper())

    def _bring_up(self):
        hw_configurator.can_up(self._iface, self._bitrate)
        QTimer.singleShot(500, self._refresh_status)

    def _bring_down(self):
        hw_configurator.can_down(self._iface)
        QTimer.singleShot(500, self._refresh_status)

    def _load_mcp2515(self):
        osc = self._osc_spin.value()
        int_pin = self._int_spin.value()
        hw_configurator.load_mcp2515_overlay(osc, int_pin)

    def closeEvent(self, event):
        self._refresh_timer.stop()
        super().closeEvent(event)


class GPIOControlDialog(QDialog):
    """Dialog for controlling individual GPIO pins."""

    def __init__(
        self,
        parent: QWidget | None = None,
        bcm: int = 0,
        pin_name: str = "",
    ):
        super().__init__(parent)
        self._bcm = bcm
        self._pin_name = pin_name
        self._direction = "in"
        self._refresh_timer = QTimer(self)
        self._refresh_timer.timeout.connect(self._refresh_value)
        self._setup_ui()

    def _setup_ui(self):
        title = f"GPIO Control - {self._pin_name} (BCM{self._bcm})"
        self.setWindowTitle(title)
        self.setMinimumWidth(380)
        self.setStyleSheet(f"""
            QDialog {{
                background: {Theme.color.BG_CARD};
            }}
            QLabel {{
                color: {Theme.color.TEXT_PRIMARY};
                font-size: {Theme.font.SIZE_BODY}px;
                border: none;
            }}
        """)

        lay = QVBoxLayout(self)
        lay.setSpacing(16)
        lay.setContentsMargins(24, 20, 24, 20)

        # Pin info
        info_row = QHBoxLayout()
        info_lbl = QLabel(f"BCM Pin: {self._bcm}")
        info_lbl.setStyleSheet(f"font-weight: 600; font-size: {Theme.font.SIZE_H2}px;")
        info_row.addWidget(info_lbl)

        self._export_badge = StatusBadge("Not Exported", "idle")
        info_row.addWidget(self._export_badge)
        info_row.addStretch()
        lay.addLayout(info_row)

        # Direction
        dir_section = QVBoxLayout()
        dir_section.setSpacing(8)
        dir_lbl = QLabel("Direction:")
        dir_lbl.setStyleSheet(f"font-weight: 600;")
        dir_section.addWidget(dir_lbl)

        dir_row = QHBoxLayout()
        dir_row.setSpacing(12)

        self._in_btn = IconButton("INPUT", style="outline")
        self._in_btn.clicked.connect(lambda: self._set_direction("in"))

        self._out_btn = IconButton("OUTPUT", style="outline")
        self._out_btn.clicked.connect(lambda: self._set_direction("out"))

        dir_row.addWidget(self._in_btn)
        dir_row.addWidget(self._out_btn)
        dir_row.addStretch()
        dir_section.addLayout(dir_row)
        lay.addLayout(dir_section)

        # Value display / control
        value_section = QVBoxLayout()
        value_section.setSpacing(8)
        value_lbl = QLabel("Value:")
        value_lbl.setStyleSheet(f"font-weight: 600;")
        value_section.addWidget(value_lbl)

        value_row = QHBoxLayout()
        value_row.setSpacing(8)

        self._value_badge = StatusBadge("0 (LOW)", "idle")
        value_row.addWidget(self._value_badge)

        self._high_btn = IconButton("Set HIGH", icon_text="\u25b2", style="primary")
        self._high_btn.clicked.connect(lambda: self._write_value(1))

        self._low_btn = IconButton("Set LOW", icon_text="\u25bc", style="danger")
        self._low_btn.clicked.connect(lambda: self._write_value(0))

        self._refresh_btn = IconButton("Refresh", icon_text="\u21bb", style="outline")
        self._refresh_btn.clicked.connect(self._refresh_value)

        value_row.addWidget(self._high_btn)
        value_row.addWidget(self._low_btn)
        value_row.addWidget(self._refresh_btn)
        value_row.addStretch()
        value_section.addLayout(value_row)
        lay.addLayout(value_section)

        # Export/Unexport buttons
        export_row = QHBoxLayout()
        export_row.setSpacing(10)

        self._export_btn = IconButton("Export Pin", style="primary")
        self._export_btn.clicked.connect(self._export)

        self._unexport_btn = IconButton("Unexport Pin", style="outline")
        self._unexport_btn.clicked.connect(self._unexport)

        export_row.addWidget(self._export_btn)
        export_row.addWidget(self._unexport_btn)
        export_row.addStretch()
        lay.addLayout(export_row)

        # Dialog buttons
        btn_box = QDialogButtonBox(QDialogButtonBox.Close)
        btn_box.setStyleSheet(Theme.stylesheet.BTN_OUTLINE)
        btn_box.rejected.connect(self.reject)
        lay.addWidget(btn_box)

        # Initial state
        self._load_initial_state()
        self._refresh_value()
        self._refresh_timer.start(1000)

    def _load_initial_state(self):
        exported = hw_configurator.gpio_is_exported(self._bcm)
        direction = hw_configurator.gpio_get_direction(self._bcm)
        if direction not in {"in", "out"}:
            direction = "in"
        self._direction = direction
        self._update_ui_for_direction(direction)
        self._export_badge.set_preset("ok" if exported else "idle", "Exported" if exported else "Not Exported")

    def _set_direction(self, direction: str):
        self._direction = direction
        hw_configurator.gpio_set_direction(self._bcm, direction)
        self._update_ui_for_direction(direction)

    def _update_ui_for_direction(self, direction: str):
        is_in = direction == "in"
        self._in_btn.setStyleSheet(
            Theme.stylesheet.BTN_PRIMARY if is_in else Theme.stylesheet.BTN_OUTLINE
        )
        self._out_btn.setStyleSheet(
            Theme.stylesheet.BTN_PRIMARY if not is_in else Theme.stylesheet.BTN_OUTLINE
        )
        self._high_btn.setEnabled(not is_in)
        self._low_btn.setEnabled(not is_in)

    def _write_value(self, value: int):
        hw_configurator.gpio_write(self._bcm, value)
        QTimer.singleShot(100, self._refresh_value)

    def _refresh_value(self):
        val = hw_configurator.gpio_read(self._bcm)
        text = f"{val} ({'HIGH' if val else 'LOW'})"
        preset = "ok" if val else "idle"
        self._value_badge.set_preset(preset, text)
        self._export_badge.set_preset(
            "ok" if hw_configurator.gpio_is_exported(self._bcm) else "idle",
            "Exported" if hw_configurator.gpio_is_exported(self._bcm) else "Not Exported",
        )

    def _export(self):
        hw_configurator.gpio_export(self._bcm)
        self._export_badge.set_preset("ok", "Exported")

    def _unexport(self):
        hw_configurator.gpio_unexport(self._bcm)
        self._export_badge.set_preset("idle", "Not Exported")

    def closeEvent(self, event):
        self._refresh_timer.stop()
        super().closeEvent(event)


class IPConfigDialog(QDialog):
    """Dialog for configuring network interface IP settings."""

    def __init__(
        self,
        parent: QWidget | None = None,
        iface: str = "eth0",
    ):
        super().__init__(parent)
        self._iface = iface
        self._setup_ui()

    def _setup_ui(self):
        self.setWindowTitle(f"Network Configuration - {self._iface}")
        self.setMinimumWidth(400)
        self.setStyleSheet(f"""
            QDialog {{
                background: {Theme.color.BG_CARD};
            }}
            QLabel {{
                color: {Theme.color.TEXT_PRIMARY};
                font-size: {Theme.font.SIZE_BODY}px;
                border: none;
            }}
        """)

        lay = QVBoxLayout(self)
        lay.setSpacing(16)
        lay.setContentsMargins(24, 20, 24, 20)

        # Current status
        status_row = QHBoxLayout()
        status_row.setSpacing(8)
        lbl = QLabel("Current IP:")
        lbl.setStyleSheet(f"font-weight: 600;")
        status_row.addWidget(lbl)
        self._ip_badge = StatusBadge("Checking...", "idle")
        status_row.addWidget(self._ip_badge)
        status_row.addStretch()
        lay.addLayout(status_row)

        # IP configuration
        config_section = QVBoxLayout()
        config_section.setSpacing(10)

        # IP Address
        ip_row = QHBoxLayout()
        ip_lbl = QLabel("IP Address:")
        ip_lbl.setFixedWidth(100)
        self._ip_edit = QLineEdit()
        self._ip_edit.setStyleSheet(Theme.stylesheet.INPUT)
        self._ip_edit.setPlaceholderText("e.g. 192.168.1.100")
        ip_row.addWidget(ip_lbl)
        ip_row.addWidget(self._ip_edit)
        config_section.addLayout(ip_row)

        # Netmask/CIDR
        mask_row = QHBoxLayout()
        mask_lbl = QLabel("CIDR Prefix:")
        mask_lbl.setFixedWidth(100)
        self._mask_edit = QLineEdit()
        self._mask_edit.setStyleSheet(Theme.stylesheet.INPUT)
        self._mask_edit.setPlaceholderText("e.g. 24")
        self._mask_edit.setText("24")
        self._mask_edit.setMaximumWidth(80)
        mask_row.addWidget(mask_lbl)
        mask_row.addWidget(self._mask_edit)
        mask_row.addStretch()
        config_section.addLayout(mask_row)

        lay.addLayout(config_section)

        # Note about persistence
        note = QLabel(
            "Note: Changes are temporary. For persistent config, edit /etc/dhcpcd.conf"
        )
        note.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            font-style: italic;
            padding: 8px 0;
        """)
        note.setWordWrap(True)
        lay.addWidget(note)

        # Action buttons
        action_row = QHBoxLayout()
        action_row.setSpacing(10)

        apply_btn = IconButton("Apply Static IP", icon_text="\u2713", style="primary")
        apply_btn.clicked.connect(self._apply)

        refresh_btn = IconButton("Refresh", icon_text="\u21bb", style="outline")
        refresh_btn.clicked.connect(self._refresh_status)

        action_row.addWidget(apply_btn)
        action_row.addWidget(refresh_btn)
        action_row.addStretch()
        lay.addLayout(action_row)

        # Dialog buttons
        btn_box = QDialogButtonBox(QDialogButtonBox.Close)
        btn_box.setStyleSheet(Theme.stylesheet.BTN_OUTLINE)
        btn_box.rejected.connect(self.reject)
        lay.addWidget(btn_box)

        # Initial status
        self._refresh_status()

    def _refresh_status(self):
        info = hw_configurator.network_info(self._iface)
        ip = info.get("ip", "N/A")
        state = info.get("state", "unknown")

        if ip != "N/A":
            self._ip_badge.set_preset("ok", ip)
            self._ip_edit.setText(ip)
        else:
            self._ip_badge.set_preset("warning", "No IP")

    def _apply(self):
        ip = self._ip_edit.text().strip()
        mask = self._mask_edit.text().strip() or "24"

        if not ip:
            return

        hw_configurator.set_static_ip(self._iface, ip, mask)
        from PySide6.QtCore import QTimer
        QTimer.singleShot(500, self._refresh_status)
