"""
Hex viewer / editor widget for PDU byte inspection.

Displays data in classic hex-dump format with colour-coded sections
for Header, Payload, Freshness, and Authenticator.  Light theme variant.
"""

from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QFont, QColor
from PySide6.QtWidgets import (
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QPlainTextEdit,
    QWidget,
)

from config.theme import Theme


class HexViewer(QWidget):
    """
    Displays a hex dump with optional colour regions.

    Parameters
    ----------
    title : str
        Header text above the hex view.
    """

    # Colour map for each PDU section
    SECTION_COLORS = {
        "header":        Theme.color.YELLOW_DARK,
        "payload":       Theme.color.TEXT_PRIMARY,
        "freshness":     Theme.color.CYAN_DARK,
        "authenticator": Theme.color.GREEN_DARK,
    }

    def __init__(self, title: str = "Hex View", parent: QWidget | None = None):
        super().__init__(parent)
        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(4)

        # Title row with byte-count badge
        row = QHBoxLayout()
        self._title = QLabel(title)
        self._title.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
        """)
        self._badge = QLabel("0 bytes")
        self._badge.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            background: {Theme.color.BG_ELEVATED};
            border-radius: 8px;
            padding: 2px 8px;
        """)
        row.addWidget(self._title)
        row.addStretch()
        row.addWidget(self._badge)
        lay.addLayout(row)

        # Hex text area – light editor style
        self._text = QPlainTextEdit()
        self._text.setReadOnly(True)
        self._text.setFont(QFont(Theme.font.FAMILY_MONO, Theme.font.SIZE_BODY))
        self._text.setStyleSheet(f"""
            QPlainTextEdit {{
                background: {Theme.color.BG_ELEVATED};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: {Theme.radius.INPUT}px;
                padding: 8px;
                selection-background-color: rgba(0, 152, 206, 0.25);
            }}
        """)
        self._text.setMaximumHeight(160)
        lay.addWidget(self._text)

        # Legend row
        legend = QHBoxLayout()
        legend.setSpacing(12)
        for section, color in self.SECTION_COLORS.items():
            dot = QLabel("\u25cf")
            dot.setStyleSheet(f"color: {color}; font-size: 10px; border: none;")
            lbl = QLabel(section.capitalize())
            lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px;
                border: none;
            """)
            legend.addWidget(dot)
            legend.addWidget(lbl)
        legend.addStretch()
        lay.addLayout(legend)

    # ── Public API ───────────────────────────────────────────────────

    def set_data(self, hex_string: str, byte_count: int = 0) -> None:
        """Set the displayed hex string."""
        self._text.setPlainText(hex_string)
        self._badge.setText(f"{byte_count} bytes" if byte_count else "")

    def set_raw_bytes(self, data: bytes) -> None:
        """Format raw bytes into a hex dump."""
        lines = []
        for offset in range(0, len(data), 16):
            chunk = data[offset : offset + 16]
            hex_part = " ".join(f"{b:02X}" for b in chunk)
            ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
            lines.append(f"{offset:04X}  {hex_part:<48s}  {ascii_part}")
        self._text.setPlainText("\n".join(lines))
        self._badge.setText(f"{len(data)} bytes")

    def clear(self) -> None:
        self._text.clear()
        self._badge.setText("0 bytes")
