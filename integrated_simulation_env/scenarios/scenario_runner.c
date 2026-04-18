/*
 * Scenario runner: CLI entry point for the Integrated Simulation Environment.
 *
 *   ise_runner --scenario <name> [--bus <bus>] [--iterations N]
 *              [--seed <u64>] [--out <dir>] [--protection hmac|pqc|hybrid]
 *              [--deterministic] [--attack <kind>] [--log-level info]
 *
 * Every scenario receives the parsed SimConfig and is free to set up its
 * own ECUs / buses. At the end sim_metrics_dump() flushes the summary.
 */

#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PQC.h"
#include "PQC_KeyDerivation.h"

/* Scenario entry-points (see sc_*.c files). */
int sc_baseline_run  (const SimConfig *);
int sc_throughput_run(const SimConfig *);
int sc_mixed_bus_run (const SimConfig *);
int sc_attacks_run   (const SimConfig *);
int sc_rekey_run     (const SimConfig *);

static struct {
    const char *name;
    int (*fn)(const SimConfig *);
    const char *desc;
} g_scenarios[] = {
    { "baseline",    sc_baseline_run,   "Baseline per-signal latency & overhead" },
    { "throughput",  sc_throughput_run, "Sustained throughput stress test"       },
    { "mixed_bus",   sc_mixed_bus_run,  "CAN-FD ↔ Ethernet gateway scenario"    },
    { "attacks",     sc_attacks_run,    "All 10 attack scenarios, sequential"    },
    { "rekey",       sc_rekey_run,      "Session rekey + key-exchange stress"    },
    { NULL, NULL, NULL }
};

static void usage(void)
{
    fprintf(stderr,
        "usage: ise_runner --scenario <name> [options]\n"
        "scenarios:\n");
    for (int i = 0; g_scenarios[i].name; ++i) {
        fprintf(stderr, "  %-12s  %s\n",
                g_scenarios[i].name, g_scenarios[i].desc);
    }
    fprintf(stderr,
        "options:\n"
        "  --iterations N        number of messages per signal (default 200)\n"
        "  --seed U              seed for BER + attacker RNGs (default 0xC0FFEE)\n"
        "  --out DIR             output directory (default results/<utc>)\n"
        "  --protection MODE     hmac | pqc | hybrid (default pqc)\n"
        "  --bus KIND            can20 | canfd | flexray | eth100 | eth1000\n"
        "  --attack KIND         none | replay | tamper_payload | ... (for 'attacks')\n"
        "  --deterministic       use discrete-tick clock\n"
        "  --log-level LVL       trace|debug|info|warn|error\n"
        "  --rebuild-pqc         force ML-DSA/ML-KEM key regeneration\n"
        "  --help                show this help\n");
}

static SimProtectionMode parse_protection(const char *s)
{
    if (!s) return SIM_PROT_PQC;
    if (!strcmp(s, "hmac"))   return SIM_PROT_HMAC;
    if (!strcmp(s, "pqc"))    return SIM_PROT_PQC;
    if (!strcmp(s, "hybrid")) return SIM_PROT_HYBRID;
    if (!strcmp(s, "none"))   return SIM_PROT_NONE;
    return SIM_PROT_PQC;
}

static SimBusKind parse_bus(const char *s)
{
    if (!s) return SIM_BUS_CAN_FD;
    if (!strcmp(s, "can20"))   return SIM_BUS_CAN_20;
    if (!strcmp(s, "canfd"))   return SIM_BUS_CAN_FD;
    if (!strcmp(s, "flexray")) return SIM_BUS_FLEXRAY;
    if (!strcmp(s, "eth100"))  return SIM_BUS_ETH_100;
    if (!strcmp(s, "eth1000")) return SIM_BUS_ETH_1000;
    return SIM_BUS_CAN_FD;
}

static SimLogLevel parse_level(const char *s)
{
    if (!s) return SIM_LOG_INFO;
    if (!strcmp(s, "trace")) return SIM_LOG_TRACE;
    if (!strcmp(s, "debug")) return SIM_LOG_DEBUG;
    if (!strcmp(s, "info"))  return SIM_LOG_INFO;
    if (!strcmp(s, "warn"))  return SIM_LOG_WARN;
    if (!strcmp(s, "error")) return SIM_LOG_ERROR;
    return SIM_LOG_INFO;
}

int main(int argc, char **argv)
{
    SimConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.iterations      = 200;
    cfg.seed            = 0x00000000C0FFEEULL;
    cfg.protection      = SIM_PROT_PQC;
    cfg.bus_kind        = SIM_BUS_CAN_FD;
    cfg.attack_kind     = SIM_ATK_NONE;
    cfg.log_level       = SIM_LOG_INFO;
    cfg.deterministic   = false;
    cfg.rebuild_pqc_keys = false;

    char out_dir[512] = {0};
    const char *scenario_name = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (!strcmp(a, "--help") || !strcmp(a, "-h")) { usage(); return 0; }

        #define TAKE_ARG() do { \
            if (i + 1 >= argc) { usage(); return 2; } \
            a = argv[++i]; \
        } while (0)

        if (!strcmp(a, "--scenario"))          { TAKE_ARG(); scenario_name = a; }
        else if (!strcmp(a, "--iterations"))   { TAKE_ARG(); cfg.iterations = (uint32_t)atoi(a); }
        else if (!strcmp(a, "--seed"))         { TAKE_ARG(); cfg.seed = strtoull(a, NULL, 0); }
        else if (!strcmp(a, "--out"))          { TAKE_ARG(); strncpy(out_dir, a, sizeof(out_dir) - 1); }
        else if (!strcmp(a, "--protection"))   { TAKE_ARG(); cfg.protection = parse_protection(a); }
        else if (!strcmp(a, "--bus"))          { TAKE_ARG(); cfg.bus_kind = parse_bus(a); }
        else if (!strcmp(a, "--attack"))       { TAKE_ARG(); cfg.attack_kind = (SimAttackKind)atoi(a); }
        else if (!strcmp(a, "--log-level"))    { TAKE_ARG(); cfg.log_level = parse_level(a); }
        else if (!strcmp(a, "--deterministic")){ cfg.deterministic = true; }
        else if (!strcmp(a, "--rebuild-pqc"))  { cfg.rebuild_pqc_keys = true; }
        else { fprintf(stderr, "unknown option: %s\n", a); usage(); return 2; }
    }

    if (!scenario_name) { usage(); return 2; }

    if (out_dir[0] == '\0') {
        char ts[32];
        sim_utc_timestamp(ts, sizeof(ts));
        snprintf(out_dir, sizeof(out_dir), "results/%s_%s", scenario_name, ts);
    }

    sim_clock_init(cfg.deterministic ? SIM_CLOCK_DETERMINISTIC : SIM_CLOCK_WALLTIME,
                   1000);

    SimLoggerCfg lcfg;
    lcfg.out_dir       = out_dir;
    lcfg.scenario      = scenario_name;
    lcfg.seed          = cfg.seed;
    lcfg.git_hash      = "ise";
    lcfg.console_level = cfg.log_level;
    sim_logger_init(&lcfg);
    cfg.out_dir = out_dir;

    /* Initialise the real AUTOSAR PQC module. Required once per process. */
    if (PQC_Init() != PQC_E_OK) {
        sim_log(SIM_LOG_ERROR, "PQC_Init() failed");
        sim_logger_shutdown();
        return 2;
    }
    if (PQC_KeyDerivation_Init() != PQC_E_OK) {
        sim_log(SIM_LOG_ERROR, "PQC_KeyDerivation_Init() failed");
        sim_logger_shutdown();
        return 2;
    }

    int rc = -1;
    for (int i = 0; g_scenarios[i].name; ++i) {
        if (!strcmp(g_scenarios[i].name, scenario_name)) {
            sim_log(SIM_LOG_INFO, "running scenario '%s' (%s)",
                    scenario_name, g_scenarios[i].desc);
            rc = g_scenarios[i].fn(&cfg);
            break;
        }
    }
    if (rc < 0) {
        sim_log(SIM_LOG_ERROR, "unknown scenario '%s'", scenario_name);
    }

    sim_logger_shutdown();
    return rc < 0 ? 1 : 0;
}
