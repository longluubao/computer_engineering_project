#ifndef ISE_SIM_ECU_H
#define ISE_SIM_ECU_H

#include <stdint.h>
#include <stdbool.h>
#include "sim_bus.h"
#include "sim_metrics.h"
#include "sim_signals.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Virtual ECU. Owns a SecOC instance and the upper-layer Com/PduR
 * plumbing. Each ECU is bound to exactly one primary bus (Tx/Rx) and
 * optionally a gateway bus.
 *
 * The ECU's lifecycle:
 *   sim_ecu_create() → sim_ecu_init_stack() → sim_ecu_start()
 *   → ... send/receive signals ...
 *   → sim_ecu_stop() → sim_ecu_destroy().
 */

typedef enum {
    SIM_ECU_ROLE_TX  = 0,
    SIM_ECU_ROLE_RX  = 1,
    SIM_ECU_ROLE_GW  = 2
} SimEcuRole;

typedef enum {
    SIM_PROT_NONE = 0,
    SIM_PROT_HMAC = 1,
    SIM_PROT_PQC  = 2,
    SIM_PROT_HYBRID = 3
} SimProtectionMode;

typedef struct {
    const char        *name;
    SimEcuRole         role;
    SimBus            *primary_bus;
    SimBus            *gateway_bus;       /* NULL for non-GW roles */
    SimProtectionMode  protection;
    SimMetrics        *metrics;           /* shared with scenario */
    uint64_t           seed;
} SimEcuCfg;

typedef struct SimEcu SimEcu;

SimEcu *sim_ecu_create(const SimEcuCfg *cfg);
bool    sim_ecu_init_stack(SimEcu *ecu);        /* EcuM_Init + SecOC_Init + ... */
bool    sim_ecu_start(SimEcu *ecu);             /* spawn Tx/Rx thread           */
void    sim_ecu_stop(SimEcu *ecu);
void    sim_ecu_destroy(SimEcu *ecu);

/*
 * Application-level transmit: the ECU cooks a SecOC-authenticated PDU
 * and hands it to the stack. Timing is recorded in the shared metrics.
 * Returns false on queue-full / immediate error.
 */
bool sim_ecu_send_signal(SimEcu *ecu, const SimSignalDef *sig,
                         const uint8_t *payload, uint32_t len);

/*
 * Blocking wait for the next authenticated signal (only Rx role).
 * Returns true on successful verification. `out_len` may be less than
 * the caller's buffer.
 */
bool sim_ecu_recv_signal(SimEcu *ecu, uint16_t *signal_id,
                         uint8_t *buf, uint32_t max_len, uint32_t *out_len,
                         uint64_t timeout_ns);

/*
 * Drive the SecOC main function once (useful in deterministic scenarios
 * that want lock-step control rather than a background thread).
 */
void sim_ecu_tick(SimEcu *ecu);

/*
 * Session establishment helpers (PQC mode only). Performs ML-KEM
 * handshake between two ECUs and derives an HKDF session key. The two
 * ECUs share the virtual bus used for key material transport.
 */
bool sim_ecu_pqc_handshake(SimEcu *tx, SimEcu *rx);

/*
 * Distribute the signer's ML-DSA public key (and, for HMAC/HYBRID, the
 * session key) to the verifier. Scenarios call this after key material
 * is provisioned so that Rx-side verification can succeed without
 * peeking into the opaque SimEcu struct.
 */
bool sim_ecu_share_keys(SimEcu *src, SimEcu *dst);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_ECU_H */
