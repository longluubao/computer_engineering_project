"""
Automotive-inspired gauge widgets – radial and linear.

Light-theme variant: clean arcs on white/light backgrounds,
dark text for readability, accent-colour fills.
"""

from __future__ import annotations

import math
from PySide6.QtCore import Qt, QRectF, Property, QPropertyAnimation, QEasingCurve
from PySide6.QtGui import (
    QColor,
    QConicalGradient,
    QFont,
    QPainter,
    QPen,
    QLinearGradient,
)
from PySide6.QtWidgets import QWidget

from config.theme import Theme


# ── RadialGauge ──────────────────────────────────────────────────────

class RadialGauge(QWidget):
    """
    Circular arc gauge reminiscent of an automotive instrument cluster.

    Parameters
    ----------
    label     : Display label under the value.
    unit      : Unit string (e.g. "µs", "kB/s").
    min_val   : Minimum value.
    max_val   : Maximum value.
    color     : Accent colour for the filled arc.
    size      : Widget diameter in pixels.
    """

    def __init__(
        self,
        label: str = "",
        unit: str = "",
        min_val: float = 0,
        max_val: float = 100,
        color: str = "",
        size: int = 160,
        parent: QWidget | None = None,
    ):
        super().__init__(parent)
        self.setFixedSize(size, size)
        self._label = label
        self._unit = unit
        self._min = min_val
        self._max = max_val
        self._value = min_val
        self._color = color or Theme.color.CYAN
        self._animated_value = min_val

        self._anim = QPropertyAnimation(self, b"animated_value")
        self._anim.setDuration(500)
        self._anim.setEasingCurve(QEasingCurve.OutCubic)

    # ── Animated property ────────────────────────────────────────────

    def _get_animated_value(self):
        return self._animated_value

    def _set_animated_value(self, v):
        self._animated_value = v
        self.update()

    animated_value = Property(float, _get_animated_value, _set_animated_value)

    # ── Public API ───────────────────────────────────────────────────

    def set_value(self, v: float) -> None:
        v = max(self._min, min(self._max, v))
        self._anim.stop()
        self._anim.setStartValue(self._animated_value)
        self._anim.setEndValue(v)
        self._anim.start()
        self._value = v

    def set_range(self, mn: float, mx: float) -> None:
        self._min, self._max = mn, mx

    # ── Paint ────────────────────────────────────────────────────────

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        s = min(self.width(), self.height())
        margin = 12
        rect = QRectF(margin, margin, s - 2 * margin, s - 2 * margin)
        pen_width = 8

        # Background arc (full 270°)  – light track
        start_angle = 225 * 16
        span_total = -270 * 16
        bg_pen = QPen(QColor(Theme.color.BORDER), pen_width, Qt.SolidLine, Qt.RoundCap)
        p.setPen(bg_pen)
        p.drawArc(rect, start_angle, span_total)

        # Value arc
        ratio = 0.0
        rng = self._max - self._min
        if rng > 0:
            ratio = (self._animated_value - self._min) / rng
        span_val = int(-270 * 16 * ratio)

        accent = QColor(self._color)
        val_pen = QPen(accent, pen_width, Qt.SolidLine, Qt.RoundCap)
        p.setPen(val_pen)
        p.drawArc(rect, start_angle, span_val)

        # Centre value text
        p.setPen(Qt.NoPen)
        font_val = QFont()
        font_val.setPixelSize(int(s * 0.22))
        font_val.setBold(True)
        p.setFont(font_val)
        p.setPen(QColor(Theme.color.TEXT_PRIMARY))
        val_text = f"{self._animated_value:,.0f}"
        p.drawText(rect, Qt.AlignCenter, val_text)

        # Unit label
        font_unit = QFont()
        font_unit.setPixelSize(int(s * 0.10))
        p.setFont(font_unit)
        p.setPen(QColor(Theme.color.TEXT_SECONDARY))
        unit_rect = QRectF(
            rect.left(), rect.center().y() + s * 0.12, rect.width(), s * 0.15
        )
        p.drawText(unit_rect, Qt.AlignHCenter | Qt.AlignTop, self._unit)

        # Bottom label
        font_lbl = QFont()
        font_lbl.setPixelSize(int(s * 0.09))
        font_lbl.setBold(True)
        p.setFont(font_lbl)
        p.setPen(QColor(Theme.color.TEXT_MUTED))
        lbl_rect = QRectF(0, s - margin - 4, s, margin + 4)
        p.drawText(lbl_rect, Qt.AlignHCenter | Qt.AlignBottom, self._label.upper())

        p.end()


# ── LinearGauge ──────────────────────────────────────────────────────

class LinearGauge(QWidget):
    """Horizontal bar gauge with gradient fill."""

    def __init__(
        self,
        label: str = "",
        unit: str = "",
        min_val: float = 0,
        max_val: float = 100,
        color: str = "",
        parent: QWidget | None = None,
    ):
        super().__init__(parent)
        self.setFixedHeight(48)
        self._label = label
        self._unit = unit
        self._min = min_val
        self._max = max_val
        self._value = min_val
        self._color = color or Theme.color.CYAN

    def set_value(self, v: float) -> None:
        self._value = max(self._min, min(self._max, v))
        self.update()

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        w, h = self.width(), self.height()
        bar_h = 10
        bar_y = h - bar_h - 4
        radius = bar_h // 2

        # Label + value
        font = QFont()
        font.setPixelSize(Theme.font.SIZE_SMALL)
        p.setFont(font)
        p.setPen(QColor(Theme.color.TEXT_SECONDARY))
        p.drawText(0, 0, w, bar_y - 2, Qt.AlignLeft | Qt.AlignBottom, self._label)

        val_text = f"{self._value:,.1f} {self._unit}"
        p.setPen(QColor(Theme.color.TEXT_PRIMARY))
        p.drawText(0, 0, w, bar_y - 2, Qt.AlignRight | Qt.AlignBottom, val_text)

        # Background track – light
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(Theme.color.BG_ELEVATED))
        p.drawRoundedRect(0, bar_y, w, bar_h, radius, radius)

        # Filled bar
        ratio = 0.0
        rng = self._max - self._min
        if rng > 0:
            ratio = (self._value - self._min) / rng
        fill_w = max(0, int(w * ratio))

        grad = QLinearGradient(0, 0, fill_w, 0)
        c = QColor(self._color)
        grad.setColorAt(0, c.lighter(120))
        grad.setColorAt(1, c)
        p.setBrush(grad)
        p.drawRoundedRect(0, bar_y, fill_w, bar_h, radius, radius)

        p.end()
