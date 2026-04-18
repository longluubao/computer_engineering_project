#ifndef ISE_SIM_ATTACKER_H
#define ISE_SIM_ATTACKER_H

#include <stdint.h>
#include <stdbool.h>
#include "sim_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Every attack is implemented as a small hook installed on a bus.
 * The hook may drop, modify, inject, or observe frames. The attacker
 * also records into sim_logger whether the target SecOC instance
 * detected the attack; this is what gives us the detection-rate
 * columns of the thesis tables.
 */

typedef enum {
    SIM_ATK_NONE              = 0,
    SIM_ATK_REPLAY            = 1,   /* store old frame, re-inject later   */
    SIM_ATK_TAMPER_PAYLOAD    = 2,   /* flip bits in the authentic part    */
    SIM_ATK_TAMPER_AUTH       = 3,   /* flip bits in the authenticator     */
    SIM_ATK_FRESHNESS_ROLLBACK= 4,   /* rewrite freshness counter          */
    SIM_ATK_MITM_KEY_CONFUSE  = 5,   /* break ML-KEM handshake             */
    SIM_ATK_DOS_FLOOD         = 6,   /* saturate the bus                   */
    SIM_ATK_SIG_FUZZ          = 7,   /* random signature bytes             */
    SIM_ATK_DOWNGRADE_HMAC    = 8,   /* force classical MAC over PQC link  */
    SIM_ATK_HARVEST_NOW       = 9,   /* record ciphertext for later quantum decrypt */
    SIM_ATK_TIMING_PROBE      = 10   /* measure reaction time              */
} SimAttackKind;

typedef struct {
    SimAttackKind kind;
    uint32_t      period_ms;       /* how often to fire; 0 = every frame   */
    uint32_t      target_signal;   /* 0 = any                              */
    uint8_t       target_bus_id;   /* 0xFF = any                           */
    uint32_t      payload_u32;     /* kind-specific parameter              */
    uint64_t      seed;
} SimAttackCfg;

typedef struct SimAttacker SimAttacker;

SimAttacker *sim_attacker_create(const SimAttackCfg *cfg);
void         sim_attacker_attach(SimAttacker *atk, SimBus *bus);
void         sim_attacker_detach(SimAttacker *atk);
void         sim_attacker_destroy(SimAttacker *atk);

/* Called by sim_ecu_rx after verifying a frame; increments detection counter. */
void sim_attacker_notify_verify_outcome(SimAttacker *atk, bool verified_ok,
                                        uint16_t signal_id);

/* Final statistics for summary reporting. */
typedef struct {
    uint64_t injected;
    uint64_t detected;
    uint64_t delivered;
} SimAttackerStats;

void sim_attacker_get_stats(const SimAttacker *atk, SimAttackerStats *out);

const char *sim_attack_name(SimAttackKind kind);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_ATTACKER_H */
