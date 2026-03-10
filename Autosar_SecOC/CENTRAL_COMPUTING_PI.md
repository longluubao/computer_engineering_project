# Raspberry Pi as Central Computing Node (Zonal Architecture)

## 1) Decision Summary

For this project, the Raspberry Pi 4 should be treated as a **Central Gateway / Central Compute prototype**:

- Keep user-facing controls at the **network/service/security** level.
- Move low-level pin controls (GPIO) out of normal operator workflows.
- Focus on routing, diagnostics, service orchestration, and cybersecurity.

This aligns with zonal E/E architecture guidance where the central node handles cross-domain communication and the zonal layer connects local actuators/sensors.

## 2) What the UI should configure (and what it should not)

### In Scope (Primary)

- Gateway identity and role (HPCU / CGW)
- Interface state: Ethernet, CAN, service health
- Routing rules (zone-to-zone / ECU-to-ECU)
- Security policies:
  - firewall profile
  - IDS mode
  - secure service policy
- Diagnostics:
  - link errors
  - packet drops
  - CAN error counters
  - service latency
- Deployment:
  - start/stop/restart central services
  - profile switching (Lab / Demo / Safe mode)

### Out of Scope (Operator UI)

- Manual GPIO export/direction toggling
- Per-pin on/off controls
- hardware bring-up scripts exposed to normal users

If needed, keep these under a hidden "Engineering Mode" only.

## 3) Target Architecture on Raspberry Pi 4

## 3.1 Core services

Run each as a supervised Linux service (`systemd`):

1. **Gateway Agent (Python)**
   - owns UI/backend state
   - exposes internal API for status/control
2. **Routing Service**
   - CAN <-> Ethernet policy routing (project-defined mapping)
3. **Security Service**
   - firewall profile management (`nftables`)
   - IDS integration mode (`Suricata` optional in prototype)
4. **Diagnostics Service**
   - collects system/bus/network metrics
   - publishes events to UI
5. **Service Middleware Node**
   - start with a simple message bus or SOME/IP proof-of-concept

## 3.2 Data plane

- **Southbound (zone side)**: CAN (SocketCAN), optional local sensors
- **Backbone**: Ethernet (100/1000BASE-T class links for prototype network)
- **Northbound**: diagnostics endpoint / cloud bridge (optional)

## 3.3 Control plane

- Configuration profiles stored in JSON/YAML
- Versioned profile loader
- Safe rollback to "last known good" profile

## 4) Suggested software stack (prototype-realistic)

- **Linux networking**: `iproute2`, `tc`, `nftables`
- **CAN**: SocketCAN (`python-can` for app-level integration)
- **Security**: `nftables`, optional `Suricata` inline/monitor mode
- **Timing/sync**: `linuxptp` for PTP/gPTP experiments
- **Service orchestration**: `systemd`
- **Observability**:
  - structured logs (JSON)
  - local metrics exporter (Prometheus-style optional)

## 5) Service API model (for your GUI/backend)

Use capability-level APIs instead of pin-level APIs:

- `GET /gateway/status`
- `GET /interfaces`
- `POST /interfaces/can0/up`
- `POST /interfaces/can0/down`
- `GET /routes`
- `POST /routes`
- `GET /security/policy`
- `POST /security/policy/apply`
- `GET /diagnostics/summary`
- `GET /diagnostics/events`

## 6) Performance and reliability targets (Pi 4 prototype)

- UI update latency: < 300 ms for status views
- command apply latency: < 1 s for control actions
- startup to ready: < 15 s
- graceful degradation:
  - if IDS disabled, gateway still routes
  - if route rule invalid, reject and keep previous policy

## 7) Phased implementation plan

### Phase A (Now)

- Remove GPIO from normal Hardware page.
- Keep Interface Status + CAN + IP controls.
- Add "Central Gateway Health" card:
  - service state
  - CPU/mem
  - link/can summary

### Phase B

- Introduce routing policy editor (zone path rules).
- Add security profile manager (firewall presets).
- Add diagnostics timeline/event console.

### Phase C

- Service-oriented communication experiment:
  - SOME/IP prototype (e.g., vSomeIP test node) or
  - alternative pub/sub middleware test
- PTP/gPTP sync tests for deterministic timing.

## 8) Why this direction fits zonal architecture

- Zonal systems separate central "thinking" from local "acting".
- Central gateway responsibility is cross-domain communication + security + diagnostics.
- Ethernet-centric evolution reduces reliance on heterogeneous, pin-level operational controls.

For your thesis/demo use case, this gives a cleaner and more realistic product story than GPIO-heavy UI.

## References

- Bosch Zone ECU: https://www.bosch-mobility.com/en/solutions/control-units/zone-ecu/
- Bosch Central Gateway: https://www.bosch-mobility.com/en/solutions/vehicle-computer/central-gateway/
- Keysight Automotive Ethernet (reference URL): https://www.keysight.com/blogs/en/tech/educ/2024/automotive-ethernet
- Linux PTP project: https://linuxptp.nwtime.org/
- COVESA vSomeIP: https://github.com/COVESA/vsomeip/wiki/vsomeip-in-10-minutes
- Suricata inline mode (Linux): https://docs.suricata.io/en/suricata-7.0.11/setting-up-ipsinline-for-linux.html
