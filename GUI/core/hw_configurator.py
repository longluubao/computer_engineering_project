"""
Hardware Configurator — platform-aware backend for real RPi configuration.

On Raspberry Pi: executes real commands (SPI, CAN, GPIO, network).
On Windows/non-RPi: simulation mode — logs commands but doesn't execute.
"""

from __future__ import annotations

import os
import platform
import re
import subprocess
from pathlib import Path

from PySide6.QtCore import QObject, Signal

try:
    import RPi.GPIO as _RPIGPIO  # type: ignore
except Exception:  # pragma: no cover - optional dependency
    _RPIGPIO = None


# ── Input validation helpers ─────────────────────────────────────────

_RE_IP = re.compile(
    r"^(?:(?:25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(?:25[0-5]|2[0-4]\d|[01]?\d\d?)$"
)
_RE_NETMASK = _RE_IP  # same format
_VALID_BITRATES = {125000, 250000, 500000, 1000000}
_VALID_CAN_IFACES = {"can0", "can1", "vcan0", "vcan1"}
_VALID_NET_IFACES = {"eth0", "eth1", "wlan0", "wlan1", "enp0s3"}
_VALID_DIRECTIONS = {"in", "out"}


def _validate_ip(ip: str) -> bool:
    return bool(_RE_IP.match(ip))


def _validate_iface(name: str, allowed: set[str]) -> bool:
    return name in allowed


def _validate_bcm(bcm: int) -> bool:
    return 0 <= bcm <= 27


class HardwareConfigurator(QObject):
    """Singleton backend for hardware configuration commands."""

    # Signal: (operation_name, success, message)
    operation_completed = Signal(str, bool, str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._is_rpi: bool | None = None
        self._gpio_backend = "sysfs"
        self._gpio_directions: dict[int, str] = {}
        self._sim_state = {
            "spi": True,
            "i2c": False,
            "can": {
                "can0": {"state": "down", "bitrate": 0},
                "can1": {"state": "down", "bitrate": 0},
            },
            "net": {
                "eth0": {"ip": "192.168.1.1", "state": "up"},
                "eth1": {"ip": "N/A", "state": "down"},
                "wlan0": {"ip": "192.168.1.10", "state": "up"},
                "wlan1": {"ip": "N/A", "state": "down"},
            },
            "gpio_exported": set(),
            "gpio_value": {},
            "gpio_direction": {},
        }

    # ── Platform detection ────────────────────────────────────────────

    @property
    def is_raspberry_pi(self) -> bool:
        if self._is_rpi is None:
            self._is_rpi = False
            if platform.system() == "Linux":
                try:
                    cpuinfo = Path("/proc/cpuinfo").read_text()
                    if "BCM" in cpuinfo or "Raspberry" in cpuinfo:
                        self._is_rpi = True
                except OSError:
                    pass
        return self._is_rpi

    @property
    def simulation_mode(self) -> bool:
        return not self.is_raspberry_pi

    def _ensure_gpio_backend(self) -> None:
        if self.simulation_mode:
            return
        if _RPIGPIO is None:
            self._gpio_backend = "sysfs"
            return
        if self._gpio_backend == "rpigpio":
            return
        try:
            _RPIGPIO.setwarnings(False)
            _RPIGPIO.setmode(_RPIGPIO.BCM)
            self._gpio_backend = "rpigpio"
        except Exception:
            self._gpio_backend = "sysfs"

    # ── Command runner ────────────────────────────────────────────────

    def _run(
        self, cmd: list[str], sudo: bool = False
    ) -> tuple[bool, str, str]:
        """Run a command. In simulation mode, just log it."""
        if sudo:
            cmd = ["sudo"] + cmd

        cmd_str = " ".join(cmd)

        if self.simulation_mode:
            msg = f"[SIM] Would execute: {cmd_str}"
            print(msg)
            return True, msg, ""

        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=10
            )
            success = result.returncode == 0
            return success, result.stdout.strip(), result.stderr.strip()
        except subprocess.TimeoutExpired:
            return False, "", "Command timed out"
        except FileNotFoundError:
            return False, "", f"Command not found: {cmd[0]}"
        except OSError as e:
            return False, "", str(e)

    def _emit(self, op: str, success: bool, msg: str) -> None:
        self.operation_completed.emit(op, success, msg)

    # ── SPI ───────────────────────────────────────────────────────────

    def spi_enabled(self) -> bool:
        """Check if SPI devices exist."""
        if self.simulation_mode:
            return bool(self._sim_state["spi"])
        return bool(list(Path("/dev").glob("spidev*")))

    def enable_spi(self) -> None:
        """Enable SPI interface."""
        ok1, out1, err1 = self._run(["modprobe", "spi_bcm2835"], sudo=True)
        # Also try raspi-config nonint
        ok2, out2, err2 = self._run(
            ["raspi-config", "nonint", "do_spi", "0"], sudo=True
        )
        success = ok1 or ok2
        if self.simulation_mode:
            self._sim_state["spi"] = True
        msg = out1 or out2 or err1 or err2 or "SPI enabled"
        self._emit("enable_spi", success, msg)

    def disable_spi(self) -> None:
        """Disable SPI interface."""
        ok, out, err = self._run(
            ["raspi-config", "nonint", "do_spi", "1"], sudo=True
        )
        if self.simulation_mode and ok:
            self._sim_state["spi"] = False
        self._emit("disable_spi", ok, out or err or "SPI disabled")

    def load_mcp2515_overlay(
        self, osc: int = 16000000, int_pin: int = 25
    ) -> None:
        """Load MCP2515 CAN controller device tree overlay."""
        ok, out, err = self._run(
            [
                "dtoverlay", "mcp2515-can0",
                f"oscillator={osc}", f"interrupt={int_pin}",
            ],
            sudo=True,
        )
        self._emit("load_mcp2515", ok, out or err or "MCP2515 overlay loaded")

    # ── I2C ───────────────────────────────────────────────────────────

    def i2c_enabled(self) -> bool:
        if self.simulation_mode:
            return bool(self._sim_state["i2c"])
        return Path("/dev/i2c-1").exists()

    def enable_i2c(self) -> None:
        ok, out, err = self._run(
            ["raspi-config", "nonint", "do_i2c", "0"], sudo=True
        )
        if self.simulation_mode and ok:
            self._sim_state["i2c"] = True
        self._emit("enable_i2c", ok, out or err or "I2C enabled")

    def disable_i2c(self) -> None:
        ok, out, err = self._run(
            ["raspi-config", "nonint", "do_i2c", "1"], sudo=True
        )
        if self.simulation_mode and ok:
            self._sim_state["i2c"] = False
        self._emit("disable_i2c", ok, out or err or "I2C disabled")

    # ── CAN ───────────────────────────────────────────────────────────

    def can_status(self, iface: str = "can0") -> dict:
        """Get CAN interface status. Returns {state, bitrate}."""
        if not _validate_iface(iface, _VALID_CAN_IFACES):
            return {"state": "invalid", "bitrate": 0}

        if self.simulation_mode:
            return dict(self._sim_state["can"].get(iface, {"state": "down", "bitrate": 0}))

        ok, out, _ = self._run(["ip", "-details", "link", "show", iface])
        if not ok:
            return {"state": "not_found", "bitrate": 0}

        state = "down"
        bitrate = 0
        if "UP" in out or "state UP" in out:
            state = "up"
        m = re.search(r"bitrate\s+(\d+)", out)
        if m:
            bitrate = int(m.group(1))
        return {"state": state, "bitrate": bitrate}

    def can_up(self, iface: str = "can0", bitrate: int = 500000) -> None:
        """Bring CAN interface up with specified bitrate."""
        if not _validate_iface(iface, _VALID_CAN_IFACES):
            self._emit("can_up", False, f"Invalid interface: {iface}")
            return
        if bitrate not in _VALID_BITRATES:
            self._emit("can_up", False, f"Invalid bitrate: {bitrate}")
            return

        # Set type and bitrate
        ok1, out1, err1 = self._run(
            ["ip", "link", "set", iface, "type", "can", "bitrate", str(bitrate)],
            sudo=True,
        )
        # Bring up
        ok2, out2, err2 = self._run(
            ["ip", "link", "set", iface, "up"], sudo=True
        )
        success = ok1 and ok2
        if self.simulation_mode and success:
            self._sim_state["can"][iface] = {"state": "up", "bitrate": bitrate}
        msg = f"{iface} up at {bitrate} bps" if success else (err1 or err2)
        self._emit("can_up", success, msg)

    def can_down(self, iface: str = "can0") -> None:
        """Bring CAN interface down."""
        if not _validate_iface(iface, _VALID_CAN_IFACES):
            self._emit("can_down", False, f"Invalid interface: {iface}")
            return
        ok, out, err = self._run(
            ["ip", "link", "set", iface, "down"], sudo=True
        )
        if self.simulation_mode and ok:
            curr = self._sim_state["can"].get(iface, {"state": "down", "bitrate": 0})
            self._sim_state["can"][iface] = {"state": "down", "bitrate": curr.get("bitrate", 0)}
        self._emit("can_down", ok, out or err or f"{iface} down")

    # ── GPIO (sysfs) ──────────────────────────────────────────────────

    def gpio_export(self, bcm: int) -> None:
        if not _validate_bcm(bcm):
            self._emit("gpio_export", False, f"Invalid BCM pin: {bcm}")
            return
        if self.simulation_mode:
            self._sim_state["gpio_exported"].add(bcm)
            self._emit("gpio_export", True, f"[SIM] Exported GPIO{bcm}")
            return
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            self._gpio_directions.setdefault(bcm, "in")
            self._emit("gpio_export", True, f"Prepared GPIO{bcm} (RPi.GPIO)")
            return
        try:
            Path("/sys/class/gpio/export").write_text(str(bcm))
            self._emit("gpio_export", True, f"Exported GPIO{bcm}")
        except OSError as e:
            self._emit("gpio_export", False, str(e))

    def gpio_unexport(self, bcm: int) -> None:
        if not _validate_bcm(bcm):
            self._emit("gpio_unexport", False, f"Invalid BCM pin: {bcm}")
            return
        if self.simulation_mode:
            self._sim_state["gpio_exported"].discard(bcm)
            self._emit("gpio_unexport", True, f"[SIM] Unexported GPIO{bcm}")
            return
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            try:
                _RPIGPIO.cleanup(bcm)
            except Exception:
                pass
            self._gpio_directions.pop(bcm, None)
            self._emit("gpio_unexport", True, f"Released GPIO{bcm} (RPi.GPIO)")
            return
        try:
            Path("/sys/class/gpio/unexport").write_text(str(bcm))
            self._emit("gpio_unexport", True, f"Unexported GPIO{bcm}")
        except OSError as e:
            self._emit("gpio_unexport", False, str(e))

    def gpio_set_direction(self, bcm: int, direction: str) -> None:
        if not _validate_bcm(bcm):
            self._emit("gpio_direction", False, f"Invalid BCM pin: {bcm}")
            return
        if direction not in _VALID_DIRECTIONS:
            self._emit("gpio_direction", False, f"Invalid direction: {direction}")
            return
        if self.simulation_mode:
            self._sim_state["gpio_direction"][bcm] = direction
            self._emit(
                "gpio_direction", True,
                f"[SIM] GPIO{bcm} direction -> {direction}",
            )
            return
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            try:
                mode = _RPIGPIO.IN if direction == "in" else _RPIGPIO.OUT
                _RPIGPIO.setup(bcm, mode)
                self._gpio_directions[bcm] = direction
                self._emit("gpio_direction", True, f"GPIO{bcm} -> {direction}")
            except Exception as e:
                self._emit("gpio_direction", False, str(e))
            return
        try:
            Path(f"/sys/class/gpio/gpio{bcm}/direction").write_text(direction)
            self._emit("gpio_direction", True, f"GPIO{bcm} -> {direction}")
        except OSError as e:
            self._emit("gpio_direction", False, str(e))

    def gpio_read(self, bcm: int) -> int:
        if not _validate_bcm(bcm):
            return 0
        if self.simulation_mode:
            return int(self._sim_state["gpio_value"].get(bcm, 0))
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            try:
                if self._gpio_directions.get(bcm) != "in":
                    _RPIGPIO.setup(bcm, _RPIGPIO.IN)
                    self._gpio_directions[bcm] = "in"
                return int(_RPIGPIO.input(bcm))
            except Exception:
                return 0
        try:
            val = Path(f"/sys/class/gpio/gpio{bcm}/value").read_text().strip()
            return int(val)
        except (OSError, ValueError):
            return 0

    def gpio_write(self, bcm: int, value: int) -> None:
        if not _validate_bcm(bcm):
            self._emit("gpio_write", False, f"Invalid BCM pin: {bcm}")
            return
        val = "1" if value else "0"
        if self.simulation_mode:
            self._sim_state["gpio_value"][bcm] = int(value)
            self._emit("gpio_write", True, f"[SIM] GPIO{bcm} = {val}")
            return
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            try:
                if self._gpio_directions.get(bcm) != "out":
                    _RPIGPIO.setup(bcm, _RPIGPIO.OUT)
                    self._gpio_directions[bcm] = "out"
                _RPIGPIO.output(bcm, int(value))
                self._emit("gpio_write", True, f"GPIO{bcm} = {val}")
            except Exception as e:
                self._emit("gpio_write", False, str(e))
            return
        try:
            Path(f"/sys/class/gpio/gpio{bcm}/value").write_text(val)
            self._emit("gpio_write", True, f"GPIO{bcm} = {val}")
        except OSError as e:
            self._emit("gpio_write", False, str(e))

    def gpio_is_exported(self, bcm: int) -> bool:
        if not _validate_bcm(bcm):
            return False
        if self.simulation_mode:
            return bcm in self._sim_state["gpio_exported"]
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            return bcm in self._gpio_directions
        return Path(f"/sys/class/gpio/gpio{bcm}").exists()

    def gpio_get_direction(self, bcm: int) -> str:
        if not _validate_bcm(bcm):
            return "in"
        if self.simulation_mode:
            return str(self._sim_state["gpio_direction"].get(bcm, "in"))
        self._ensure_gpio_backend()
        if self._gpio_backend == "rpigpio":
            return self._gpio_directions.get(bcm, "in")
        try:
            return Path(f"/sys/class/gpio/gpio{bcm}/direction").read_text().strip()
        except OSError:
            return "in"

    # ── Network ───────────────────────────────────────────────────────

    def network_info(self, iface: str = "eth0") -> dict:
        """Get IP address info for an interface."""
        if not _validate_iface(iface, _VALID_NET_IFACES | _VALID_CAN_IFACES):
            return {"ip": "N/A", "state": "invalid"}

        if self.simulation_mode:
            return dict(self._sim_state["net"].get(iface, {"ip": "N/A", "state": "down"}))

        ok, out, _ = self._run(["ip", "addr", "show", iface])
        if not ok:
            return {"ip": "N/A", "state": "not_found"}

        state = "down"
        if "state UP" in out:
            state = "up"

        ip = "N/A"
        m = re.search(r"inet\s+([\d.]+)", out)
        if m:
            ip = m.group(1)

        return {"ip": ip, "state": state}

    def set_static_ip(
        self, iface: str, ip: str, netmask: str = "24"
    ) -> None:
        """Set a static IP on an interface."""
        if not _validate_iface(iface, _VALID_NET_IFACES):
            self._emit("set_ip", False, f"Invalid interface: {iface}")
            return
        if not _validate_ip(ip):
            self._emit("set_ip", False, f"Invalid IP: {ip}")
            return
        # netmask as CIDR prefix
        try:
            prefix = int(netmask)
            if not 1 <= prefix <= 32:
                raise ValueError
        except ValueError:
            self._emit("set_ip", False, f"Invalid netmask/prefix: {netmask}")
            return

        if self.simulation_mode:
            self._sim_state["net"][iface] = {"ip": ip, "state": "up"}
            self._emit("set_ip", True, f"[SIM] {iface} -> {ip}/{prefix}")
            return

        # Flush existing
        self._run(["ip", "addr", "flush", "dev", iface], sudo=True)
        # Add new
        ok, out, err = self._run(
            ["ip", "addr", "add", f"{ip}/{prefix}", "dev", iface], sudo=True
        )
        self._emit("set_ip", ok, f"{iface} -> {ip}/{prefix}" if ok else (err or out))


# ── Singleton ─────────────────────────────────────────────────────────
hw_configurator = HardwareConfigurator()
