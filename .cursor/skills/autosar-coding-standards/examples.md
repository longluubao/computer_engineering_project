# AUTOSAR Coding Standards Examples

## Example 1: Naming a New Enum and Literals

Input:

"Add a PDU type enum for SecOC."

Expected response style:

- Enum type uses `<Ma>_<Tn>_Type`:
  - `SecOC_PduType_Type`
- Literals use `<MIP>_<SN>`:
  - `SECOC_IFPDU`, `SECOC_TPPDU`
- Confirm naming aligns with `Autosar_SecOC/source/Code_Style.md`.

## Example 2: Adding a New Error Macro

Input:

"Add an error code for invalid freshness length."

Expected response style:

- Use error naming pattern `<MIP>_E_<EN>`:
  - `SECOC_E_INVALID_FRESHNESS_LENGTH`
- Keep uppercase with underscores.
- Place declaration and usage consistently in module scope.

## Example 3: Adding a Config Type and Parameter

Input:

"Create config container for message-link behavior."

Expected response style:

- Container type naming:
  - `SecOC_UseMessageLinkType`
- Config parameter naming:
  - `SecOCMessageLinkLen`, `SecOCMessageLinkPos`
- Ensure final symbols follow `Code_Style.md` checklist.
