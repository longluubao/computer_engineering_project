"""
PQC Key Exchange visualisation page.

Animates the ML-KEM-768 three-phase key exchange handshake
between two simulated ECU peers, showing byte sizes at each step.
Light professional theme.
"""

from __future__ import annotations

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from widgets.common import (
    GlowLabel,
    IconButton,
    SectionHeader,
    StatusBadge,
    StyledCard,
    Separator,
)
from widgets.hex_viewer import HexViewer
from widgets.log_console import LogConsole
from core.data_models import KeyExchangeState


class KeyExchangePage(QWidget):
    """ML-KEM-768 key exchange visualisation."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._current_step = 0
        self._build_ui()
        self._connect_signals()

    # ── UI ───────────────────────────────────────────────────────────

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(24, 20, 24, 20)
        root.setSpacing(16)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet(Theme.stylesheet.SCROLL_AREA)
        scroll.setFrameShape(QFrame.NoFrame)

        container = QWidget()
        lay = QVBoxLayout(container)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(16)

        # Header
        hdr = GlowLabel(
            "ML-KEM-768 Key Exchange Protocol",
            color=Theme.color.PQC_PURPLE,
            font_size=Theme.font.SIZE_H1,
            bold=True,
        )
        lay.addWidget(hdr)

        info = QLabel(
            "NIST FIPS 203 \u2014 Module-Lattice-Based Key-Encapsulation Mechanism  \u2022  "
            f"PK {Settings.PQC_ALGORITHMS['ML-KEM-768']['pk_bytes']} B  \u2022  "
            f"CT {Settings.PQC_ALGORITHMS['ML-KEM-768']['ct_bytes']} B  \u2022  "
            f"SS {Settings.PQC_ALGORITHMS['ML-KEM-768']['ss_bytes']} B"
        )
        info.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; font-size:{Theme.font.SIZE_BODY}px;"
        )
        info.setWordWrap(True)
        lay.addWidget(info)

        # ── Two ECU columns ──────────────────────────────────────────
        peers = QHBoxLayout()
        peers.setSpacing(20)
        self._ecu_a_card = self._build_ecu_card(
            "ECU-A (Initiator / Alice)", Theme.color.CYAN
        )
        self._ecu_b_card = self._build_ecu_card(
            "ECU-B (Responder / Bob)", Theme.color.ORANGE
        )
        peers.addWidget(self._ecu_a_card, stretch=1)
        peers.addWidget(self._build_arrow_column(), stretch=0)
        peers.addWidget(self._ecu_b_card, stretch=1)
        lay.addLayout(peers)

        # ── Step-by-step panel ───────────────────────────────────────
        lay.addWidget(self._build_steps_card())

        # ── Controls ─────────────────────────────────────────────────
        ctrl_row = QHBoxLayout()
        self._btn_start = IconButton("Start Key Exchange", "\U0001F511", "primary")
        self._btn_next = IconButton("Next Step", "\u27A1\uFE0F", "outline")
        self._btn_reset = IconButton("Reset", "\U0001F504", "danger")
        self._btn_next.setEnabled(False)
        ctrl_row.addWidget(self._btn_start)
        ctrl_row.addWidget(self._btn_next)
        ctrl_row.addStretch()
        ctrl_row.addWidget(self._btn_reset)
        lay.addLayout(ctrl_row)

        # Log
        self._log = LogConsole("Key Exchange Log")
        lay.addWidget(self._log)

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    def _build_ecu_card(self, title: str, color: str) -> StyledCard:
        card = StyledCard(title, accent_color=color)
        status = StatusBadge("IDLE", "idle")
        card._status_badge = status
        card.card_layout.addWidget(status, alignment=Qt.AlignLeft)

        hex_view = HexViewer("Key Material")
        card._hex_view = hex_view
        card.card_layout.addWidget(hex_view)
        return card

    def _build_arrow_column(self) -> QWidget:
        col = QWidget()
        col.setFixedWidth(80)
        lay = QVBoxLayout(col)
        lay.setAlignment(Qt.AlignCenter)
        lay.setSpacing(16)

        self._arrow_labels = []
        arrows = [
            ("\u2192 PK", "1,184 B"),
            ("\u2190 CT", "1,088 B"),
            ("\u2713 SS", "32 B"),
        ]
        for txt, size in arrows:
            lbl = QLabel(f"{txt}\n{size}")
            lbl.setAlignment(Qt.AlignCenter)
            lbl.setStyleSheet(self._arrow_default_style())
            self._arrow_labels.append(lbl)
            lay.addWidget(lbl)

        return col

    def _build_steps_card(self) -> StyledCard:
        card = StyledCard("Protocol Steps")
        self._step_labels = []
        steps = [
            ("1. KeyGen", "Alice generates ML-KEM-768 key pair (pk, sk)"),
            (
                "2. Encapsulate",
                "Bob uses Alice's pk to create ciphertext ct and shared secret ss",
            ),
            (
                "3. Decapsulate",
                "Alice uses sk to extract shared secret ss from ct",
            ),
            ("4. Established", "Both ECUs now share the same 32-byte secret"),
        ]
        for title, desc in steps:
            row = QHBoxLayout()
            t = QLabel(title)
            t.setFixedWidth(140)
            t.setStyleSheet(f"""
                color: {Theme.color.PQC_PURPLE};
                font-weight: bold;
                font-size: {Theme.font.SIZE_BODY}px;
                border: none;
            """)
            d = QLabel(desc)
            d.setStyleSheet(f"""
                color: {Theme.color.TEXT_SECONDARY};
                font-size: {Theme.font.SIZE_BODY}px;
                border: none;
            """)
            d.setWordWrap(True)
            badge = StatusBadge("Pending", "idle")
            row.addWidget(t)
            row.addWidget(d, stretch=1)
            row.addWidget(badge)
            card.card_layout.addLayout(row)
            self._step_labels.append(badge)
        return card

    # ── Signals ──────────────────────────────────────────────────────

    def _connect_signals(self):
        self._btn_start.clicked.connect(self._on_start)
        self._btn_next.clicked.connect(self._on_next_step)
        self._btn_reset.clicked.connect(self._on_reset)

    # ── Helpers ──────────────────────────────────────────────────────

    @staticmethod
    def _arrow_default_style() -> str:
        return f"""
            color: {Theme.color.TEXT_MUTED};
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
            background: {Theme.color.BG_ELEVATED};
            border: 1px solid {Theme.color.BORDER};
            border-radius: 8px;
            padding: 8px 4px;
        """

    @staticmethod
    def _arrow_active_style(color: str) -> str:
        return f"""
            color: {color};
            font-size: {Theme.font.SIZE_SMALL}px;
            font-weight: bold;
            background: {Theme.color.BG_CARD};
            border: 1px solid {color};
            border-radius: 8px;
            padding: 8px 4px;
        """

    # ── Simulation slots ─────────────────────────────────────────────

    def _on_start(self):
        self._current_step = 0
        self._btn_start.setEnabled(False)
        self._btn_next.setEnabled(True)
        self._log.log(
            "Key exchange initiated \u2014 Alice generates ML-KEM-768 keypair", "pqc"
        )
        self._highlight_step(0)
        self._ecu_a_card._status_badge.set_preset("info", "INITIATED")
        self._arrow_labels[0].setStyleSheet(
            self._arrow_active_style(Theme.color.CYAN)
        )

    def _on_next_step(self):
        self._current_step += 1
        if self._current_step == 1:
            self._log.log(
                "Bob encapsulates: generates ciphertext (1088 B) + shared secret (32 B)",
                "pqc",
            )
            self._highlight_step(1)
            self._ecu_b_card._status_badge.set_preset("info", "RESPONDED")
            self._arrow_labels[1].setStyleSheet(
                self._arrow_active_style(Theme.color.ORANGE)
            )
        elif self._current_step == 2:
            self._log.log(
                "Alice decapsulates ciphertext \u2192 derives shared secret (32 B)",
                "pqc",
            )
            self._highlight_step(2)
            self._arrow_labels[2].setStyleSheet(
                self._arrow_active_style(Theme.color.GREEN)
            )
        elif self._current_step >= 3:
            self._log.log(
                "\u2713 Key exchange ESTABLISHED \u2014 both ECUs share 32 B secret",
                "success",
            )
            self._highlight_step(3)
            self._ecu_a_card._status_badge.set_preset("success", "ESTABLISHED")
            self._ecu_b_card._status_badge.set_preset("success", "ESTABLISHED")
            self._btn_next.setEnabled(False)

    def _on_reset(self):
        self._current_step = 0
        self._btn_start.setEnabled(True)
        self._btn_next.setEnabled(False)
        for badge in self._step_labels:
            badge.set_preset("idle", "Pending")
        for lbl in self._arrow_labels:
            lbl.setStyleSheet(self._arrow_default_style())
        self._ecu_a_card._status_badge.set_preset("idle", "IDLE")
        self._ecu_b_card._status_badge.set_preset("idle", "IDLE")
        self._log.log("Key exchange reset", "warning")

    def _highlight_step(self, idx: int):
        for i, badge in enumerate(self._step_labels):
            if i < idx:
                badge.set_preset("success", "Done")
            elif i == idx:
                badge.set_preset("pqc", "Active")
            else:
                badge.set_preset("idle", "Pending")
