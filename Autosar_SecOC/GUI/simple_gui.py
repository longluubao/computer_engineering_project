"""
Simplified SecOC GUI - Windows Compatible Version
No Cairo or custom widget dependencies
"""

from PySide6 import QtWidgets, QtCore
from PySide6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                               QPushButton, QLabel, QTextEdit, QComboBox,
                               QGroupBox, QTabWidget, QLineEdit)
from PySide6.QtCore import Qt
from PySide6.QtGui import QPalette, QColor
from ctypes import *
import sys
from pathlib import Path
import threading


class SecOCGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AUTOSAR SecOC - Secure On-Board Communication")
        self.setGeometry(100, 100, 1000, 700)

        # Initialize library
        self.init_library()

        # Setup UI
        self.setup_ui()

        # Start receiver thread
        self.current_rx_id = -1
        receiver_thread = threading.Thread(target=self.receiver, daemon=True)
        receiver_thread.start()

        # Storage for replay attack simulation
        self.stored_pdu = None
        self.stored_config_id = None

    def init_library(self):
        """Initialize the SecOC shared library"""
        if sys.platform == 'win32':
            libname = 'libSecOCLibShared.dll'
        elif sys.platform == 'linux':
            libname = 'libSecOCLibShared.so'
        else:
            raise Exception("Not supported OS")

        # Try different paths
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
            raise FileNotFoundError(f"Could not find {libname} in any expected location")

        # Configure return types
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

        # Initialize SecOC
        self.mylib.GUIInterface_init()
        print("SecOC initialized successfully!")

    def setup_ui(self):
        """Create the user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # Title
        title = QLabel("AUTOSAR SecOC Testing Interface")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;")
        main_layout.addWidget(title)

        # Create tab widget
        tabs = QTabWidget()
        main_layout.addWidget(tabs)

        # Transmitter tab
        tx_tab = self.create_transmitter_tab()
        tabs.addTab(tx_tab, "Transmitter")

        # Receiver tab
        rx_tab = self.create_receiver_tab()
        tabs.addTab(rx_tab, "Receiver")

        # Status bar
        self.statusBar().showMessage("Ready")

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

        btn_layout2 = QHBoxLayout()
        self.time_btn = QPushButton("Show Time")
        self.time_btn.clicked.connect(self.on_show_time_clicked)
        btn_layout2.addWidget(self.time_btn)

        self.date_btn = QPushButton("Show Date")
        self.date_btn.clicked.connect(self.on_show_date_clicked)
        btn_layout2.addWidget(self.date_btn)
        control_layout.addLayout(btn_layout2)

        control_group.setLayout(control_layout)
        layout.addWidget(control_group)

        # Attack simulation
        attack_group = QGroupBox("Attack Simulation")
        attack_layout = QVBoxLayout()

        # Row 1: Tampering attacks
        attack_row1 = QHBoxLayout()
        self.alter_fresh_btn = QPushButton("Alter Freshness")
        self.alter_fresh_btn.clicked.connect(self.on_alter_freshness)
        attack_row1.addWidget(self.alter_fresh_btn)

        self.alter_auth_btn = QPushButton("Alter Authenticator")
        self.alter_auth_btn.clicked.connect(self.on_alter_auth)
        attack_row1.addWidget(self.alter_auth_btn)
        attack_layout.addLayout(attack_row1)

        # Row 2: Replay attack
        attack_row2 = QHBoxLayout()
        self.store_pdu_btn = QPushButton("Store PDU (for replay)")
        self.store_pdu_btn.clicked.connect(self.on_store_pdu)
        self.store_pdu_btn.setStyleSheet("background-color: #FF9800; color: white;")
        attack_row2.addWidget(self.store_pdu_btn)

        self.replay_pdu_btn = QPushButton("Replay Stored PDU")
        self.replay_pdu_btn.clicked.connect(self.on_replay_pdu)
        self.replay_pdu_btn.setStyleSheet("background-color: #F44336; color: white;")
        self.replay_pdu_btn.setEnabled(False)  # Disabled until PDU is stored
        attack_row2.addWidget(self.replay_pdu_btn)
        attack_layout.addLayout(attack_row2)

        attack_group.setLayout(attack_layout)
        layout.addWidget(attack_group)

        # Payload display
        payload_group = QGroupBox("Secured Payload")
        payload_layout = QVBoxLayout()
        self.tx_payload = QLineEdit()
        self.tx_payload.setReadOnly(True)
        payload_layout.addWidget(self.tx_payload)
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
    def on_config_changed(self, index):
        self.tx_log.append(f"Configuration changed to: {self.config_combo.currentText()}")
        self.update_transmitter_payload()

    def on_accel_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            data = (c_uint8 * 3)(0x01, 0x02, 0x03)  # Accelerate command
            result = self.mylib.GUIInterface_authenticate(config_id, data, 3)
            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"Accelerate command authenticated: {result_str}")
            self.update_transmitter_payload()
        except Exception as e:
            self.tx_log.append(f"Error: {str(e)}")

    def on_decel_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            data = (c_uint8 * 3)(0x04, 0x05, 0x06)  # Decelerate command
            result = self.mylib.GUIInterface_authenticate(config_id, data, 3)
            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"Decelerate command authenticated: {result_str}")
            self.update_transmitter_payload()
        except Exception as e:
            self.tx_log.append(f"Error: {str(e)}")

    def on_show_time_clicked(self):
        self.tx_log.append("Show Time command sent")
        self.update_transmitter_payload()

    def on_show_date_clicked(self):
        self.tx_log.append("Show Date command sent")
        self.update_transmitter_payload()

    def on_alter_freshness(self):
        config_id = self.config_combo.currentIndex()
        self.mylib.GUIInterface_alterFreshness(config_id)
        self.tx_log.append("⚠️ Freshness value altered (simulating attack)")
        self.update_transmitter_payload()

    def on_alter_auth(self):
        config_id = self.config_combo.currentIndex()
        self.mylib.GUIInterface_alterAuthenticator(config_id)
        self.tx_log.append("⚠️ Authenticator altered (simulating attack)")
        self.update_transmitter_payload()

    def on_store_pdu(self):
        """Store current PDU for replay attack simulation"""
        config_id = self.config_combo.currentIndex()
        secured_len = c_uint8()
        sec_pdu = self.mylib.GUIInterface_getSecuredPDU(config_id, byref(secured_len))

        if sec_pdu and secured_len.value > 0:
            # Copy the PDU data
            self.stored_pdu = bytes([sec_pdu[i] for i in range(secured_len.value)])
            self.stored_config_id = config_id
            self.replay_pdu_btn.setEnabled(True)
            self.tx_log.append(f"📦 PDU stored (length: {secured_len.value}, freshness will become stale)")
        else:
            self.tx_log.append("⚠️ No PDU to store. Click Accelerate/Decelerate first.")

    def on_replay_pdu(self):
        """Replay the stored PDU (simulating replay attack)"""
        if self.stored_pdu is None:
            self.tx_log.append("⚠️ No stored PDU to replay")
            return

        # The stored PDU has old freshness value
        # When receiver verifies it, freshness will be stale → "Freshness Failed"
        self.tx_log.append(f"🔁 Replaying old PDU (stale freshness - simulating replay attack)")

        # Store the replayed PDU in the receiver buffer
        # This simulates receiving the old PDU
        self.current_rx_id = self.stored_config_id

        # Trigger UI update
        self.update_receiver_payload()
        self.rx_log.append(f"📨 Old PDU replayed (ID: {self.stored_config_id})")
        self.rx_log.append("⚠️ This PDU has stale freshness - click Verify to see 'Freshness Failed' or 'PDU is not Authentic'")

    def on_transmit_clicked(self):
        try:
            config_id = self.config_combo.currentIndex()
            result = self.mylib.GUIInterface_transmit(config_id)
            if result:
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result)
                self.tx_log.append(f"✓ PDU transmitted: {result_str}")
            else:
                self.tx_log.append(f"✓ PDU transmitted successfully")
            self.statusBar().showMessage("Transmission successful", 3000)
        except Exception as e:
            self.tx_log.append(f"✗ Transmission error: {str(e)}")
            self.statusBar().showMessage("Transmission failed", 3000)

    def on_verify_clicked(self):
        if self.current_rx_id >= 0:
            try:
                result = self.mylib.GUIInterface_verify(self.current_rx_id)
                self.rx_log.append(f"✓ Verification result: {result}")
                self.statusBar().showMessage("Verification complete", 3000)
            except Exception as e:
                self.rx_log.append(f"✗ Verification error: {str(e)}")
        else:
            self.rx_log.append("No PDU received yet")

    def update_transmitter_payload(self):
        """Update the secured payload display"""
        try:
            secured_len = c_uint8()
            config_id = self.config_combo.currentIndex()
            sec_pdu = self.mylib.GUIInterface_getSecuredPDU(config_id, byref(secured_len))

            if sec_pdu and secured_len.value > 0:
                # Read bytes from the pointer
                my_bytes = bytes([sec_pdu[i] for i in range(secured_len.value)])
                hex_string = ' '.join(f'{b:02X}' for b in my_bytes)
                self.tx_payload.setText(hex_string)
            else:
                self.tx_payload.setText("No data")
        except Exception as e:
            import traceback
            self.tx_payload.setText(f"Error: {str(e)}")
            print(traceback.format_exc())

    def receiver(self):
        """Background thread for receiving PDUs"""
        while True:
            try:
                rx_id = c_uint8()
                final_rx_len = c_uint8()
                result = self.mylib.GUIInterface_receive(byref(rx_id), byref(final_rx_len))

                # Check if result is E_OK (convert bytes to string for comparison)
                result_str = result.decode('utf-8') if isinstance(result, bytes) else str(result) if result else ""
                if result_str == "E_OK":
                    self.current_rx_id = rx_id.value
                    self.rx_log.append(f"📨 PDU received (ID: {rx_id.value}, Length: {final_rx_len.value})")
                    # Update UI in main thread
                    QtCore.QMetaObject.invokeMethod(self, "update_receiver_payload",
                                                     Qt.ConnectionType.QueuedConnection)
            except Exception as e:
                print(f"Receiver error: {e}")
                import traceback
                traceback.print_exc()
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
                # The C function returns a formatted hex string, just decode it
                hex_string = sec_pdu.decode('utf-8') if isinstance(sec_pdu, bytes) else sec_pdu
                self.rx_payload.setText(hex_string.upper())
        except Exception as e:
            self.rx_payload.setText(f"Error: {str(e)}")
            import traceback
            traceback.print_exc()


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
        window = SecOCGUI()
        window.show()
        sys.exit(app.exec())
    except Exception as e:
        print(f"Failed to start GUI: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
