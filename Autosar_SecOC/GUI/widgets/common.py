"""
Common reusable widgets – light professional theme.

Every widget auto-applies styling from the central Theme
so page-level code stays clean.
"""

from __future__ import annotations

from PySide6.QtCore import (
    Qt,
    QPropertyAnimation,
    QEasingCurve,
    Property,
    QSize,
    Signal,
)
from PySide6.QtGui import QColor, QPainter, QFont, QPen, QBrush
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
    QGraphicsDropShadowEffect,
)

from config.theme import Theme


# ── StyledCard ───────────────────────────────────────────────────────

class StyledCard(QFrame):
    """
    Rounded white card container with a subtle shadow.

    Parameters
    ----------
    title : str, optional
        Section header placed at the top.
    accent_color : str, optional
        Left-border accent colour override.
    """

    def __init__(
        self,
        title: str = "",
        accent_color: str = "",
        parent: QWidget | None = None,
    ):
        super().__init__(parent)
        accent = accent_color or Theme.color.CYAN
        self.setStyleSheet(f"""
            StyledCard {{
                background: {Theme.color.BG_CARD};
                border: none;
                border-left: 3px solid {accent};
                border-radius: {Theme.radius.CARD}px;
                padding: 0px;
            }}
        """)

        self._layout = QVBoxLayout(self)
        self._layout.setContentsMargins(16, 12, 16, 12)
        self._layout.setSpacing(8)

        if title:
            hdr = SectionHeader(title, parent=self)
            self._layout.addWidget(hdr)

        # Soft card shadow
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(20)
        shadow.setOffset(0, 2)
        shadow.setColor(QColor(0, 0, 0, 25))
        self.setGraphicsEffect(shadow)

    @property
    def card_layout(self) -> QVBoxLayout:
        return self._layout


# ── SectionHeader ────────────────────────────────────────────────────

class SectionHeader(QWidget):
    """Small uppercase header with a subtle underline."""

    def __init__(self, text: str, parent: QWidget | None = None):
        super().__init__(parent)
        lay = QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 4)
        lbl = QLabel(text.upper())
        lbl.setStyleSheet(f"""
            color: {Theme.color.TEXT_SECONDARY};
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
            letter-spacing: 1px;
            border: none;
            padding: 0;
        """)
        lay.addWidget(lbl)
        lay.addStretch()


# ── StatusBadge ──────────────────────────────────────────────────────

class StatusBadge(QLabel):
    """Compact tinted pill showing a status word."""

    # (background, foreground) – light tinted backgrounds
    _PRESETS = {
        "ok":      ("#E8F5E9", Theme.color.GREEN_DARK),
        "success": ("#E8F5E9", Theme.color.GREEN_DARK),
        "error":   ("#FFEBEE", Theme.color.RED_DARK),
        "fail":    ("#FFEBEE", Theme.color.RED_DARK),
        "warning": ("#FFF8E1", Theme.color.YELLOW_DARK),
        "info":    ("#E3F2FD", Theme.color.CYAN_DARK),
        "pqc":     ("#EDE9FE", Theme.color.PQC_PURPLE),
        "idle":    (Theme.color.BG_ELEVATED, Theme.color.TEXT_MUTED),
    }

    def __init__(self, text: str = "", preset: str = "idle", parent=None):
        super().__init__(text, parent)
        self.set_preset(preset, text)

    def set_preset(self, preset: str, text: str = "") -> None:
        bg, fg = self._PRESETS.get(preset, self._PRESETS["idle"])
        if text:
            self.setText(text)
        self.setStyleSheet(f"""
            background: {bg};
            color: {fg};
            border-radius: {Theme.radius.PILL}px;
            padding: 4px 14px;
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
        """)
        self.setAlignment(Qt.AlignCenter)


# ── IconButton ───────────────────────────────────────────────────────

class IconButton(QPushButton):
    """Flat button with optional emoji/icon prefix."""

    def __init__(
        self,
        text: str,
        icon_text: str = "",
        style: str = "primary",
        parent=None,
    ):
        display = f"{icon_text}  {text}" if icon_text else text
        super().__init__(display, parent)
        self.setCursor(Qt.PointingHandCursor)
        qss_map = {
            "primary": Theme.stylesheet.BTN_PRIMARY,
            "danger":  Theme.stylesheet.BTN_DANGER,
            "outline": Theme.stylesheet.BTN_OUTLINE,
        }
        self.setStyleSheet(qss_map.get(style, Theme.stylesheet.BTN_PRIMARY))
        self.setMinimumHeight(38)


# ── GlowLabel ────────────────────────────────────────────────────────

class GlowLabel(QLabel):
    """Bold coloured heading label (no glow on light theme)."""

    def __init__(
        self,
        text: str = "",
        color: str = "",
        font_size: int = 0,
        bold: bool = False,
        parent=None,
    ):
        super().__init__(text, parent)
        c = color or Theme.color.CYAN
        fs = font_size or Theme.font.SIZE_H2
        weight = "bold" if bold else "normal"
        self.setStyleSheet(f"""
            color: {c};
            font-size: {fs}px;
            font-weight: {weight};
            background: transparent;
            border: none;
        """)


# ── Separator ────────────────────────────────────────────────────────

class Separator(QFrame):
    """Thin horizontal divider line."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFrameShape(QFrame.HLine)
        self.setFixedHeight(1)
        self.setStyleSheet(f"background: {Theme.color.BORDER}; border: none;")


# ── AnimatedToggle ───────────────────────────────────────────────────

class AnimatedToggle(QWidget):
    """iOS-style toggle switch: CLASSIC ⇄ PQC."""

    toggled = Signal(bool)  # True → PQC, False → Classic

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(120, 36)
        self._pqc_mode = False
        self._handle_x = 2.0
        self.setCursor(Qt.PointingHandCursor)

        self._anim = QPropertyAnimation(self, b"handle_x")
        self._anim.setDuration(200)
        self._anim.setEasingCurve(QEasingCurve.InOutCubic)

    # ── Property for animation ───────────────────────────────────────

    def _get_handle_x(self):
        return self._handle_x

    def _set_handle_x(self, val):
        self._handle_x = val
        self.update()

    handle_x = Property(float, _get_handle_x, _set_handle_x)

    # ── Paint ────────────────────────────────────────────────────────

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        w, h = self.width(), self.height()
        r = h / 2

        # Track
        track_color = QColor(
            Theme.color.PQC_PURPLE if self._pqc_mode else Theme.color.BORDER
        )
        p.setBrush(track_color)
        p.setPen(Qt.NoPen)
        p.drawRoundedRect(0, 0, w, h, r, r)

        # Handle (white with subtle shadow look)
        handle_r = h - 4
        p.setBrush(QColor("#FFFFFF"))
        p.setPen(QPen(QColor(0, 0, 0, 30), 1))
        p.drawEllipse(int(self._handle_x), 2, handle_r, handle_r)

        # Label
        font = QFont()
        font.setPixelSize(Theme.font.SIZE_TINY)
        font.setBold(True)
        p.setFont(font)
        if self._pqc_mode:
            p.setPen(QPen(QColor(Theme.color.TEXT_BRIGHT)))
            p.drawText(8, 0, 50, h, Qt.AlignVCenter, "PQC")
        else:
            p.setPen(QPen(QColor(Theme.color.TEXT_SECONDARY)))
            p.drawText(w - 68, 0, 60, h, Qt.AlignVCenter, "Classic")
        p.end()

    # ── Interaction ──────────────────────────────────────────────────

    def mousePressEvent(self, _):
        self._pqc_mode = not self._pqc_mode
        end_x = self.width() - self.height() + 2
        self._anim.setStartValue(self._handle_x)
        self._anim.setEndValue(end_x if self._pqc_mode else 2.0)
        self._anim.start()
        self.toggled.emit(self._pqc_mode)

    @property
    def is_pqc(self) -> bool:
        return self._pqc_mode

    def set_pqc(self, enabled: bool) -> None:
        if enabled != self._pqc_mode:
            self.mousePressEvent(None)
