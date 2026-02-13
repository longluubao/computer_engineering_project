#!/usr/bin/env python3
"""
AUTOSAR SecOC — PQC Automotive Dashboard
=========================================

Entry point.  Loads the C shared library, initialises SecOC,
and launches the Qt main window.

Usage
-----
    cd Autosar_SecOC/GUI
    python main.py                  # Normal launch (loads DLL)
    python main.py --no-backend     # UI-only mode (no DLL required)
"""

from __future__ import annotations

import argparse
import os
import sys

# Ensure the GUI package root is on sys.path so relative imports work
_gui_dir = os.path.dirname(os.path.abspath(__file__))
if _gui_dir not in sys.path:
    sys.path.insert(0, _gui_dir)

from PySide6.QtWidgets import QApplication, QMessageBox
from PySide6.QtCore import Qt

from config.theme import Theme
from config.settings import Settings
from core.backend_bridge import BackendBridge
from main_window import MainWindow


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="AUTOSAR SecOC PQC Automotive Dashboard"
    )
    parser.add_argument(
        "--no-backend",
        action="store_true",
        help="Launch in UI-only mode without loading the C shared library",
    )
    parser.add_argument(
        "--dll",
        type=str,
        default=None,
        help="Override path to SecOCLibShared DLL / SO",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    # ── Qt application ───────────────────────────────────────────────
    app = QApplication(sys.argv)
    app.setApplicationName(Settings.APP_NAME)
    app.setApplicationVersion(Settings.APP_VERSION)

    # High-DPI support
    app.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    # Apply global theme
    Theme.apply_global(app)

    # ── Backend bridge ───────────────────────────────────────────────
    bridge = BackendBridge()

    if not args.no_backend:
        try:
            bridge.load(args.dll)
            bridge.init()
        except OSError as exc:
            QMessageBox.warning(
                None,
                "Backend Not Found",
                f"Could not load the SecOC shared library:\n\n{exc}\n\n"
                "The application will run in UI-only mode.\n"
                "Build the project first:  cd Autosar_SecOC && bash rebuild_pqc.sh",
            )
        except Exception as exc:
            QMessageBox.critical(
                None,
                "Initialisation Error",
                f"An error occurred during SecOC initialisation:\n\n{exc}",
            )

    # ── Main window ──────────────────────────────────────────────────
    window = MainWindow(bridge)
    window.show()

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())

