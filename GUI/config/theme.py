"""
Centralised theme for the AUTOSAR SecOC PQC Dashboard.

Modern borderless design — elevation via shadows, no hard borders.
All colour tokens, font stacks, border radii, and reusable QSS
fragments live here so that every widget and page stays in sync.
"""

from __future__ import annotations


class _Palette:
    """Colour tokens – clean modern palette."""

    # ── Primary accent ────────────────────────────────────────────────
    CYAN          = "#0098CE"
    CYAN_DARK     = "#007AA3"
    CYAN_GLOW     = "#00B8F0"
    CYAN_LIGHT    = "#E6F7FF"

    # ── Secondary accent ──────────────────────────────────────────────
    ORANGE        = "#E67E22"
    ORANGE_DARK   = "#D35400"
    ORANGE_LIGHT  = "#FFF3E0"

    # ── Status ────────────────────────────────────────────────────────
    GREEN         = "#10B981"
    GREEN_DARK    = "#059669"
    GREEN_LIGHT   = "#ECFDF5"
    RED           = "#EF4444"
    RED_DARK      = "#DC2626"
    RED_LIGHT     = "#FEF2F2"
    YELLOW        = "#F59E0B"
    YELLOW_DARK   = "#D97706"

    # ── Surfaces ──────────────────────────────────────────────────────
    BG_PAGE       = "#F4F6F9"           # page root
    BG_DARKEST    = "#F4F6F9"           # alias
    BG_DARK       = "#F7F8FA"
    BG_CARD       = "#FFFFFF"
    BG_CARD_HOVER = "#FAFBFC"
    BG_ELEVATED   = "#F7F8FB"
    BG_INPUT      = "#FFFFFF"

    # ── Sidebar (light — integrated with content) ─────────────────────
    BG_SIDEBAR         = "#FFFFFF"
    BG_SIDEBAR_HOVER   = "#F0F4F8"
    TEXT_SIDEBAR        = "#334155"
    TEXT_SIDEBAR_MUTED  = "#94A3B8"

    # ── Borders — very subtle, only where needed ──────────────────────
    BORDER        = "#EDF0F4"
    BORDER_LIGHT  = "#F3F5F8"
    BORDER_ACCENT = "#0098CE"

    # ── Text ──────────────────────────────────────────────────────────
    TEXT_PRIMARY   = "#1E293B"
    TEXT_SECONDARY = "#64748B"
    TEXT_MUTED     = "#94A3B8"
    TEXT_BRIGHT    = "#FFFFFF"

    # ── PQC ───────────────────────────────────────────────────────────
    PQC_PURPLE     = "#7C3AED"
    PQC_PURPLE_DIM = "#6D28D9"
    PQC_PURPLE_LIGHT = "#F3E8FF"
    CLASSIC_BLUE   = "#2563EB"

    # ── ECU domain colours (matching the zonal diagram) ───────────────
    ECU_POWERTRAIN   = "#10B981"   # green
    ECU_INFOTAINMENT = "#EF4444"   # red
    ECU_BODY         = "#60A5FA"   # blue
    ECU_CHASSIS      = "#F59E0B"   # orange
    ECU_ADAS         = "#A78BFA"   # purple


class _Fonts:
    """Font-family stacks and sizes."""

    FAMILY_PRIMARY = "'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif"
    FAMILY_MONO    = "'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace"

    SIZE_HERO  = 26
    SIZE_H1    = 20
    SIZE_H2    = 17
    SIZE_H3    = 14
    SIZE_BODY  = 13
    SIZE_SMALL = 11
    SIZE_TINY  = 10


class _Radius:
    """Border-radius values (px)."""

    CARD   = 16
    BUTTON = 10
    INPUT  = 8
    PILL   = 20
    CIRCLE = 50


class Theme:
    """Central theme — modern borderless design with shadows for depth."""

    color  = _Palette
    font   = _Fonts
    radius = _Radius

    class stylesheet:
        """Reusable Qt stylesheet snippets."""

        GLOBAL = f"""
            QWidget {{
                font-family: {_Fonts.FAMILY_PRIMARY};
                font-size: {_Fonts.SIZE_BODY}px;
                color: {_Palette.TEXT_PRIMARY};
                background: transparent;
            }}
            QToolTip {{
                background: {_Palette.BG_CARD};
                color: {_Palette.TEXT_PRIMARY};
                border: none;
                padding: 8px 12px;
                border-radius: {_Radius.INPUT}px;
                font-size: {_Fonts.SIZE_SMALL}px;
            }}
        """

        # No border — shadow-only elevation
        CARD = f"""
            background: {_Palette.BG_CARD};
            border: none;
            border-radius: {_Radius.CARD}px;
            padding: 20px;
        """

        CARD_HOVER = f"""
            background: {_Palette.BG_CARD_HOVER};
            border: none;
            border-radius: {_Radius.CARD}px;
            padding: 20px;
        """

        BTN_PRIMARY = f"""
            QPushButton {{
                background: {_Palette.CYAN};
                color: {_Palette.TEXT_BRIGHT};
                border: none;
                border-radius: {_Radius.BUTTON}px;
                padding: 10px 24px;
                font-weight: 600;
                font-size: {_Fonts.SIZE_BODY}px;
            }}
            QPushButton:hover {{
                background: {_Palette.CYAN_GLOW};
            }}
            QPushButton:pressed {{
                background: {_Palette.CYAN_DARK};
            }}
            QPushButton:disabled {{
                background: {_Palette.BG_ELEVATED};
                color: {_Palette.TEXT_MUTED};
            }}
        """

        BTN_DANGER = f"""
            QPushButton {{
                background: {_Palette.RED};
                color: {_Palette.TEXT_BRIGHT};
                border: none;
                border-radius: {_Radius.BUTTON}px;
                padding: 10px 24px;
                font-weight: 600;
                font-size: {_Fonts.SIZE_BODY}px;
            }}
            QPushButton:hover {{
                background: {_Palette.RED_DARK};
            }}
            QPushButton:pressed {{
                background: {_Palette.RED_DARK};
            }}
        """

        BTN_OUTLINE = f"""
            QPushButton {{
                background: transparent;
                color: {_Palette.CYAN};
                border: 1.5px solid {_Palette.CYAN};
                border-radius: {_Radius.BUTTON}px;
                padding: 10px 24px;
                font-size: {_Fonts.SIZE_BODY}px;
            }}
            QPushButton:hover {{
                background: rgba(0, 152, 206, 0.06);
            }}
            QPushButton:pressed {{
                background: rgba(0, 152, 206, 0.12);
            }}
        """

        INPUT = f"""
            QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox {{
                background: {_Palette.BG_INPUT};
                color: {_Palette.TEXT_PRIMARY};
                border: 1.5px solid {_Palette.BORDER};
                border-radius: {_Radius.INPUT}px;
                padding: 8px 12px;
                font-family: {_Fonts.FAMILY_MONO};
                font-size: {_Fonts.SIZE_BODY}px;
                selection-background-color: rgba(0, 152, 206, 0.20);
            }}
            QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus {{
                border-color: {_Palette.CYAN};
            }}
        """

        COMBO = f"""
            QComboBox {{
                background: {_Palette.BG_INPUT};
                color: {_Palette.TEXT_PRIMARY};
                border: 1.5px solid {_Palette.BORDER};
                border-radius: {_Radius.INPUT}px;
                padding: 8px 12px;
                font-size: {_Fonts.SIZE_BODY}px;
                min-width: 120px;
            }}
            QComboBox:hover {{
                border-color: {_Palette.CYAN};
            }}
            QComboBox::drop-down {{
                border: none;
                width: 30px;
            }}
            QComboBox QAbstractItemView {{
                background: {_Palette.BG_CARD};
                color: {_Palette.TEXT_PRIMARY};
                border: none;
                selection-background-color: rgba(0, 152, 206, 0.12);
                selection-color: {_Palette.CYAN_DARK};
                outline: none;
            }}
        """

        SCROLL_AREA = f"""
            QScrollArea {{
                border: none;
                background: transparent;
            }}
            QScrollBar:vertical {{
                background: transparent;
                width: 6px;
                border-radius: 3px;
            }}
            QScrollBar::handle:vertical {{
                background: rgba(0, 0, 0, 0.10);
                border-radius: 3px;
                min-height: 30px;
            }}
            QScrollBar::handle:vertical:hover {{
                background: rgba(0, 0, 0, 0.18);
            }}
            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {{
                height: 0px;
            }}
            QScrollBar:horizontal {{
                background: transparent;
                height: 6px;
                border-radius: 3px;
            }}
            QScrollBar::handle:horizontal {{
                background: rgba(0, 0, 0, 0.10);
                border-radius: 3px;
                min-width: 30px;
            }}
            QScrollBar::handle:horizontal:hover {{
                background: rgba(0, 0, 0, 0.18);
            }}
            QScrollBar::add-line:horizontal,
            QScrollBar::sub-line:horizontal {{
                width: 0px;
            }}
        """

        TAB_BAR = f"""
            QTabWidget::pane {{
                border: none;
                border-radius: {_Radius.CARD}px;
                background: {_Palette.BG_CARD};
                top: -1px;
            }}
            QTabBar::tab {{
                background: transparent;
                color: {_Palette.TEXT_MUTED};
                border: none;
                padding: 10px 20px;
                margin-right: 4px;
                border-radius: {_Radius.BUTTON}px;
                font-size: {_Fonts.SIZE_BODY}px;
            }}
            QTabBar::tab:selected {{
                background: {_Palette.BG_CARD};
                color: {_Palette.CYAN};
                font-weight: bold;
            }}
            QTabBar::tab:hover:!selected {{
                background: rgba(0, 0, 0, 0.04);
                color: {_Palette.TEXT_PRIMARY};
            }}
        """

        PROGRESS = f"""
            QProgressBar {{
                background: {_Palette.BG_ELEVATED};
                border: none;
                border-radius: 6px;
                height: 12px;
                text-align: center;
                font-size: {_Fonts.SIZE_TINY}px;
                color: {_Palette.TEXT_SECONDARY};
            }}
            QProgressBar::chunk {{
                background: {_Palette.CYAN};
                border-radius: 6px;
            }}
        """

    @classmethod
    def apply_global(cls, app):
        """Apply the global stylesheet to a QApplication instance."""
        app.setStyleSheet(cls.stylesheet.GLOBAL)
