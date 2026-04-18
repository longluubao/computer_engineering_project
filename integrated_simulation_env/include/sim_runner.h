#ifndef ISE_SIM_RUNNER_H
#define ISE_SIM_RUNNER_H

#include <stdint.h>
#include <stdbool.h>
#include "sim_ecu.h"
#include "sim_attacker.h"
#include "sim_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SimConfig is the parsed CLI configuration passed to every scenario.
 * Scenarios may override any field (e.g. mixed_bus always runs on two
 * buses regardless of --bus) but should respect seed, iterations and
 * protection.
 */
typedef struct {
    uint32_t          iterations;
    uint64_t          seed;
    SimProtectionMode protection;
    SimBusKind        bus_kind;
    SimAttackKind     attack_kind;
    SimLogLevel       log_level;
    bool              deterministic;
    bool              rebuild_pqc_keys;
    const char       *out_dir;
} SimConfig;

/* Convenience helper used by every scenario to finalise the summary. */
void sim_scenario_finalise(const SimMetrics *m, const char *scenario,
                           const SimConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_RUNNER_H */
