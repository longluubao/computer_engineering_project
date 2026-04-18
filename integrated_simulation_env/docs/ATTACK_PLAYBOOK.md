# Attack Playbook

The ISE implements ten parameterised attackers. Every attacker is an
on-the-wire hook installed by `sim_attacker_attach()`; it may drop,
modify, inject or simply observe frames. Each attack maps back to at
least one clause of UN R155 Annex 5, ISO/SAE 21434 §9.4, or the
AUTOSAR SecOC security goals.

For every run the logger emits:

- `*_attacks.csv` — one row per injected attack with `detected`,
  `delivered` flags,
- `*_summary.json` — aggregate counters,
- `attack_detection.csv` (post-processed by the Python report).

## 1. Replay  (`SIM_ATK_REPLAY`)

- **How it works:** the hook stores the last few authentic frames and
  re-injects one of them periodically.
- **Expected outcome:** SecOC rejects by freshness mismatch.
- **Maps to:** UN R155 Annex 5, threat 4.3.2.
- **Success criterion:** `detected == injected` and
  `delivered == 0`.

## 2. Payload Tamper  (`SIM_ATK_TAMPER_PAYLOAD`)

- **How it works:** flips a random bit inside the authentic payload.
- **Expected outcome:** MAC or ML-DSA verification fails.
- **Maps to:** UN R155 Annex 5, threats 4.3.1 and 4.3.6.

## 3. Authenticator Tamper  (`SIM_ATK_TAMPER_AUTH`)

- **How it works:** flips bits inside the MAC or signature bytes.
- **Expected outcome:** verify fails (EUF-CMA / SUF-CMA).
- **Maps to:** FIPS 204 §8 security requirements.

## 4. Freshness Rollback  (`SIM_ATK_FRESHNESS_ROLLBACK`)

- **How it works:** rewrites the 64-bit freshness counter to zero.
- **Expected outcome:** SecOC drops — current counter must strictly
  increase.
- **Maps to:** AUTOSAR SWS_SecOC_00209 (freshness management).

## 5. MITM / Key Confusion  (`SIM_ATK_MITM_KEY_CONFUSE`)

- **How it works:** scrambles ML-KEM handshake ciphertext so both
  sides end up with different shared secrets.
- **Expected outcome:** Rx side cannot verify any subsequent MAC /
  signature.
- **Maps to:** FIPS 203 §7 IND-CCA2 guarantee; ISO 21434 §9.4.

## 6. DoS Flood  (`SIM_ATK_DOS_FLOOD`)

- **How it works:** increases frame duplication so the bus saturates.
- **Expected outcome:** `bus_dropped_queue` climbs, but no forged
  frame is accepted. Thesis point: SecOC does not prevent DoS, but it
  prevents *successful* tampering under DoS.
- **Maps to:** UN R155 Annex 5, threat 4.3.5.

## 7. Signature Fuzz  (`SIM_ATK_SIG_FUZZ`)

- **How it works:** randomises the last 64 bytes of the ML-DSA
  signature.
- **Expected outcome:** verify fails every time.
- **Maps to:** FIPS 204 §8 (negative test).

## 8. Downgrade to HMAC  (`SIM_ATK_DOWNGRADE_HMAC`)

- **How it works:** flips the protection-mode bits in the secured PDU
  header so it claims to be HMAC-protected when both ECUs are in PQC
  mode.
- **Expected outcome:** receiving ECU must refuse to downgrade — it
  expected PQC.
- **Maps to:** emerging PQC requirement (post-Harvest-Now threat
  model).

## 9. Harvest-Now-Decrypt-Later  (`SIM_ATK_HARVEST_NOW`)

- **How it works:** passive observer that copies every ciphertext to
  disk.
- **Expected outcome:** long-term quantum adversary cannot break
  ML-KEM / ML-DSA → even harvested traffic remains confidential.
- **Maps to:** FIPS 203/204 motivation section.
- **Evidence:** the harvested bytes are emitted as
  `harvest_<n>.bin` so the thesis can show the storage overhead
  estimate.

## 10. Timing Probe  (`SIM_ATK_TIMING_PROBE`)

- **How it works:** passive observation of verification time; produces
  a histogram for side-channel analysis.
- **Expected outcome:** ML-DSA verification time should be
  input-independent (constant-time implementation).
- **Maps to:** ISO 21434 §9.4 (side-channel considerations).

## Thesis interpretation

The three canonical tables the thesis needs are:

| Attack category               | Expected `detected`/`injected` | PQC-specific? |
|-------------------------------|--------------------------------|---------------|
| Active tampering (2, 3, 7)    | 100 %                          | partially     |
| Replay family (1, 4)          | 100 %                          | no            |
| MITM / downgrade (5, 8)       | 100 %                          | yes           |
| Passive (9, 10)               | "observed only"                | yes           |
| Availability (6)              | "no forgery under DoS"         | no            |

Any detected-rate < 100 % in categories 1–3/5 is a defect that must be
investigated before thesis submission.
