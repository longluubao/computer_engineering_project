# AUTOSAR Coding Standards Reference

This skill is anchored to:

- `Autosar_SecOC/source/Code_Style.md`

Use that file as final authority.

## Quick Symbol Patterns

- Configuration parameter: `<Ma><Pn>`
- Config container type: `<Ma>_<Tn>Type`
- Enumeration type: `<Ma>_<Tn>_Type`
- Enum literals and preprocessor constants: `<MIP>_<SN>`
- Error values: `<MIP>_E_<EN>`
- Global variable: `<Mip>_<Vn>`
- Non-config typedef: `<Ma>_<Tn>Type`
- Callout declaration pattern:
  - `#define <MIP>_START_SEC_<CN>_CODE`
  - `FUNC(void, <MIP>_<CN>_CODE) <Cn>(void);`
  - `#define <MIP>_STOP_SEC_<CN>_CODE`

## Examples (from project style guide)

- `SecOCSecuredTxPduLength`
- `SecOC_UseMessageLinkType`
- `SecOC_PduType_Type`
- `SECOC_AUTH_PDUHEADER_LENGTH`
- `SECOC_BUFREQ_E_NOT_OK`
- `SecOC_MessageBuffer`
- `SecOC_VerificationStatusType`

## Agent Checklist for Every Change

1. New symbols match required naming pattern.
2. Added constants/enums are uppercase with underscores.
3. Public type names remain AUTOSAR-aligned.
4. Read-only globals use `const`.
5. Callout APIs use required section macros and naming.
6. No accidental mixed naming style in same module.

## Ambiguity Handling

If a rule appears unclear in a specific file:

1. Prefer the nearest existing module-local naming precedent.
2. Keep consistency with public headers already in use.
3. Flag the ambiguity explicitly in the final response.
