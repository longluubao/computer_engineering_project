#ifndef ISE_SIM_METRICS_H
#define ISE_SIM_METRICS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Online histogram + percentile estimator used by every scenario.
 *
 * Buckets are log-scaled (power-of-two) so that microsecond and
 * millisecond events both get adequate resolution without using a
 * runtime-configurable structure. This makes the thesis tables
 * reproducible across runs.
 */

#define SIM_METRICS_BUCKETS 64
/* Reservoir size — enough samples to compute accurate percentiles for
 * any single scenario in the thesis (max scenario = ~20k samples). */
#define SIM_METRICS_RESERVOIR 16384

typedef struct {
    uint64_t count;
    uint64_t sum;
    uint64_t min;
    uint64_t max;
    uint64_t buckets[SIM_METRICS_BUCKETS]; /* log2 buckets in ns */
    /* Reservoir sample for accurate percentile computation. Samples
     * beyond SIM_METRICS_RESERVOIR are kept only in the log2 buckets. */
    uint64_t samples[SIM_METRICS_RESERVOIR];
    uint32_t sample_count;
    /* Welford online variance */
    double   mean;
    double   m2;
} SimHistogram;

void   sim_hist_init(SimHistogram *h);
void   sim_hist_add(SimHistogram *h, uint64_t value_ns);
double sim_hist_mean(const SimHistogram *h);
double sim_hist_stddev(const SimHistogram *h);
uint64_t sim_hist_percentile(const SimHistogram *h, double pct /*0..1*/);

/*
 * Scenario-wide rollup. Every scenario keeps one of these and hands it
 * to sim_metrics_dump() at the end.
 */
typedef struct {
    SimHistogram e2e_latency;      /* application-to-application */
    SimHistogram secoc_auth;
    SimHistogram secoc_verify;
    SimHistogram cantp;
    SimHistogram bus_transit;
    SimHistogram pdu_bytes;
    SimHistogram fragments;

    uint64_t     deadline_miss_count[7];   /* index by deadline class */
    uint64_t     deadline_total_count[7];
    uint64_t     verify_fail_count;
    uint64_t     drop_count;
    uint64_t     success_count;

    uint64_t     attacks_injected;
    uint64_t     attacks_detected;
    uint64_t     attacks_delivered;        /* passed verification incorrectly */

    uint64_t     tx_bytes_total;
    uint64_t     rx_bytes_total;
    uint64_t     session_ns_start;
    uint64_t     session_ns_end;
} SimMetrics;

void sim_metrics_init(SimMetrics *m);
void sim_metrics_dump(const SimMetrics *m, const char *path_json,
                      const char *scenario, uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_METRICS_H */
