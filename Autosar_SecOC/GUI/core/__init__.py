"""Core package – backend bridge, data models, event bus, reactive state, topology."""

from .backend_bridge import BackendBridge
from .data_models import PduData, PqcKeyInfo, VerificationResult, PerfSample
from .signal_hub import hub
from .app_state import state
from .hw_configurator import hw_configurator, HardwareConfigurator
from .topology import (
    topology, TopologyModel, ECU_DOMAINS, MCU_CATALOGUE,
    ECUConfig, ZoneController, GatewayConfig, RouteConfig, ASIL_LEVELS,
)

__all__ = [
    "BackendBridge",
    "PduData",
    "PqcKeyInfo",
    "VerificationResult",
    "PerfSample",
    "hub",
    "state",
    "hw_configurator",
    "HardwareConfigurator",
    "topology",
    "TopologyModel",
    "ECU_DOMAINS",
    "MCU_CATALOGUE",
    "ECUConfig",
    "ZoneController",
    "GatewayConfig",
    "RouteConfig",
    "ASIL_LEVELS",
]
