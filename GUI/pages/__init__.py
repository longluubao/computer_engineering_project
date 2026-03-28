"""Application pages - each is a standalone QWidget panel."""

from .dashboard import DashboardPage
from .routing import RoutingPage
from .secoc_config import SecOCConfigPage
from .diagnostics import DiagnosticsPage
from .hardware_config import HardwareConfigPage

__all__ = [
    "DashboardPage",
    "RoutingPage",
    "SecOCConfigPage",
    "DiagnosticsPage",
    "HardwareConfigPage",
]
