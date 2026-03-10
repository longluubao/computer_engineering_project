# AUTOSAR BswM Research Notes

## 1) What BswM Does

BswM (Basic Software Mode Manager) is the central mode arbitration module in AUTOSAR Classic Platform. It receives mode indications/requests from software components and BSW modules, evaluates configured rules, and executes action lists to control system behavior.

In practice, BswM is the decision hub for:

- ECU mode transitions
- Communication enable/limitation behavior
- Network management controls
- Coordination of state-related BSW actions

## 2) Core Architecture Model

### Inputs

- Mode requests or indications from modules such as EcuM, ComM, CanSM, Nm, Dcm, and possibly application interfaces.

### Decision

- Rule arbitration based on configured conditions and logical expressions (AND/OR/NOT/XOR/NAND style constructs depending on tooling/spec profile).

### Outputs

- Action list execution, typically invoking BSW APIs (for example ComM, EcuM, Nm related operations).

## 3) Rule and Action List Design

### Rules

Rules evaluate condition objects tied to current mode/event status and return boolean decisions that determine action execution.

Common condition semantics:

- Mode equals / not equals expected state
- Event request set / clear

### Action lists

Action lists are ordered operations executed when rules are satisfied. They may:

- Call BSW module APIs
- Chain to other action lists
- Trigger additional arbitration steps (tool/spec dependent)

Design guidance:

- Keep rules readable and low-coupling.
- Prefer composable, reusable action lists.
- Explicitly define precedence for conflicting requests.

## 4) Immediate vs Deferred Arbitration

### Immediate operation

- Evaluates and executes in the request context.
- Lower response latency.
- Higher risk of context-dependent timing effects.

### Deferred operation

- Evaluates in periodic `BswM_MainFunction`.
- More deterministic scheduling integration.
- Adds bounded reaction delay.

Choose based on criticality:

- Urgent safety/availability transitions: consider immediate.
- Non-urgent or high-frequency chatter: prefer deferred.

## 5) Integration with Key Modules

### EcuM

BswM can coordinate ECU state-related decisions via EcuM-oriented actions (for example state switch or shutdown target selection depending on stack/tool support).

### ComM

BswM commonly controls communication permissions and limitations:

- Allow/deny communication
- Limit communication mode
- Request/switch communication mode

### CanSM and network state modules

BswM uses their mode indications as conditions and may trigger communication-related actions that indirectly affect network behavior.

### Nm

BswM may enable/disable network management communication (for example through NM control actions), often coupled with diagnostics or power-state conditions.

## 6) Typical Failure Modes

### Rule appears correct but never triggers

Likely causes:

- Wrong request/condition port mapping
- Condition references stale or unexpected mode source
- Rule evaluated in deferred context but expectation assumed immediate behavior

### Wrong action list executed

Likely causes:

- Overlapping rule conditions without explicit priority/precedence intent
- Action list links causing unintended cascades

### Mode oscillation/chattering

Likely causes:

- No hysteresis/debounce-like logic in mode conditions
- Competing requests from multiple modules with unclear ownership

### Communication state inconsistent with expected ECU state

Likely causes:

- BswM action sequencing mismatch across EcuM/ComM/Nm
- Partial execution path due to missing route/action configuration

## 7) Bring-Up and Debug Strategy

1. Validate one input request path end-to-end (request port to action API call).
2. Verify each rule transition with explicit traces (false->true and true->false).
3. Validate immediate/deferred reaction timing against requirement budgets.
4. Test conflict scenarios where two request sources demand different modes.
5. Test startup/shutdown and degraded/fallback paths explicitly.

## 8) External Research Links Used

- BswM overview:
  - [BSWM Overview](https://www.autosartoday.com/posts/bswm_overview)
- BswM actions:
  - [BSWM Actions](https://www.autosartoday.com/posts/bswm_actions)
- Mode management context:
  - [Mode Management](https://www.autosartoday.com/posts/mode_management)
- Additional BswM explanation:
  - [Basic Software Mode Manager In AUTOSAR](https://piembsystech.com/basic-software-mode-manager-in-autosar-bswm)
- Rule example references:
  - [An example of BswM rule](https://rtahotline.etas.com/confluence/display/RH/An+example+of+BswM+rule)
  - [Mode Management configuration example](https://rtahotline.etas.com/confluence/display/RH/05+-+Mode+Management+configuration+-+RTA-CAR+9.2.0)
