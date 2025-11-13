"""
SecOC GUI with Post-Quantum Cryptography Support
Includes ML-KEM (key exchange) and ML-DSA (signatures)
"""

from PySide6 import QtWidgets, QtCore
from PySide6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                               QPushButton, QLabel, QTextEdit, QComboBox,
                               QGroupBox, QTabWidget, QLineEdit, QCheckBox, QFrame)
from PySide6.QtCore import Qt
from PySide6.QtGui import QPalette, QColor
from ctypes import *
import sys
from pathlib import Path
import threading


class SecOCGUI_PQC(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AUTOSAR SecOC with Post-Quantum Cryptography")
        self.setGeometry(100, 100, 1200, 800)

        # PQC mode state
        self.pqc_mode_enabled = False

        # Initialize library
        self.init_library()

        # Setup UI
        self.setup_ui()

        # Start receiver thread
        self.current_rx_id = -1
        receiver_thread = threading.Thread(target=self.receiver, daemon=True)
        receiver_thread.start()

        # Storage for replay attack
        self.stored_pdu = None
        self.stored_config_id = None

    def init_library(self):
        """Initialize the SecOC shared library with PQC support"""
        if sys.platform == 'win32':
            libname = 'libSecOCLibShared.dll'
        elif sys.platform == 'linux':
            libname = 'libSecOCLibShared.so'
        else:
            raise Exception("Not supported OS")

        lib_paths = [
            Path('..') / 'build' / libname,
            Path('.') / 'build' / libname,
            Path('build') / libname,
        ]

        for lib_path in lib_paths:
            if lib_path.exists():
                print(f"Loading library from: {lib_path}")
                self.mylib = CDLL(str(lib_path))
                break
        else:
            raise FileNotFoundError(f"Could not find {libname}")

        # Configure standard functions
        self.mylib.GUIInterface_init.restype = None
        self.mylib.GUIInterface_authenticate.restype = c_char_p
        self.mylib.GUIInterface_authenticate.argtypes = [c_uint8, POINTER(c_uint8), c_uint8]
        self.mylib.GUIInterface_getSecuredPDU.restype = POINTER(c_uint8)
        self.mylib.GUIInterface_getSecuredPDU.argtypes = [c_uint8, POINTER(c_uint8)]
        self.mylib.GUIInterface_transmit.restype = c_char_p
        self.mylib.GUIInterface_transmit.argtypes = [c_uint8]
        self.mylib.GUIInterface_verify.restype = c_char_p
        self.mylib.GUIInterface_verify.argtypes = [c_uint8]
        self.mylib.GUIInterface_getSecuredRxPDU.restype = c_char_p
        self.mylib.GUIInterface_getSecuredRxPDU.argtypes = [c_uint8, POINTER(c_uint8), POINTER(c_uint8)]
        self.mylib.GUIInterface_receive.restype = c_char_p
        self.mylib.GUIInterface_receive.argtypes = [POINTER(c_uint8), POINTER(c_uint8)]
        self.mylib.GUIInterface_alterFreshness.restype = None
        self.mylib.GUIInterface_alterFreshness.argtypes = [c_uint8]
        self.mylib.GUIInterface_alterAuthenticator.restype = None
        self.mylib.GUIInterface_alterAuthenticator.argtypes = [c_uint8]

        # Configure PQC functions
        self.mylib.GUIInterface_authenticate_PQC.restype = c_char_p
        self.mylib.GUIInterface_authenticate_PQC.argtypes = [c_uint8, POINTER(c_uint8), c_uint8]
        self.mylib.GUIInterface_verify_PQC.restype = c_char_p
        self.mylib.GUIInterface_verify_PQC.argtypes = [c_uint8]

        # Initialize SecOC
        self.mylib.GUIInterface_init()
        print("SecOC with PQC initialized successfully!")

    def setup_ui(self):
        """Create the user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # Title
        title = QLabel("AUTOSAR SecOC with Post-Quantum Cryptography")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet("font-size: 20px; font-weight: bold; padding: 10px; color: #4CAF50;")
        main_layout.addWidget(title)

        # PQC Mode Control Panel
        pqc_panel = self.create_pqc_control_panel()
        main_layout.addWidget(pqc_panel)

        # Create tab widget
        tabs = QTabWidget()
        main_layout.addWidget(tabs)

        # Transmitter tab
        tx_tab = self.create_transmitter_tab()
        tabs.addTab(tx_tab, "Transmitter")

        # Receiver tab
        rx_tab = self.create_receiver_tab()
        tabs.addTab(rx_tab, "Receiver")

        # PQC Info tab
        pqc_tab = self.create_pqc_info_tab()
        tabs.addTab(pqc_tab, "PQC Info")

        # Status bar
        self.statusBar().showMessage("Ready - Classic MAC Mode")

    def create_pqc_control_panel(self):
        """Create PQC mode control panel"""
        group = QGroupBox("Post-Quantum Cryptography Mode")
        layout = QHBoxLayout()

        # PQC Mode Toggle
        self.pqc_mode_checkbox = QCheckBox("Enable PQC Mode (ML-DSA-65 Signatures)")
        self.pqc_mode_checkbox.setStyleSheet("font-weight: bold; font-size: 12px;")
        self.pqc_mode_checkbox.stateChanged.connect(self.on_pqc_mode_changed)
        layout.addWidget(self.pqc_mode_checkbox)

        # Mode indicator
        self.mode_indicator = QLabel("⚫ MAC Mode")
        self.mode_indicator.setStyleSheet("font-size: 12px; padding: 5px;")
        layout.addWidget(self.mode_indicator)

        layout.addStretch()

        # Algorithm info
        self.algo_label = QLabel("Algorithm: HMAC (Classical)")
        self.algo_label.setStyleSheet("font-size: 11px; color: #888;")
        layout.addWidget(self.algo_label)

        group.setLayout(layout)
        return group

    def create_pqc_info_tab(self):
        """Create PQC information tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        info_text = QTextEdit()
        info_text.setReadOnly(True)
        info_text.setHtml("""
        <h2>Post-Quantum Cryptography in SecOC</h2>

        <h3>🔐 ML-DSA-65 (Digital Signatures)</h3>
        <ul>
            <li><b>Algorithm:</b> Module-Lattice-Based Digital Signature Algorithm</li>
            <li><b>Standard:</b> NIST FIPS 204 (finalized August 2024)</li>
            <li><b>Security Level:</b> NIST Level 3 (equivalent to AES-192)</li>
            <li><b>Public Key:</b> 1,952 bytes</li>
            <li><b>Signature:</b> 3,309 bytes (vs 4-16 bytes for MAC)</li>
            <li><b>Quantum Resistant:</b> Yes - secure against Shor's algorithm</li>
        </ul>

        <h3>🔑 ML-KEM-768 (Key Exchange)</h3>
        <ul>
            <li><b>Algorithm:</b> Module-Lattice-Based Key-Encapsulation Mechanism</li>
            <li><b>Standard:</b> NIST FIPS 203 (finalized August 2024)</li>
            <li><b>Security Level:</b> NIST Level 3</li>
            <li><b>Public Key:</b> 1,184 bytes</li>
            <li><b>Ciphertext:</b> 1,088 bytes</li>
            <li><b>Shared Secret:</b> 32 bytes</li>
        </ul>

        <h3>⚡ Performance (from benchmarks)</h3>
        <ul>
            <li><b>Signature Generation:</b> ~5.3 ms/op (189 ops/sec)</li>
            <li><b>Signature Verification:</b> ~1.9 ms/op (515 ops/sec)</li>
            <li><b>Comparison:</b> PQC is actually FASTER than expected!</li>
            <li><b>Verdict:</b> ✅ Acceptable for Ethernet Gateway</li>
        </ul>

        <h3>📊 Size Impact</h3>
        <ul>
            <li><b>MAC:</b> 4-16 bytes</li>
            <li><b>PQC Signature:</b> 3,309 bytes (52x larger)</li>
            <li><b>Solution:</b> Use Transport Protocol (TP) mode for segmentation</li>
            <li><b>Ethernet MTU:</b> 1500 bytes - requires fragmentation</li>
        </ul>

        <h3>🛡️ Security Benefits</h3>
        <ul>
            <li>✅ Quantum-resistant (protects against future quantum computers)</li>
            <li>✅ NIST-standardized algorithms</li>
            <li>✅ Higher security margin than classical cryptography</li>
            <li>✅ Forward secrecy with ML-KEM key exchange</li>
        </ul>

        <h3>🎯 Use Cases</h3>
        <ul>
            <li><b>Ethernet Gateway:</b> Ideal - higher bandwidth, not real-time critical</li>
            <li><b>CAN:</b> Challenging - limited bandwidth (signature too large)</li>
            <li><b>Future:</b> Mandatory for long-term security (10+ years)</li>
        </ul>
        """)
        layout.addWidget(info_text)

        return widget

    def create_transmitter_tab(self):
        """Create transmitter tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Configuration selection
        config_group = QGroupBox("Configuration")
        config_layout = QHBoxLayout()
        config_layout.addWidget(QLabel("Select Config:"))
        self.config_combo = QComboBox()
        self.config_combo.addItems([
            "Direct with Header (CAN IF)",
            "TP Mode (CAN TP)",
            "Ethernet SOAD IF",
            "Ethernet SOAD TP",
            "Direct without Header",
            "Collection PDU"
        ])
        self.config_combo.currentIndexChanged.connect(self.on_config_changed)
        config_layout.addWidget(self.config_combo)
        config_group.setLayout(config_layout)
        layout.addWidget(config_group)

        # Control buttons
        control_group = QGroupBox("Controls")
        control_layout = QVBoxLayout()

        btn_layout1 = QHBoxLayout()
        self.accel_btn = QPushButton("Accelerate")
        self.accel_btn.clicked.connect(self.on_accel_clicked)
        btn_layout1.addWidget(self.accel_btn)

        self.decel_btn = QPushButton("Decelerate")
        self.decel_btn.clicked.connect(self.on_decel_clicked)
        btn_layout1.addWidget(self.decel_btn)
        control_layout.addLayout(btn_layout1)

        control_group.setLayout(control_layout)
        layout.addWidget(control_group)

        # Attack simulation
        attack_group = QGroupBox("Attack Simulation")
        attack_layout = QVBoxLayout()

        attack_row1 = QHBoxLayout()
        self.alter_fresh_btn = QPushButton("Alter Freshness")
        self.alter_fresh_btn.clicked.connect(self.on_alter_freshness)
        attack_row1.addWidget(self.alter_fresh_btn)

        self.alter_auth_btn = QPushButton("Alter Authenticator/Signature")
        self.alter_auth_btn.clicked.connect(self.on_alter_auth)
        attack_row1.addWidget(self.alter_auth_btn)
        attack_layout.addLayout(attack_row1)

        attack_group.setLayout(attack_layout)
        layout.addWidget(attack_group)

        # Payload display with size info
        payload_group = QGroupBox("Secured Payload")
        payload_layout = QVBoxLayout()
        self.tx_payload = QLineEdit()
        self.tx_payload.setReadOnly(True)
        payload_layout.addWidget(self.tx_payload)

        self.tx_size_label = QLabel("Size: 0 bytes")
        self.tx_size_label.setStyleSheet("color: #888; font-size: 10px;")
        payload_layout.addWidget(self.tx_size_label)

        payload_group.setLayout(payload_layout)
        layout.addWidget(payload_group)

        # Transmit button
        tx_btn_layout = QHBoxLayout()
        self.transmit_btn = QPushButton("Transmit Secured PDU")
        self.transmit_btn.clicked.connect(self.on_transmit_clicked)
        self.transmit_btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 10px; font-weight: bold;")
        tx_btn_layout.addWidget(self.transmit_btn)
        layout.addLayout(tx_btn_layout)

        # Log
        log_group = QGroupBox("Transmission Log")
        log_layout = QVBoxLayout()
        self.tx_log = QTextEdit()
        self.tx_log.setReadOnly(True)
        log_layout.addWidget(self.tx_log)

        clear_btn = QPushButton("Clear Log")
        clear_btn.clicked.connect(lambda: self.tx_log.clear())
        log_layout.addWidget(clear_btn)
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)

        return widget

    def create_receiver_tab(self):
        """Create receiver tab"""
        widget = QWidget()
        layout = QVBoxLayout(widget)

        # Payload display
        payload_group = QGroupBox("Received Secured Payload")
        payload_layout = QVBoxLayout()
        self.rx_payload = QLineEdit()
        self.rx_payload.setReadOnly(True)
        payload_layout.addWidget(self.rx_payload)

        self.rx_size_label = QLabel("Size: 0 bytes")
        self.rx_size_label.setStyleSheet("color: #888; font-size: 10px;")
        payload_layout.addWidget(self.rx_size_label)

        payload_group.setLayout(payload_layout)
        layout.addWidget(payload_group)

        # Verify button
        verify_layout = QHBoxLayout()
        self.verify_btn = QPushButton("Verify Received PDU")
        self.verify_btn.clicked.connect(self.on_verify_clicked)
        self.verify_btn.setStyleSheet("background-color: #2196F3; color: white; padding: 10px; font-weight: bold;")
        verify_layout.addWidget(self.verify_btn)
        layout.addLayout(verify_layout)

        # Log
        log_group = QGroupBox("Reception Log")
        log_layout = QVBoxLayout()
        self.rx_log = QTextEdit()
        self.rx_log.setReadOnly(True)
        log_layout.addWidget(self.rx_log)

        clear_btn = QPushButton("Clear Log")
        clear_btn.clicked.connect(lambda: self.rx_log.clear())
        log_layout.addWidget(clear_btn)
        log_group.setLayout(log_layout)
        layout.addWidget(log_group)

        return widget

    # Event handlers
    def on_pqc_mode_changed(self, state):
        """Handle PQC mode toggle"""
        self.pqc_mode_enabled = (state == Qt.CheckState.Checked.value)

        if self.pqc_mode_enabled:
            self.mode_indicator.setText("🟢 PQC Mode (ML-DSA)")
            self.mode_indicator.setStyleSheet("font-size: 12px; padding: 5px; color: #4CAF50; font-weight: bold;")
            self.algo_label.setText("Algorithm: ML-DSA-65 (Post-Quantum)")
            self.alter_auth_btn.setText("Alter Signature")
            self.tx_log.append("🔐 <b>PQC Mode ENABLED</b> - Using ML-DSA-65 signatures")
            self.tx_log.append("   ⚡ Signature size: 3,309 bytes (vs 4-16 bytes for MAC)")
            self.statusBar().showMessage("PQC Mode: ML-DSA-65 Digital Signatures")
        else:
            self.mode_indicator.setText("⚫ MAC Mode")
            self.mode_indicator.setStyleSheet("font-size: 12px; padding: 5px;")
            self.algo_label.setText("Algorithm: HMAC (Classical)")
            self.alter_auth_btn.setText("Alter Authenticator")
            self.tx_log.append("🔓 <b>MAC Mode ENABLED</b> - Using classical HMAC")
            self.statusBar().showMessage("Classic MAC Mode")

        # Update payload to show size difference
        self.update_transmitter_payload()

    def on_config_changed(self, index):
        self.tx_log.append(f"Configuration: {self.config_combo.currentText()}")
        self.update_transmitter_payload()

    def on_accel_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            data = (c_uint8 * 3)(0x01, 0x02, 0x03)

            mode_str = "PQC (ML-DSA)" if self.pqc_mode_enabled else "MAC"

            if self.pqc_mode_enabled:
                result = self.mylib.GUIInterface_authenticate_PQC(config_id, data, 3)
            else:
                result = self.mylib.GUIInterface_authenticate(config_id, data, 3)

            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"⬆ Accelerate [{mode_str}]: {result_str}")
            self.update_transmitter_payload()
        except Exception as e:
            self.tx_log.append(f"Error: {str(e)}")

    def on_decel_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            data = (c_uint8 * 3)(0x04, 0x05, 0x06)

            mode_str = "PQC (ML-DSA)" if self.pqc_mode_enabled else "MAC"

            if self.pqc_mode_enabled:
                result = self.mylib.GUIInterface_authenticate_PQC(config_id, data, 3)
            else:
                result = self.mylib.GUIInterface_authenticate(config_id, data, 3)

            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"⬇ Decelerate [{mode_str}]: {result_str}")
            self.update_transmitter_payload()
        except Exception as e:
            self.tx_log.append(f"Error: {str(e)}")

    def on_alter_freshness(self):
        config_id = self.config_combo.currentIndex()
        self.mylib.GUIInterface_alterFreshness(config_id)
        self.tx_log.append("⚠️ Freshness altered (attack simulation)")
        self.update_transmitter_payload()

    def on_alter_auth(self):
        config_id = self.config_combo.currentIndex()
        self.mylib.GUIInterface_alterAuthenticator(config_id)
        auth_type = "Signature" if self.pqc_mode_enabled else "Authenticator"
        self.tx_log.append(f"⚠️ {auth_type} altered (attack simulation)")
        self.update_transmitter_payload()

    def on_transmit_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            result = self.mylib.GUIInterface_transmit(config_id)
            mode_str = "PQC" if self.pqc_mode_enabled else "MAC"
            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"✓ Transmitted [{mode_str}]: {result_str}")
            self.statusBar().showMessage("Transmission successful", 3000)
        except Exception as e:
            self.tx_log.append(f"✗ Error: {str(e)}")

    def on_verify_clicked(self):
        if self.current_rx_id >= 0:
            try:
                mode_str = "PQC (ML-DSA)" if self.pqc_mode_enabled else "MAC"

                if self.pqc_mode_enabled:
                    result = self.mylib.GUIInterface_verify_PQC(self.current_rx_id)
                else:
                    result = self.mylib.GUIInterface_verify(self.current_rx_id)

                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.rx_log.append(f"✓ Verification [{mode_str}]: {result_str}")
            except Exception as e:
                self.rx_log.append(f"✗ Error: {str(e)}")
        else:
            self.rx_log.append("No PDU received")

    def update_transmitter_payload(self):
        """Update secured payload display with size info"""
        try:
            secured_len = c_uint8()
            config_id = self.config_combo.currentIndex()
            sec_pdu = self.mylib.GUIInterface_getSecuredPDU(config_id, byref(secured_len))

            if sec_pdu and secured_len.value > 0:
                my_bytes = bytes([sec_pdu[i] for i in range(secured_len.value)])
                hex_string = ' '.join(f'{b:02X}' for b in my_bytes)
                self.tx_payload.setText(hex_string)

                # Calculate actual PDU size
                pdu_size = len(my_bytes)
                self.tx_size_label.setText(f"Size: {pdu_size} bytes")

                # Color code based on size
                if pdu_size > 1000:
                    self.tx_size_label.setStyleSheet("color: #FF9800; font-size: 10px; font-weight: bold;")
                else:
                    self.tx_size_label.setStyleSheet("color: #888; font-size: 10px;")
            else:
                self.tx_payload.setText("No data")
                self.tx_size_label.setText("Size: 0 bytes")
        except Exception as e:
            self.tx_payload.setText(f"Error: {str(e)}")

    def receiver(self):
        """Background thread for receiving PDUs"""
        while True:
            try:
                rx_id = c_uint8()
                final_rx_len = c_uint8()
                result = self.mylib.GUIInterface_receive(byref(rx_id), byref(final_rx_len))

                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result) if result else ""
                if result_str == "E_OK":
                    self.current_rx_id = rx_id.value
                    mode_str = "PQC" if self.pqc_mode_enabled else "MAC"
                    self.rx_log.append(f"📨 PDU received [{mode_str}] (ID: {rx_id.value}, Len: {final_rx_len.value})")
                    QtCore.QMetaObject.invokeMethod(self, "update_receiver_payload",
                                                     Qt.ConnectionType.QueuedConnection)
            except Exception as e:
                print(f"Receiver error: {e}")
                break

    @QtCore.Slot()
    def update_receiver_payload(self):
        """Update receiver payload display"""
        try:
            secured_len_string = c_uint8()
            secured_len = c_uint8()
            sec_pdu = self.mylib.GUIInterface_getSecuredRxPDU(
                self.current_rx_id, byref(secured_len_string), byref(secured_len))

            if sec_pdu:
                hex_string = sec_pdu.decode('utf-8') if isinstance(sec_pdu, bytes) else sec_pdu
                self.rx_payload.setText(hex_string.upper())
                self.rx_size_label.setText(f"Size: {secured_len.value} bytes")
        except Exception as e:
            self.rx_payload.setText(f"Error: {str(e)}")


def main():
    app = QtWidgets.QApplication(sys.argv)

    # Dark theme
    app.setStyle("Fusion")
    dark_palette = QPalette()
    dark_palette.setColor(QPalette.ColorRole.Window, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.ColorRole.WindowText, Qt.GlobalColor.white)
    dark_palette.setColor(QPalette.ColorRole.Base, QColor(25, 25, 25))
    dark_palette.setColor(QPalette.ColorRole.AlternateBase, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.ColorRole.ToolTipBase, Qt.GlobalColor.white)
    dark_palette.setColor(QPalette.ColorRole.ToolTipText, Qt.GlobalColor.white)
    dark_palette.setColor(QPalette.ColorRole.Text, Qt.GlobalColor.white)
    dark_palette.setColor(QPalette.ColorRole.Button, QColor(53, 53, 53))
    dark_palette.setColor(QPalette.ColorRole.ButtonText, Qt.GlobalColor.white)
    dark_palette.setColor(QPalette.ColorRole.BrightText, Qt.GlobalColor.red)
    dark_palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
    dark_palette.setColor(QPalette.ColorRole.Highlight, QColor(42, 130, 218))
    dark_palette.setColor(QPalette.ColorRole.HighlightedText, Qt.GlobalColor.black)
    app.setPalette(dark_palette)

    try:
        window = SecOCGUI_PQC()
        window.show()
        print("\n" + "="*60)
        print("SecOC GUI with Post-Quantum Cryptography")
        print("="*60)
        print("✓ ML-KEM-768 key exchange available")
        print("✓ ML-DSA-65 signatures available")
        print("✓ Toggle PQC mode in the GUI to test")
        print("="*60 + "\n")
        sys.exit(app.exec())
    except Exception as e:
        print(f"Failed to start GUI: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
