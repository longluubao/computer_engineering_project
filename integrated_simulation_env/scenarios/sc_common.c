#include "sim_runner.h"
#include "sim_metrics.h"
#include "sim_logger.h"

#include <stdio.h>
#include <string.h>

void sim_scenario_finalise(const SimMetrics *m, const char *scenario,
                           const SimConfig *cfg)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s_summary.json",
             cfg->out_dir, scenario);
    sim_metrics_dump(m, path, scenario, cfg->seed);

    sim_log(SIM_LOG_INFO, "=========== %s summary ===========", scenario);
    sim_log(SIM_LOG_INFO,
            "messages   success=%llu  verify_fail=%llu  drop=%llu",
            (unsigned long long)m->success_count,
            (unsigned long long)m->verify_fail_count,
            (unsigned long long)m->drop_count);
    if (m->e2e_latency.count > 0) {
        sim_log(SIM_LOG_INFO,
                "e2e latency p50/p95/p99 = %.1f / %.1f / %.1f us",
                (double)sim_hist_percentile(&m->e2e_latency, 0.50) / 1000.0,
                (double)sim_hist_percentile(&m->e2e_latency, 0.95) / 1000.0,
                (double)sim_hist_percentile(&m->e2e_latency, 0.99) / 1000.0);
    }
    if (m->secoc_auth.count > 0) {
        sim_log(SIM_LOG_INFO,
                "SecOC auth   mean=%.1f us  verify mean=%.1f us",
                sim_hist_mean(&m->secoc_auth) / 1000.0,
                sim_hist_mean(&m->secoc_verify) / 1000.0);
    }
    for (int i = 1; i <= 6; ++i) {
        if (m->deadline_total_count[i] > 0) {
            double rate = 100.0 * (double)m->deadline_miss_count[i] /
                          (double)m->deadline_total_count[i];
            sim_log(SIM_LOG_INFO,
                    "deadline D%d miss %llu/%llu (%.2f%%)",
                    i,
                    (unsigned long long)m->deadline_miss_count[i],
                    (unsigned long long)m->deadline_total_count[i],
                    rate);
        }
    }
    if (m->attacks_injected > 0) {
        sim_log(SIM_LOG_INFO,
                "attacks injected=%llu detected=%llu delivered=%llu",
                (unsigned long long)m->attacks_injected,
                (unsigned long long)m->attacks_detected,
                (unsigned long long)m->attacks_delivered);
    }
}
