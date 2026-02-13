"""
Instrument-cluster-style status indicators.

Provides:
  - StatusLight : small coloured dot with label
  - MetricsBar  : single unified card with KPIs separated by dividers
  - MiniGauge   : tiny radial indicator for embedding in panels
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QRectF
from PySide6.QtGui import (
    QColor,
    QFont,
    QPainter,
    QPen,
)
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QVBoxLayout,
    QWidget,
    QGraphicsDropShadowEffect,
)

from config.theme import Theme


# ── StatusLight ───────────────────────────────────────────────────────

class StatusLight(QWidget):
    """Small coloured dot with a text label (tell-tale style)."""

    _PRESETS = {
        "ok":      Theme.color.GREEN,
        "error":   Theme.color.RED,
        "warning": Theme.color.YELLOW,
        "info":    Theme.color.CYAN,
        "pqc":     Theme.color.PQC_PURPLE,
        "idle":    Theme.color.TEXT_MUTED,
    }

    def __init__(self, text: str = "", preset: str = "idle", parent=None):
        super().__init__(parent)
        self._preset = preset
        self._color = QColor(self._PRESETS.get(preset, self._PRESETS["idle"]))
        self._text = text
        self.setFixedHeight(24)

    def set_preset(self, preset: str, text: str = "") -> None:
        self._preset = preset
        self._color = QColor(self._PRESETS.get(preset, self._PRESETS["idle"]))
        if text:
            self._text = text
        self.update()

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        dot_r = 8
        y_center = self.height() // 2

        # Dot glow (subtle)
        glow = QColor(self._color)
        glow.setAlpha(40)
        p.setBrush(glow)
        p.setPen(Qt.NoPen)
        p.drawEllipse(2, y_center - dot_r, dot_r * 2, dot_r * 2)

        # Solid dot
        p.setBrush(self._color)
        p.drawEllipse(4, y_center - dot_r + 2, dot_r * 2 - 4, dot_r * 2 - 4)

        # Label
        font = QFont()
        font.setPixelSize(Theme.font.SIZE_SMALL)
        p.setFont(font)
        p.setPen(QColor(Theme.color.TEXT_SECONDARY))
        p.drawText(
            dot_r * 2 + 8, 0, self.width() - dot_r * 2 - 8, self.height(),
            Qt.AlignVCenter | Qt.AlignLeft,
            self._text,
        )
        p.end()


# ── MetricsBar ────────────────────────────────────────────────────────

class MetricsBar(QFrame):
    """
    Single unified card holding all KPIs in a row, separated
    by thin vertical dividers.  No individual tiles, no icons.

    ┌──────────┬──────────┬──────────┬──────────┬──────────┐
    │  LABEL   │  LABEL   │  LABEL   │  LABEL   │  LABEL   │
    │  value   │  value   │  value   │  value   │  value   │
    └──────────┴──────────┴──────────┴──────────┴──────────┘
    """

    def __init__(
        self,
        metrics: list[tuple[str, str, str]],
        parent: QWidget | None = None,
    ):
        """
        Parameters
        ----------
        metrics : list of (label, initial_value, accent_colour)
        """
        super().__init__(parent)

        self.setStyleSheet(f"""
            MetricsBar {{
                background: {Theme.color.BG_CARD};
                border: none;
                border-radius: {Theme.radius.CARD}px;
            }}
        """)
        self.setFixedHeight(88)

        # Shadow
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(24)
        shadow.setOffset(0, 4)
        shadow.setColor(QColor(0, 0, 0, 14))
        self.setGraphicsEffect(shadow)

        row = QHBoxLayout(self)
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(0)

        self._value_labels: list[QLabel] = []

        for i, (label, value, accent) in enumerate(metrics):
            # Divider between cells
            if i > 0:
                div = QFrame()
                div.setFixedWidth(1)
                div.setStyleSheet(
                    f"background: {Theme.color.BORDER};"
                    f"margin-top: 0px; margin-bottom: 0px;"
                )
                row.addWidget(div)

            # ── Cell ──────────────────────────────────────────────
            cell = QWidget()
            cell.setStyleSheet("background: transparent; border: none;")
            cl = QVBoxLayout(cell)
            cl.setContentsMargins(16, 14, 16, 14)
            cl.setSpacing(4)
            cl.setAlignment(Qt.AlignCenter)

            lbl = QLabel(label.upper())
            lbl.setAlignment(Qt.AlignCenter)
            lbl.setStyleSheet(f"""
                color: {Theme.color.TEXT_MUTED};
                font-size: {Theme.font.SIZE_TINY}px;
                font-weight: 600;
                letter-spacing: 0.8px;
                border: none;
            """)

            val = QLabel(value)
            val.setAlignment(Qt.AlignCenter)
            val.setStyleSheet(f"""
                color: {accent};
                font-size: {Theme.font.SIZE_H2}px;
                font-weight: bold;
                border: none;
            """)

            cl.addWidget(lbl)
            cl.addWidget(val)
            row.addWidget(cell, stretch=1)
            self._value_labels.append(val)

    # ── public API ────────────────────────────────────────────────────

    def set_value(self, index: int, text: str) -> None:
        if 0 <= index < len(self._value_labels):
            self._value_labels[index].setText(text)


# ── StatusTile (kept for other pages) ─────────────────────────────────

class StatusTile(QFrame):
    """Compact metric tile — borderless, shadow-only."""

    def __init__(
        self,
        label: str = "",
        value: str = "—",
        accent: str = "",
        parent=None,
    ):
        super().__init__(parent)
        self._accent = accent or Theme.color.CYAN
        self.setMinimumHeight(82)

        self.setStyleSheet(f"""
            StatusTile {{
                background: {Theme.color.BG_CARD};
                border: none;
                border-radius: {Theme.radius.CARD}px;
            }}
        """)

        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(18)
        shadow.setOffset(0, 3)
        shadow.setColor(QColor(0, 0, 0, 16))
        self.setGraphicsEffect(shadow)

        lay = QVBoxLayout(self)
        lay.setContentsMargins(14, 10, 14, 10)
        lay.setSpacing(2)

        lbl = QLabel(label.upper())
        lbl.setStyleSheet(f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_TINY}px;
            font-weight: bold;
            letter-spacing: 1px;
            border: none;
        """)
        lay.addWidget(lbl)

        self._value_lbl = QLabel(value)
        self._value_lbl.setStyleSheet(f"""
            color: {self._accent};
            font-size: {Theme.font.SIZE_H2}px;
            font-weight: bold;
            border: none;
        """)
        lay.addWidget(self._value_lbl)

    def set_value(self, val: str) -> None:
        self._value_lbl.setText(val)


# ── MiniGauge ─────────────────────────────────────────────────────────

class MiniGauge(QWidget):
    """Tiny radial ring gauge for embedding in panels."""

    def __init__(
        self,
        value: float = 0,
        max_val: float = 100,
        color: str = "",
        size: int = 48,
        parent=None,
    ):
        super().__init__(parent)
        self.setFixedSize(size, size)
        self._value = value
        self._max = max_val
        self._color = color or Theme.color.CYAN

    def set_value(self, v: float) -> None:
        self._value = max(0, min(self._max, v))
        self.update()

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        s = min(self.width(), self.height())
        margin = 3
        rect = QRectF(margin, margin, s - 2 * margin, s - 2 * margin)
        pen_w = 4

        # Track
        p.setPen(QPen(QColor(Theme.color.BORDER), pen_w, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(rect, 225 * 16, -270 * 16)

        # Value arc
        ratio = self._value / self._max if self._max else 0
        span = int(-270 * 16 * ratio)
        p.setPen(QPen(QColor(self._color), pen_w, Qt.SolidLine, Qt.RoundCap))
        p.drawArc(rect, 225 * 16, span)

        # Centre text
        font = QFont()
        font.setPixelSize(int(s * 0.28))
        font.setBold(True)
        p.setFont(font)
        p.setPen(QColor(Theme.color.TEXT_PRIMARY))
        p.drawText(rect, Qt.AlignCenter, f"{int(self._value)}")
        p.end()
