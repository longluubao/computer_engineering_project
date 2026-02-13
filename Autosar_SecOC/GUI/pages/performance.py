"""
Performance benchmark page.

Runs sign / verify operations in batches and visualises latency,
throughput, and PQC-vs-Classic comparison charts using Matplotlib
embedded in Qt.  Light professional theme.
"""

from __future__ import annotations

import time
from typing import List

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QScrollArea,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from core.data_models import AuthMode, PerfSample
from widgets.common import (
    GlowLabel,
    IconButton,
    StatusBadge,
    StyledCard,
)
from widgets.gauges import RadialGauge, LinearGauge
from widgets.log_console import LogConsole

# Matplotlib with Qt backend
import matplotlib

matplotlib.use("QtAgg")
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import numpy as np


class PerformancePage(QWidget):
    """PQC vs Classic performance benchmark dashboard."""

    def __init__(self, bridge: BackendBridge, parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._samples_classic: List[PerfSample] = []
        self._samples_pqc: List[PerfSample] = []
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
            "Performance Benchmark",
            color=Theme.color.GREEN_DARK,
            font_size=Theme.font.SIZE_H1,
            bold=True,
        )
        lay.addWidget(hdr)

        # ── Controls ─────────────────────────────────────────────────
        ctrl_card = StyledCard("Benchmark Configuration")
        ctrl_row = QHBoxLayout()

        lbl_iter = QLabel("Iterations:")
        lbl_iter.setStyleSheet(
            f"color:{Theme.color.TEXT_SECONDARY}; border:none;"
        )
        self._spin_iter = QSpinBox()
        self._spin_iter.setRange(1, 500)
        self._spin_iter.setValue(50)
        self._spin_iter.setStyleSheet(Theme.stylesheet.INPUT)
        self._spin_iter.setFixedWidth(80)

        self._btn_run = IconButton("Run Benchmark", "\u26A1", "primary")
        self._btn_clear = IconButton("Clear", "\U0001F5D1\uFE0F", "outline")
        self._status = StatusBadge("Ready", "idle")

        ctrl_row.addWidget(lbl_iter)
        ctrl_row.addWidget(self._spin_iter)
        ctrl_row.addWidget(self._btn_run)
        ctrl_row.addWidget(self._btn_clear)
        ctrl_row.addStretch()
        ctrl_row.addWidget(self._status)
        ctrl_card.card_layout.addLayout(ctrl_row)
        lay.addWidget(ctrl_card)

        # ── Gauges row ───────────────────────────────────────────────
        gauge_row = QHBoxLayout()
        gauge_row.setSpacing(16)

        self._gauge_classic_sign = RadialGauge(
            "HMAC Gen", "\u00b5s", 0, 50, Theme.color.CLASSIC_BLUE, 150
        )
        self._gauge_classic_verify = RadialGauge(
            "HMAC Verify", "\u00b5s", 0, 50, Theme.color.CLASSIC_BLUE, 150
        )
        self._gauge_pqc_sign = RadialGauge(
            "ML-DSA Sign", "\u00b5s", 0, 1000, Theme.color.PQC_PURPLE, 150
        )
        self._gauge_pqc_verify = RadialGauge(
            "ML-DSA Verify", "\u00b5s", 0, 500, Theme.color.PQC_PURPLE, 150
        )

        for g in [
            self._gauge_classic_sign,
            self._gauge_classic_verify,
            self._gauge_pqc_sign,
            self._gauge_pqc_verify,
        ]:
            card_w = StyledCard()
            card_w.card_layout.addWidget(g, alignment=Qt.AlignCenter)
            gauge_row.addWidget(card_w)

        lay.addLayout(gauge_row)

        # ── Chart ────────────────────────────────────────────────────
        chart_card = StyledCard("Latency Distribution")
        self._figure = Figure(figsize=(10, 4), facecolor="#FFFFFF")
        self._canvas = FigureCanvas(self._figure)
        self._canvas.setMinimumHeight(300)
        chart_card.card_layout.addWidget(self._canvas)
        lay.addWidget(chart_card)

        # ── Stats table ──────────────────────────────────────────────
        self._stats_card = StyledCard("Statistics Summary")
        self._stats_grid = QGridLayout()
        self._stats_card.card_layout.addLayout(self._stats_grid)
        lay.addWidget(self._stats_card)

        # Log
        self._log = LogConsole("Benchmark Log")
        lay.addWidget(self._log)

        lay.addStretch()
        scroll.setWidget(container)
        root.addWidget(scroll)

    # ── Signals ──────────────────────────────────────────────────────

    def _connect_signals(self):
        self._btn_run.clicked.connect(self._run_benchmark)
        self._btn_clear.clicked.connect(self._clear)

    # ── Benchmark ────────────────────────────────────────────────────

    def _run_benchmark(self):
        n = self._spin_iter.value()
        self._status.set_preset("info", f"Running {n}\u00d7\u2026")
        self._log.log(f"Starting benchmark: {n} iterations", "info")
        self._btn_run.setEnabled(False)

        data = bytes.fromhex("01A34F00FF221188AABBCCDD")

        classic_auth_times = []
        classic_verify_times = []
        pqc_auth_times = []
        pqc_verify_times = []

        for i in range(n):
            # Classic
            _, t_ca = self._bridge.authenticate(0, data, AuthMode.CLASSIC)
            classic_auth_times.append(t_ca)
            vr_c = self._bridge.verify(0, AuthMode.CLASSIC)
            classic_verify_times.append(vr_c.elapsed_us)

            # PQC
            _, t_pa = self._bridge.authenticate(0, data, AuthMode.PQC)
            pqc_auth_times.append(t_pa)
            vr_p = self._bridge.verify(0, AuthMode.PQC)
            pqc_verify_times.append(vr_p.elapsed_us)

        # Update gauges
        self._gauge_classic_sign.set_value(np.mean(classic_auth_times))
        self._gauge_classic_verify.set_value(np.mean(classic_verify_times))
        self._gauge_pqc_sign.set_value(np.mean(pqc_auth_times))
        self._gauge_pqc_verify.set_value(np.mean(pqc_verify_times))

        # Plot
        self._plot_results(
            classic_auth_times,
            classic_verify_times,
            pqc_auth_times,
            pqc_verify_times,
        )

        # Stats table
        self._update_stats(
            classic_auth_times,
            classic_verify_times,
            pqc_auth_times,
            pqc_verify_times,
        )

        self._status.set_preset("success", "Done \u2713")
        self._log.log(
            f"Benchmark complete \u2014 Classic avg sign: "
            f"{np.mean(classic_auth_times):.1f} \u00b5s, "
            f"PQC avg sign: {np.mean(pqc_auth_times):.1f} \u00b5s",
            "success",
        )
        self._btn_run.setEnabled(True)

    def _plot_results(self, ca, cv, pa, pv):
        self._figure.clear()
        ax = self._figure.add_subplot(111)

        # Light theme chart styling
        ax.set_facecolor("#FAFBFC")
        for spine in ax.spines.values():
            spine.set_color(Theme.color.BORDER)
        ax.tick_params(colors=Theme.color.TEXT_SECONDARY, labelsize=9)
        ax.xaxis.label.set_color(Theme.color.TEXT_SECONDARY)
        ax.yaxis.label.set_color(Theme.color.TEXT_SECONDARY)
        ax.grid(True, axis="y", color=Theme.color.BORDER, linewidth=0.5, alpha=0.7)

        categories = [
            "HMAC\nGenerate",
            "HMAC\nVerify",
            "ML-DSA\nSign",
            "ML-DSA\nVerify",
        ]
        means = [np.mean(ca), np.mean(cv), np.mean(pa), np.mean(pv)]
        stds = [np.std(ca), np.std(cv), np.std(pa), np.std(pv)]
        colors = [
            Theme.color.CLASSIC_BLUE,
            Theme.color.CLASSIC_BLUE,
            Theme.color.PQC_PURPLE,
            Theme.color.PQC_PURPLE,
        ]

        bars = ax.bar(
            categories,
            means,
            yerr=stds,
            color=colors,
            edgecolor="none",
            capsize=4,
            alpha=0.85,
        )
        ax.set_ylabel("Latency (\u00b5s)")
        ax.set_title(
            "PQC vs Classic \u2014 Average Latency",
            color=Theme.color.TEXT_PRIMARY,
            fontsize=12,
            fontweight="bold",
        )

        for bar, mean in zip(bars, means):
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 2,
                f"{mean:.1f}",
                ha="center",
                va="bottom",
                color=Theme.color.TEXT_PRIMARY,
                fontsize=9,
            )

        self._figure.tight_layout()
        self._canvas.draw()

    def _update_stats(self, ca, cv, pa, pv):
        # Clear previous
        while self._stats_grid.count():
            item = self._stats_grid.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        headers = [
            "Metric",
            "HMAC Gen",
            "HMAC Verify",
            "ML-DSA Sign",
            "ML-DSA Verify",
        ]
        for col, h in enumerate(headers):
            lbl = QLabel(h)
            lbl.setStyleSheet(f"""
                color: {Theme.color.CYAN_DARK};
                font-weight: bold;
                font-size: {Theme.font.SIZE_SMALL}px;
                padding: 4px 8px;
                border: none;
            """)
            self._stats_grid.addWidget(lbl, 0, col)

        datasets = [ca, cv, pa, pv]
        rows = [
            ("Mean (\u00b5s)", [f"{np.mean(d):.2f}" for d in datasets]),
            ("Median (\u00b5s)", [f"{np.median(d):.2f}" for d in datasets]),
            ("Std Dev (\u00b5s)", [f"{np.std(d):.2f}" for d in datasets]),
            ("Min (\u00b5s)", [f"{np.min(d):.2f}" for d in datasets]),
            ("Max (\u00b5s)", [f"{np.max(d):.2f}" for d in datasets]),
        ]

        for r, (name, vals) in enumerate(rows, start=1):
            bg = Theme.color.BG_ELEVATED if r % 2 == 0 else "transparent"
            name_lbl = QLabel(name)
            name_lbl.setStyleSheet(
                f"color:{Theme.color.TEXT_SECONDARY}; padding:4px 8px; "
                f"background:{bg}; border:none;"
            )
            self._stats_grid.addWidget(name_lbl, r, 0)
            for c, v in enumerate(vals):
                val_lbl = QLabel(v)
                val_lbl.setStyleSheet(
                    f"color:{Theme.color.TEXT_PRIMARY}; padding:4px 8px; "
                    f"background:{bg}; border:none;"
                )
                self._stats_grid.addWidget(val_lbl, r, c + 1)

    def _clear(self):
        self._figure.clear()
        self._canvas.draw()
        self._gauge_classic_sign.set_value(0)
        self._gauge_classic_verify.set_value(0)
        self._gauge_pqc_sign.set_value(0)
        self._gauge_pqc_verify.set_value(0)
        while self._stats_grid.count():
            item = self._stats_grid.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        self._status.set_preset("idle", "Ready")
        self._log.clear()
