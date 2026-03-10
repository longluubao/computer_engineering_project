---
name: autosar-coding-standards
description: Enforces AUTOSAR BSWGeneral naming and coding conventions for this repository, with Code_Style.md as the source of truth. Use for every code edit, refactor, review, and code generation task inside Autosar_SecOC, especially for C headers and source files under include/ and source/.
---

# AUTOSAR Coding Standards

## Scope

Apply this skill to all agent work touching this project codebase, especially:

- `Autosar_SecOC/include/**/*.h`
- `Autosar_SecOC/source/**/*.c`

Primary style reference:

- `Autosar_SecOC/source/Code_Style.md`

If there is any conflict, follow `Code_Style.md`.

## Mandatory Workflow

Before writing or editing code:

1. Identify module prefix and abbreviation (for example `SecOC` / `SECOC`).
2. Choose names that satisfy AUTOSAR conventions from `Code_Style.md`.
3. Keep existing architectural boundaries (COM/BSW/MCAL/module split) intact.

After editing:

1. Run a naming compliance pass on new/changed symbols.
2. Ensure constants/enums/macros use the required uppercase style.
3. Ensure type names and config container names follow required patterns.

## Naming Rules Checklist

- Configuration parameter:
  - `<Ma><Pn>` in CamelCase (example: `SecOCSecuredTxPduLength`)
- Config container type:
  - `<Ma>_<Tn>Type` (example: `SecOC_UseMessageLinkType`)
- Enumeration type:
  - `<Ma>_<Tn>_Type` (example: `SecOC_PduType_Type`)
- Enum literals and `#define` values:
  - `<MIP>_<SN>` uppercase with underscores (example: `SECOC_IFPDU`)
- Error values:
  - `<MIP>_E_<EN>` uppercase with underscores
- Global variables:
  - `<Mip>_<Vn>` (example style: `SecOC_MessageBuffer`)
- Non-config typedefs:
  - `<Ma>_<Tn>Type` (example: `SecOC_VerificationStatusType`)
- Callout section/function style:
  - Start/stop section macros and function prototype naming as defined in `Code_Style.md`

## Editing Rules

- Do not introduce naming that conflicts with AUTOSAR BSWGeneral conventions.
- Do not rename existing public APIs unless explicitly requested.
- For new globals with read-only purpose, use `const`.
- Keep code deterministic and simple; avoid unnecessary abstraction in BSW modules.

## Review Output Format

When reviewing or summarizing changes, report:

1. Which naming rules were applied.
2. Any deliberate exception and why.
3. Any unresolved style ambiguity that needs user decision.

## Additional Resource

- Detailed rule examples: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
