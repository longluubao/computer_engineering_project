
"""
PQC Premium Performance Dashboard
Professional visualization with glassmorphism and modern design
Based on NIST PQC benchmarking standards
"""

import sys
import csv
from pathlib import Path

try:
    from PySide6.QtWidgets import *
    from PySide6.QtCore import *
    from PySide6.QtGui import *
    PYSIDE_AVAILABLE = True
except ImportError:
    print("ERROR: PySide6 not installed. Run: pip install PySide6")
    sys.exit(1)

try:
    import matplotlib
    matplotlib.use('Qt5Agg')
    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
    from matplotlib.figure import Figure
    import numpy as np
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    print("ERROR: matplotlib not installed. Run: pip install matplotlib numpy")
    sys.exit(1)

# Premium color scheme with gradients
COLORS = {
    'bg_primary': '#0a0e27',
    'bg_secondary': '#1a1f3a',
    'bg_card': '#252b48',
    'accent_cyan': '#00f5ff',
    'accent_purple': '#a855f7',
    'accent_pink': '#ec4899',
    'accent_green': '#10b981',
    'accent_orange': '#f97316',
    'accent_blue': '#3b82f6',
    'text_primary': '#ffffff',
    'text_secondary': '#94a3b8',
    'text_muted': '#64748b',
    'success': '#22c55e',
    'warning': '#eab308',
    'danger': '#ef4444',
    'glass': 'rgba(255, 255, 255, 0.05)',
    'glass_border': 'rgba(255, 255, 255, 0.1)',
}

# Glassmorphism style
GLASS_STYLE = f"""
    background: {COLORS['glass']};
    border: 1px solid {COLORS['glass_border']};
    border-radius: 16px;
    backdrop-filter: blur(10px);
"""

class GradientWidget(QWidget):
    """Widget with gradient background"""
    def __init__(self, color1, color2, parent=None):
        super().__init__(parent)
        self.color1 = QColor(color1)
        self.color2 = QColor(color2)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        gradient = QLinearGradient(0, 0, self.width(), self.height())
        gradient.setColorAt(0, self.color1)
        gradient.setColorAt(1, self.color2)

        painter.fillRect(self.rect(), gradient)

class GlassCard(QFrame):
    """Glassmorphic card with gradient accent"""
    def __init__(self, title, value, unit, gradient_colors, icon="", parent=None):
        super().__init__(parent)
        self.setFixedHeight(160)

        # Apply glassmorphism
        self.setStyleSheet(f"""
            QFrame {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                           stop:0 rgba(255,255,255,0.03),
                                           stop:1 rgba(255,255,255,0.01));
                border: 1px solid rgba(255,255,255,0.1);
                border-radius: 20px;
            }}
        """)

        layout = QVBoxLayout(self)
        layout.setSpacing(8)
        layout.setContentsMargins(20, 20, 20, 20)

        # Gradient accent bar
        accent = QFrame()
        accent.setFixedHeight(4)
        accent.setStyleSheet(f"""
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                       stop:0 {gradient_colors[0]},
                                       stop:1 {gradient_colors[1]});
            border-radius: 2px;
        """)
        layout.addWidget(accent)

        # Title with icon
        title_container = QHBoxLayout()
        if icon:
            icon_label = QLabel(icon)
            icon_label.setStyleSheet(f"font-size: 24px; color: {gradient_colors[0]};")
            title_container.addWidget(icon_label)

        title_label = QLabel(title)
        title_label.setStyleSheet(f"""
            color: {COLORS['text_secondary']};
            font-size: 13px;
            font-weight: 500;
            letter-spacing: 0.5px;
        """)
        title_container.addWidget(title_label)
        title_container.addStretch()
        layout.addLayout(title_container)

        layout.addStretch()

        # Value
        self.value_label = QLabel(f"{value:.2f}")
        self.value_label.setStyleSheet(f"""
            color: {COLORS['text_primary']};
            font-size: 42px;
            font-weight: 700;
            letter-spacing: -1px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                       stop:0 {gradient_colors[0]},
                                       stop:1 {gradient_colors[1]});
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        """)
        layout.addWidget(self.value_label)

        # Unit
        unit_label = QLabel(unit)
        unit_label.setStyleSheet(f"""
            color: {COLORS['text_muted']};
            font-size: 14px;
            font-weight: 500;
        """)
        layout.addWidget(unit_label)

    def update_value(self, value):
        self.value_label.setText(f"{value:.2f}")

class ModernButton(QPushButton):
    """Modern button with gradient and hover effects"""
    def __init__(self, text, gradient_colors, icon="", parent=None):
        super().__init__(text, parent)
        self.gradient_colors = gradient_colors
        self.setFixedHeight(48)
        self.setCursor(Qt.PointingHandCursor)

        self.setStyleSheet(f"""
            QPushButton {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 {gradient_colors[0]},
                                           stop:1 {gradient_colors[1]});
                color: white;
                border: none;
                border-radius: 12px;
                font-size: 14px;
                font-weight: 600;
                padding: 12px 24px;
                letter-spacing: 0.5px;
            }}
            QPushButton:hover {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 {gradient_colors[1]},
                                           stop:1 {gradient_colors[0]});
            }}
            QPushButton:pressed {{
                background: {gradient_colors[0]};
            }}
        """)

class PremiumCanvas(FigureCanvasQTAgg):
    """Premium matplotlib canvas with dark theme"""
    def __init__(self, parent=None, width=8, height=6, dpi=100):
        self.fig = Figure(figsize=(width, height), dpi=dpi, facecolor='#0a0e27')
        super().__init__(self.fig)
        self.setParent(parent)
        self.fig.patch.set_alpha(0)

class PQCPremiumDashboard(QMainWindow):
    """Premium PQC Performance Dashboard"""

    def __init__(self):
        super().__init__()
        self.data = None
        self.init_ui()
        self.load_default_data()

    def init_ui(self):
        self.setWindowTitle("PQC Premium Performance Dashboard - AUTOSAR SecOC")
        self.setGeometry(50, 50, 1600, 1000)

        # Dark background
        self.setStyleSheet(f"""
            QMainWindow {{
                background: {COLORS['bg_primary']};
            }}
            QLabel {{
                color: {COLORS['text_primary']};
            }}
            QTabWidget::pane {{
                border: none;
                background: transparent;
            }}
            QTabBar::tab {{
                background: rgba(255, 255, 255, 0.05);
                color: {COLORS['text_secondary']};
                padding: 14px 28px;
                margin-right: 8px;
                border-radius: 12px 12px 0 0;
                font-size: 14px;
                font-weight: 500;
            }}
            QTabBar::tab:selected {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 {COLORS['accent_cyan']},
                                           stop:1 {COLORS['accent_purple']});
                color: white;
            }}
            QScrollArea {{
                border: none;
                background: transparent;
            }}
        """)

        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(24)
        main_layout.setContentsMargins(32, 32, 32, 32)

        # Header
        header = self.create_header()
        main_layout.addWidget(header)

        # Tabs
        tabs = QTabWidget()
        tabs.setDocumentMode(True)
        tabs.addTab(self.create_overview_tab(), "📊 Overview")
        tabs.addTab(self.create_mlkem_tab(), "🔐 ML-KEM Deep Dive")
        tabs.addTab(self.create_mldsa_tab(), "✍️ ML-DSA Deep Dive")
        tabs.addTab(self.create_comparison_tab(), "⚖️ PQC vs Classical")
        tabs.addTab(self.create_eth_gateway_tab(), "🌐 ETH Gateway Use Case")
        tabs.addTab(self.create_metrics_tab(), "📈 Advanced Metrics")

        main_layout.addWidget(tabs)

    def create_header(self):
        """Create premium header"""
        header = QFrame()
        header.setFixedHeight(120)
        header.setStyleSheet(f"""
            QFrame {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 rgba(0, 245, 255, 0.1),
                                           stop:0.5 rgba(168, 85, 247, 0.1),
                                           stop:1 rgba(236, 72, 153, 0.1));
                border: 1px solid rgba(255, 255, 255, 0.1);
                border-radius: 20px;
            }}
        """)

        layout = QHBoxLayout(header)
        layout.setContentsMargins(32, 24, 32, 24)

        # Title section
        title_layout = QVBoxLayout()
        title = QLabel("🛡️ Post-Quantum Cryptography Dashboard")
        title.setStyleSheet(f"""
            font-size: 32px;
            font-weight: 700;
            color: {COLORS['text_primary']};
            letter-spacing: -0.5px;
        """)
        subtitle = QLabel("AUTOSAR SecOC • ML-KEM-768 & ML-DSA-65 • NIST Level 3")
        subtitle.setStyleSheet(f"""
            font-size: 15px;
            color: {COLORS['text_secondary']};
            letter-spacing: 0.5px;
            margin-top: 4px;
        """)
        title_layout.addWidget(title)
        title_layout.addWidget(subtitle)
        layout.addLayout(title_layout)

        layout.addStretch()

        # Buttons
        load_btn = ModernButton("📂 Load Data", [COLORS['accent_cyan'], COLORS['accent_blue']])
        load_btn.clicked.connect(self.load_csv_file)
        layout.addWidget(load_btn)

        export_btn = ModernButton("💾 Export Report", [COLORS['accent_purple'], COLORS['accent_pink']])
        export_btn.clicked.connect(self.export_report)
        layout.addWidget(export_btn)

        return header

    def create_overview_tab(self):
        """Create overview with metrics cards"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(24)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        scroll_content = QWidget()
        scroll_layout = QVBoxLayout(scroll_content)
        scroll_layout.setSpacing(24)

        # Section: Key Exchange Performance
        section1 = QLabel("Key Exchange Performance (ML-KEM-768)")
        section1.setStyleSheet(f"""
            font-size: 18px;
            font-weight: 600;
            color: {COLORS['accent_cyan']};
            margin-top: 8px;
        """)
        scroll_layout.addWidget(section1)

        cards1 = QGridLayout()
        cards1.setSpacing(16)

        self.cards = {}
        self.cards['mlkem_keygen'] = GlassCard(
            "Key Generation", 0, "µs",
            [COLORS['accent_cyan'], COLORS['accent_blue']], "🔑"
        )
        cards1.addWidget(self.cards['mlkem_keygen'], 0, 0)

        self.cards['mlkem_encap'] = GlassCard(
            "Encapsulation", 0, "µs",
            [COLORS['accent_blue'], COLORS['accent_purple']], "📦"
        )
        cards1.addWidget(self.cards['mlkem_encap'], 0, 1)

        self.cards['mlkem_decap'] = GlassCard(
            "Decapsulation", 0, "µs",
            [COLORS['accent_purple'], COLORS['accent_pink']], "🔓"
        )
        cards1.addWidget(self.cards['mlkem_decap'], 0, 2)

        self.cards['mlkem_handshake'] = GlassCard(
            "Full Handshake", 0, "ms",
            [COLORS['accent_pink'], COLORS['accent_cyan']], "🤝"
        )
        cards1.addWidget(self.cards['mlkem_handshake'], 0, 3)

        scroll_layout.addLayout(cards1)

        # Section: Digital Signatures
        section2 = QLabel("Digital Signature Performance (ML-DSA-65)")
        section2.setStyleSheet(f"""
            font-size: 18px;
            font-weight: 600;
            color: {COLORS['accent_purple']};
            margin-top: 16px;
        """)
        scroll_layout.addWidget(section2)

        cards2 = QGridLayout()
        cards2.setSpacing(16)

        self.cards['mldsa_keygen'] = GlassCard(
            "Key Generation", 0, "µs",
            [COLORS['accent_purple'], COLORS['accent_pink']], "🔐"
        )
        cards2.addWidget(self.cards['mldsa_keygen'], 0, 0)

        self.cards['mldsa_sign'] = GlassCard(
            "Sign Operation", 0, "µs",
            [COLORS['accent_pink'], COLORS['accent_orange']], "✍️"
        )
        cards2.addWidget(self.cards['mldsa_sign'], 0, 1)

        self.cards['mldsa_verify'] = GlassCard(
            "Verify Operation", 0, "µs",
            [COLORS['accent_orange'], COLORS['accent_green']], "✅"
        )
        cards2.addWidget(self.cards['mldsa_verify'], 0, 2)

        self.cards['mldsa_roundtrip'] = GlassCard(
            "Round Trip", 0, "µs",
            [COLORS['accent_green'], COLORS['accent_cyan']], "🔄"
        )
        cards2.addWidget(self.cards['mldsa_roundtrip'], 0, 3)

        scroll_layout.addLayout(cards2)

        # Chart
        if MATPLOTLIB_AVAILABLE:
            chart_container = QFrame()
            chart_container.setStyleSheet(f"""
                QFrame {{
                    background: rgba(255, 255, 255, 0.02);
                    border: 1px solid rgba(255, 255, 255, 0.05);
                    border-radius: 20px;
                    padding: 20px;
                }}
            """)
            chart_layout = QVBoxLayout(chart_container)

            chart_title = QLabel("Performance Overview")
            chart_title.setStyleSheet(f"""
                font-size: 16px;
                font-weight: 600;
                color: {COLORS['text_primary']};
                margin-bottom: 12px;
            """)
            chart_layout.addWidget(chart_title)

            self.overview_canvas = PremiumCanvas(self, width=14, height=6)
            chart_layout.addWidget(self.overview_canvas)

            scroll_layout.addWidget(chart_container)

        scroll_layout.addStretch()
        scroll.setWidget(scroll_content)
        layout.addWidget(scroll)

        return widget

    def create_mlkem_tab(self):
        """ML-KEM deep dive tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Info card
        info = QFrame()
        info.setStyleSheet(f"""
            QFrame {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 rgba(0, 245, 255, 0.1),
                                           stop:1 rgba(59, 130, 246, 0.1));
                border: 1px solid rgba(0, 245, 255, 0.3);
                border-radius: 16px;
                padding: 24px;
            }}
        """)
        info_layout = QVBoxLayout(info)

        info_title = QLabel("🔐 ML-KEM-768 (NIST FIPS 203)")
        info_title.setStyleSheet(f"font-size: 20px; font-weight: 700; color: {COLORS['accent_cyan']};")
        info_layout.addWidget(info_title)

        info_text = QLabel(
            "Module-Lattice-Based Key-Encapsulation Mechanism\n"
            "• Security Level: NIST Level 3 (equivalent to AES-192)\n"
            "• Public Key: 1,184 bytes | Ciphertext: 1,088 bytes | Shared Secret: 32 bytes\n"
            "• Use Case: Secure key exchange for establishing session keys between ECUs"
        )
        info_text.setStyleSheet(f"color: {COLORS['text_secondary']}; font-size: 13px; line-height: 1.6;")
        info_text.setWordWrap(True)
        info_layout.addWidget(info_text)

        layout.addWidget(info)

        # Chart
        if MATPLOTLIB_AVAILABLE:
            self.mlkem_canvas = PremiumCanvas(self, width=14, height=7)
            layout.addWidget(self.mlkem_canvas)

        return widget

    def create_mldsa_tab(self):
        """ML-DSA deep dive tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Info card
        info = QFrame()
        info.setStyleSheet(f"""
            QFrame {{
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                           stop:0 rgba(168, 85, 247, 0.1),
                                           stop:1 rgba(236, 72, 153, 0.1));
                border: 1px solid rgba(168, 85, 247, 0.3);
                border-radius: 16px;
                padding: 24px;
            }}
        """)
        info_layout = QVBoxLayout(info)

        info_title = QLabel("✍️ ML-DSA-65 (NIST FIPS 204)")
        info_title.setStyleSheet(f"font-size: 20px; font-weight: 700; color: {COLORS['accent_purple']};")
        info_layout.addWidget(info_title)

        info_text = QLabel(
            "Module-Lattice-Based Digital Signature Algorithm\n"
            "• Security Level: NIST Level 3 (equivalent to AES-192)\n"
            "• Public Key: 1,952 bytes | Signature: ~3,309 bytes (vs 4 bytes MAC)\n"
            "• Use Case: Authenticate SecOC PDUs to ensure data integrity and origin"
        )
        info_text.setStyleSheet(f"color: {COLORS['text_secondary']}; font-size: 13px; line-height: 1.6;")
        info_text.setWordWrap(True)
        info_layout.addWidget(info_text)

        layout.addWidget(info)

        # Chart
        if MATPLOTLIB_AVAILABLE:
            self.mldsa_canvas = PremiumCanvas(self, width=14, height=7)
            layout.addWidget(self.mldsa_canvas)

        return widget

    def create_comparison_tab(self):
        """Comparison tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        if MATPLOTLIB_AVAILABLE:
            self.comparison_canvas = PremiumCanvas(self, width=14, height=8)
            layout.addWidget(self.comparison_canvas)

        return widget

    def create_eth_gateway_tab(self):
        """Ethernet Gateway use case"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Title
        title = QLabel("🌐 Ethernet Gateway Use Case - AUTOSAR SecOC with PQC")
        title.setStyleSheet(f"""
            font-size: 24px;
            font-weight: 700;
            color: {COLORS['accent_cyan']};
            margin-bottom: 16px;
        """)
        layout.addWidget(title)

        # Flow diagram (placeholder - will add detailed)
        flow_card = QFrame()
        flow_card.setStyleSheet(f"""
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 24px;
        """)
        flow_layout = QVBoxLayout(flow_card)

        flow_text = QLabel(
            "AUTOSAR SecOC ETH Gateway Flow:\n\n"
            "1. Session Establishment (ML-KEM-768)\n"
            "   Gateway A ←→ Gateway B: ML-KEM Key Exchange\n"
            "   Result: Shared Secret (32 bytes)\n\n"
            "2. Message Authentication (ML-DSA-65)\n"
            "   TX: AuthPDU + Freshness → ML-DSA Sign → SecuredPDU\n"
            "   RX: SecuredPDU → ML-DSA Verify → AuthPDU\n\n"
            "3. Performance Characteristics\n"
            "   Handshake Latency: ~200-400 µs\n"
            "   Per-Message Latency: ~900 µs (Sign + Verify)\n"
            "   Bandwidth Overhead: +3,317 bytes per message\n\n"
            "4. Deployment Considerations\n"
            "   ✓ Suitable for Ethernet (high bandwidth)\n"
            "   ✓ Use TP mode for fragmentation\n"
            "   ✓ Quantum-resistant security (30+ years)\n"
            "   ✗ Not suitable for CAN/CAN-FD"
        )
        flow_text.setStyleSheet(f"""
            color: {COLORS['text_secondary']};
            font-size: 14px;
            line-height: 1.8;
            font-family: 'Consolas', monospace;
        """)
        flow_text.setWordWrap(True)
        flow_layout.addWidget(flow_text)

        layout.addWidget(flow_card)
        layout.addStretch()

        return widget

    def create_metrics_tab(self):
        """Advanced metrics tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        self.metrics_display = QLabel()
        self.metrics_display.setStyleSheet(f"""
            background: rgba(255, 255, 255, 0.02);
            color: {COLORS['text_secondary']};
            padding: 24px;
            border-radius: 16px;
            font-family: 'Consolas', monospace;
            font-size: 12px;
            line-height: 1.6;
        """)
        self.metrics_display.setWordWrap(True)

        scroll = QScrollArea()
        scroll.setWidget(self.metrics_display)
        scroll.setWidgetResizable(True)
        layout.addWidget(scroll)

        return widget

    def load_default_data(self):
        """Load default CSV"""
        csv_file = Path("pqc_advanced_results.csv")
        if csv_file.exists():
            self.load_data(str(csv_file))
        else:
            QMessageBox.information(
                self,
                "No Data",
                "Please run the advanced test first:\n\n"
                "bash build_comprehensive_test.sh\n"
                "./test_pqc_comprehensive.exe"
            )

    def load_csv_file(self):
        """Load CSV dialog"""
        filename, _ = QFileDialog.getOpenFileName(
            self, "Load Performance Data", "", "CSV Files (*.csv)"
        )
        if filename:
            self.load_data(filename)

    def load_data(self, filename):
        """Load and process data"""
        try:
            with open(filename, 'r') as f:
                reader = csv.DictReader(f)
                self.data = list(reader)
            self.update_all_visualizations()
            QMessageBox.information(self, "Success", f"Loaded {len(self.data)} metrics")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load:\n{str(e)}")

    def update_all_visualizations(self):
        """Update all charts"""
        if not self.data:
            return

        self.update_cards()
        if MATPLOTLIB_AVAILABLE:
            self.plot_overview()
            self.plot_mlkem()
            self.plot_mldsa()
            self.plot_comparison()
        self.update_metrics_display()

    def update_cards(self):
        """Update metric cards"""
        for row in self.data:
            algo = row['Algorithm']
            op = row['Operation']
            avg = float(row['Avg(us)'])

            key = f"{algo.lower().replace('-', '').replace(' ', '_')}_{op.lower().replace(' ', '')}"

            if 'mlkem' in key and 'keygen' in key:
                self.cards['mlkem_keygen'].update_value(avg)
            elif 'mlkem' in key and 'encapsulate' in key:
                self.cards['mlkem_encap'].update_value(avg)
            elif 'mlkem' in key and 'decapsulate' in key:
                self.cards['mlkem_decap'].update_value(avg)
            elif 'mlkem' in key and 'handshake' in key:
                self.cards['mlkem_handshake'].update_value(avg / 1000)  # Convert to ms
            elif 'mldsa' in key and 'keygen' in key:
                self.cards['mldsa_keygen'].update_value(avg)
            elif 'mldsa' in key and 'sign' in key:
                self.cards['mldsa_sign'].update_value(avg)
            elif 'mldsa' in key and 'verify' in key:
                self.cards['mldsa_verify'].update_value(avg)
            elif 'mldsa' in key and 'roundtrip' in key:
                self.cards['mldsa_roundtrip'].update_value(avg)

    def plot_overview(self):
        """Plot overview with modern style"""
        self.overview_canvas.fig.clear()
        ax = self.overview_canvas.fig.add_subplot(111)

        operations = [row['Operation'] for row in self.data if 'HMAC' not in row['Algorithm']]
        avg_times = [float(row['Avg(us)']) for row in self.data if 'HMAC' not in row['Algorithm']]

        # Gradient colors
        colors = [COLORS['accent_cyan'], COLORS['accent_blue'], COLORS['accent_purple'],
                  COLORS['accent_pink'], COLORS['accent_purple'], COLORS['accent_orange'],
                  COLORS['accent_green'], COLORS['accent_cyan']]

        bars = ax.barh(operations, avg_times, color=colors[:len(operations)], alpha=0.9)
        ax.set_xlabel('Time (µs)', color='white', fontsize=12)
        ax.set_title('PQC Operations Performance', color='white', fontsize=16, fontweight='bold', pad=20)
        ax.set_facecolor('#0a0e27')
        ax.tick_params(colors='white')
        ax.spines['bottom'].set_color('#64748b')
        ax.spines['left'].set_color('#64748b')
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
        ax.grid(True, alpha=0.1, color='white')

        for bar, val in zip(bars, avg_times):
            ax.text(val, bar.get_y() + bar.get_height()/2, f'  {val:.1f} µs',
                   va='center', color='white', fontsize=10, fontweight='bold')

        self.overview_canvas.fig.tight_layout()
        self.overview_canvas.draw()

    def plot_mlkem(self):
        """Plot ML-KEM analysis"""
        self.mlkem_canvas.fig.clear()
        mlkem_data = [row for row in self.data if 'ML-KEM' in row['Algorithm']]

        ax1 = self.mlkem_canvas.fig.add_subplot(121)
        ax2 = self.mlkem_canvas.fig.add_subplot(122)

        operations = [row['Operation'] for row in mlkem_data]
        avg_times = [float(row['Avg(us)']) for row in mlkem_data]
        throughput = [float(row['Throughput(ops/sec)']) for row in mlkem_data]

        colors = [COLORS['accent_cyan'], COLORS['accent_blue'], COLORS['accent_purple'], COLORS['accent_pink']]

        ax1.bar(operations, avg_times, color=colors[:len(operations)], alpha=0.9)
        ax1.set_ylabel('Time (µs)', color='white', fontsize=12)
        ax1.set_title('ML-KEM-768 Latency', color='white', fontsize=14, fontweight='bold')
        ax1.set_facecolor('#0a0e27')
        ax1.tick_params(colors='white', labelsize=9)
        ax1.spines['bottom'].set_color('#64748b')
        ax1.spines['left'].set_color('#64748b')
        ax1.spines['top'].set_visible(False)
        ax1.spines['right'].set_visible(False)
        ax1.grid(True, alpha=0.1, axis='y')

        ax2.bar(operations, throughput, color=colors[:len(operations)], alpha=0.9)
        ax2.set_ylabel('Operations/Second', color='white', fontsize=12)
        ax2.set_title('ML-KEM-768 Throughput', color='white', fontsize=14, fontweight='bold')
        ax2.set_facecolor('#0a0e27')
        ax2.tick_params(colors='white', labelsize=9)
        ax2.spines['bottom'].set_color('#64748b')
        ax2.spines['left'].set_color('#64748b')
        ax2.spines['top'].set_visible(False)
        ax2.spines['right'].set_visible(False)
        ax2.grid(True, alpha=0.1, axis='y')

        self.mlkem_canvas.fig.tight_layout()
        self.mlkem_canvas.draw()

    def plot_mldsa(self):
        """Plot ML-DSA analysis"""
        self.mldsa_canvas.fig.clear()
        mldsa_data = [row for row in self.data if 'ML-DSA' in row['Algorithm']]

        ax1 = self.mldsa_canvas.fig.add_subplot(121)
        ax2 = self.mldsa_canvas.fig.add_subplot(122)

        operations = [row['Operation'] for row in mldsa_data]
        avg_times = [float(row['Avg(us)']) for row in mldsa_data]
        throughput = [float(row['Throughput(ops/sec)']) for row in mldsa_data]

        colors = [COLORS['accent_purple'], COLORS['accent_pink'], COLORS['accent_orange'], COLORS['accent_green']]

        ax1.bar(operations, avg_times, color=colors[:len(operations)], alpha=0.9)
        ax1.set_ylabel('Time (µs)', color='white', fontsize=12)
        ax1.set_title('ML-DSA-65 Latency', color='white', fontsize=14, fontweight='bold')
        ax1.set_facecolor('#0a0e27')
        ax1.tick_params(colors='white', labelsize=9)
        ax1.spines['bottom'].set_color('#64748b')
        ax1.spines['left'].set_color('#64748b')
        ax1.spines['top'].set_visible(False)
        ax1.spines['right'].set_visible(False)
        ax1.grid(True, alpha=0.1, axis='y')

        ax2.bar(operations, throughput, color=colors[:len(operations)], alpha=0.9)
        ax2.set_ylabel('Operations/Second', color='white', fontsize=12)
        ax2.set_title('ML-DSA-65 Throughput', color='white', fontsize=14, fontweight='bold')
        ax2.set_facecolor('#0a0e27')
        ax2.tick_params(colors='white', labelsize=9)
        ax2.spines['bottom'].set_color('#64748b')
        ax2.spines['left'].set_color('#64748b')
        ax2.spines['top'].set_visible(False)
        ax2.spines['right'].set_visible(False)
        ax2.grid(True, alpha=0.1, axis='y')

        self.mldsa_canvas.fig.tight_layout()
        self.mldsa_canvas.draw()

    def plot_comparison(self):
        """Plot PQC vs Classical"""
        self.comparison_canvas.fig.clear()

        # Get data
        mac_gen = mac_ver = mldsa_sign = mldsa_verify = None
        for row in self.data:
            if 'HMAC' in row['Algorithm'] and 'Generate' in row['Operation']:
                mac_gen = float(row['Avg(us)'])
            elif 'HMAC' in row['Algorithm'] and 'Verify' in row['Operation']:
                mac_ver = float(row['Avg(us)'])
            elif 'ML-DSA' in row['Algorithm'] and row['Operation'] == 'Sign':
                mldsa_sign = float(row['Avg(us)'])
            elif 'ML-DSA' in row['Algorithm'] and row['Operation'] == 'Verify':
                mldsa_verify = float(row['Avg(us)'])

        if not all([mac_gen, mac_ver, mldsa_sign, mldsa_verify]):
            return

        ax1 = self.comparison_canvas.fig.add_subplot(131)
        ax2 = self.comparison_canvas.fig.add_subplot(132)
        ax3 = self.comparison_canvas.fig.add_subplot(133)

        # Generation
        ax1.bar(['HMAC', 'ML-DSA'], [mac_gen, mldsa_sign],
               color=[COLORS['accent_orange'], COLORS['accent_purple']], alpha=0.9)
        ax1.set_ylabel('Time (µs)', color='white')
        ax1.set_title('Authentication Generation', color='white', fontweight='bold')
        ax1.set_facecolor('#0a0e27')
        ax1.tick_params(colors='white')
        overhead = mldsa_sign / mac_gen
        ax1.text(0.5, max(mac_gen, mldsa_sign) * 0.5, f'{overhead:.0f}x slower',
                ha='center', color=COLORS['warning'], fontsize=14, fontweight='bold')

        # Verification
        ax2.bar(['HMAC', 'ML-DSA'], [mac_ver, mldsa_verify],
               color=[COLORS['accent_orange'], COLORS['accent_purple']], alpha=0.9)
        ax2.set_ylabel('Time (µs)', color='white')
        ax2.set_title('Verification', color='white', fontweight='bold')
        ax2.set_facecolor('#0a0e27')
        ax2.tick_params(colors='white')

        # Size
        ax3.bar(['HMAC', 'ML-DSA'], [32, 3309],
               color=[COLORS['accent_orange'], COLORS['accent_purple']], alpha=0.9)
        ax3.set_ylabel('Size (bytes)', color='white')
        ax3.set_title('Authenticator Size', color='white', fontweight='bold')
        ax3.set_facecolor('#0a0e27')
        ax3.tick_params(colors='white')
        ax3.text(0.5, 1500, '103x larger', ha='center',
                color=COLORS['warning'], fontsize=14, fontweight='bold')

        for ax in [ax1, ax2, ax3]:
            ax.spines['bottom'].set_color('#64748b')
            ax.spines['left'].set_color('#64748b')
            ax.spines['top'].set_visible(False)
            ax.spines['right'].set_visible(False)
            ax.grid(True, alpha=0.1, axis='y')

        self.comparison_canvas.fig.tight_layout()
        self.comparison_canvas.draw()

    def update_metrics_display(self):
        """Update metrics text"""
        text = "ADVANCED PERFORMANCE METRICS\n" + "="*80 + "\n\n"

        for row in self.data:
            text += f"{row['Algorithm']} - {row['Operation']}\n"
            text += f"  Avg: {float(row['Avg(us)']):>10.3f} µs\n"
            text += f"  Min: {float(row['Min(us)']):>10.3f} µs\n"
            text += f"  Max: {float(row['Max(us)']):>10.3f} µs\n"
            if row['StdDev(us)']:
                text += f"  StdDev: {float(row['StdDev(us)']):>10.3f} µs\n"
            text += f"  Throughput: {float(row['Throughput(ops/sec)']):>10.0f} ops/sec\n"
            text += f"  Size: {row['Size(bytes)']} bytes\n"
            text += f"  Notes: {row['Notes']}\n\n"

        self.metrics_display.setText(text)

    def export_report(self):
        """Export report"""
        if not self.data:
            QMessageBox.warning(self, "No Data", "Load data first")
            return

        filename, _ = QFileDialog.getSaveFileName(
            self, "Export Report", "pqc_premium_report.txt", "Text Files (*.txt)"
        )
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.metrics_display.text())
                QMessageBox.information(self, "Success", f"Report exported to:\n{filename}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Export failed:\n{str(e)}")

def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')

    # Set dark palette
    palette = QPalette()
    palette.setColor(QPalette.Window, QColor("#0a0e27"))
    palette.setColor(QPalette.WindowText, QColor("#ffffff"))
    app.setPalette(palette)

    window = PQCPremiumDashboard()
    window.show()

    return app.exec()

if __name__ == '__main__':
    sys.exit(main())
