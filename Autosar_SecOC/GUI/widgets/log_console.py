"""
Scrollable log / console widget with timestamp and colour coding.

Light-theme variant – clean white/light background with readable
level-coloured text.
"""

from __future__ import annotations

import time
from PySide6.QtCore import Qt
from PySide6.QtGui import QFont, QColor, QTextCursor
from PySide6.QtWidgets import (
    QTextEdit,
    QVBoxLayout,
    QWidget,
    QHBoxLayout,
    QLabel,
    QPushButton,
)

from config.theme import Theme


class LogConsole(QWidget):
    """
    Light-themed scrolling log viewer.

    Usage
    -----
    >>> console = LogConsole()
    >>> console.log("SecOC_Init complete", level="info")
    >>> console.log("Verification FAILED", level="error")
    """

    _LEVEL_COLORS = {
        "info":    Theme.color.CYAN_DARK,
        "success": Theme.color.GREEN_DARK,
        "warning": Theme.color.YELLOW_DARK,
        "error":   Theme.color.RED_DARK,
        "debug":   Theme.color.TEXT_MUTED,
        "pqc":     Theme.color.PQC_PURPLE,
    }

    def __init__(self, title: str = "Log", max_lines: int = 500, parent=None):
        super().__init__(parent)
        self._max_lines = max_lines

        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(4)

        # Header row
        header = QHBoxLayout()
        lbl = QLabel(title.upper())
        lbl.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
            letter-spacing: 1px;
        """)
        self._clear_btn = QPushButton("Clear")
        self._clear_btn.setCursor(Qt.PointingHandCursor)
        self._clear_btn.setStyleSheet(f"""
            QPushButton {{
                background: transparent;
                color: {Theme.color.TEXT_MUTED};
                border: 1px solid {Theme.color.BORDER};
                border-radius: 4px;
                padding: 2px 10px;
                font-size: {Theme.font.SIZE_TINY}px;
            }}
            QPushButton:hover {{
                color: {Theme.color.TEXT_PRIMARY};
                border-color: {Theme.color.TEXT_SECONDARY};
            }}
        """)
        self._clear_btn.clicked.connect(self.clear)
        header.addWidget(lbl)
        header.addStretch()
        header.addWidget(self._clear_btn)
        lay.addLayout(header)

        # Text area – light background
        self._text = QTextEdit()
        self._text.setReadOnly(True)
        self._text.setFont(QFont(Theme.font.FAMILY_MONO, Theme.font.SIZE_SMALL))
        self._text.setStyleSheet(f"""
            QTextEdit {{
                background: {Theme.color.BG_ELEVATED};
                color: {Theme.color.TEXT_PRIMARY};
                border: 1px solid {Theme.color.BORDER};
                border-radius: {Theme.radius.INPUT}px;
                padding: 8px;
            }}
        """)
        lay.addWidget(self._text)

    # ── Public API ───────────────────────────────────────────────────

    def log(self, message: str, level: str = "info") -> None:
        """Append a timestamped, colour-coded line."""
        color = self._LEVEL_COLORS.get(level, Theme.color.TEXT_PRIMARY)
        ts = time.strftime("%H:%M:%S")
        tag = level.upper().ljust(7)
        html = (
            f'<span style="color:{Theme.color.TEXT_MUTED}">[{ts}]</span> '
            f'<span style="color:{color};font-weight:bold">{tag}</span> '
            f'<span style="color:{Theme.color.TEXT_PRIMARY}">{_escape(message)}</span>'
        )
        self._text.append(html)

        # Auto-trim
        if self._text.document().blockCount() > self._max_lines:
            cursor = self._text.textCursor()
            cursor.movePosition(QTextCursor.Start)
            cursor.movePosition(QTextCursor.Down, QTextCursor.KeepAnchor, 50)
            cursor.removeSelectedText()

        # Auto-scroll to bottom
        self._text.moveCursor(QTextCursor.End)

    def clear(self) -> None:
        self._text.clear()


def _escape(text: str) -> str:
    """Basic HTML-escape for log messages."""
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
