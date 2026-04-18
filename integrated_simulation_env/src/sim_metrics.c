#include "sim_metrics.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint32_t bucket_of(uint64_t value_ns)
{
    if (value_ns == 0) return 0;
    /* log2(value_ns), clamped to SIM_METRICS_BUCKETS-1. */
    uint32_t b = 0;
    while (value_ns > 1 && b < (SIM_METRICS_BUCKETS - 1)) {
        value_ns >>= 1;
        ++b;
    }
    return b;
}

void sim_hist_init(SimHistogram *h)
{
    memset(h, 0, sizeof(*h));
    h->min = UINT64_MAX;
}

void sim_hist_add(SimHistogram *h, uint64_t v)
{
    h->count++;
    h->sum += v;
    if (v < h->min) h->min = v;
    if (v > h->max) h->max = v;
    h->buckets[bucket_of(v)]++;

    if (h->sample_count < SIM_METRICS_RESERVOIR) {
        h->samples[h->sample_count++] = v;
    }

    /* Welford */
    double dv = (double)v;
    double delta = dv - h->mean;
    h->mean += delta / (double)h->count;
    double delta2 = dv - h->mean;
    h->m2 += delta * delta2;
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x < y) ? -1 : (x > y) ? 1 : 0;
}

double sim_hist_mean(const SimHistogram *h)
{
    return (h->count == 0) ? 0.0 : h->mean;
}

double sim_hist_stddev(const SimHistogram *h)
{
    if (h->count < 2) return 0.0;
    return sqrt(h->m2 / (double)(h->count - 1));
}

uint64_t sim_hist_percentile(const SimHistogram *h, double pct)
{
    if (h->count == 0) return 0;
    if (pct <= 0.0) return h->min;
    if (pct >= 1.0) return h->max;

    /* Exact percentile from the reservoir when all samples fit. */
    if (h->sample_count > 0 && h->count == h->sample_count) {
        uint64_t *sorted = (uint64_t *)malloc((size_t)h->sample_count *
                                              sizeof(uint64_t));
        if (!sorted) return h->max;
        memcpy(sorted, h->samples,
               (size_t)h->sample_count * sizeof(uint64_t));
        qsort(sorted, h->sample_count, sizeof(uint64_t), cmp_u64);
        uint32_t idx = (uint32_t)((double)(h->sample_count - 1) * pct);
        uint64_t r = sorted[idx];
        free(sorted);
        return r;
    }

    /* Fallback: coarse log2 bucket when we overflowed the reservoir. */
    uint64_t target = (uint64_t)((double)h->count * pct);
    uint64_t running = 0;
    for (uint32_t i = 0; i < SIM_METRICS_BUCKETS; ++i) {
        running += h->buckets[i];
        if (running >= target) {
            uint64_t lo = (i == 0) ? 0 : (1ULL << i);
            uint64_t hi = 1ULL << (i + 1);
            return (lo + hi) / 2ULL;
        }
    }
    return h->max;
}

void sim_metrics_init(SimMetrics *m)
{
    memset(m, 0, sizeof(*m));
    sim_hist_init(&m->e2e_latency);
    sim_hist_init(&m->secoc_auth);
    sim_hist_init(&m->secoc_verify);
    sim_hist_init(&m->cantp);
    sim_hist_init(&m->bus_transit);
    sim_hist_init(&m->pdu_bytes);
    sim_hist_init(&m->fragments);
}

static void dump_hist(FILE *fp, const char *name, const SimHistogram *h)
{
    fprintf(fp,
        "    \"%s\": {\n"
        "      \"count\": %llu, \"min_ns\": %llu, \"max_ns\": %llu,\n"
        "      \"mean_ns\": %.2f, \"stddev_ns\": %.2f,\n"
        "      \"p50_ns\": %llu, \"p95_ns\": %llu, \"p99_ns\": %llu, \"p999_ns\": %llu\n"
        "    }",
        name,
        (unsigned long long)h->count,
        (unsigned long long)(h->count ? h->min : 0),
        (unsigned long long)h->max,
        sim_hist_mean(h),
        sim_hist_stddev(h),
        (unsigned long long)sim_hist_percentile(h, 0.50),
        (unsigned long long)sim_hist_percentile(h, 0.95),
        (unsigned long long)sim_hist_percentile(h, 0.99),
        (unsigned long long)sim_hist_percentile(h, 0.999));
}

void sim_metrics_dump(const SimMetrics *m, const char *path_json,
                      const char *scenario, uint64_t seed)
{
    FILE *fp = fopen(path_json, "w");
    if (!fp) {
        fprintf(stderr, "[metrics] cannot open %s\n", path_json);
        return;
    }
    fprintf(fp, "{\n");
    fprintf(fp, "  \"scenario\": \"%s\",\n", scenario ? scenario : "");
    fprintf(fp, "  \"seed\": %llu,\n", (unsigned long long)seed);
    fprintf(fp, "  \"duration_ns\": %llu,\n",
            (unsigned long long)(m->session_ns_end - m->session_ns_start));
    fprintf(fp, "  \"success_count\": %llu,\n", (unsigned long long)m->success_count);
    fprintf(fp, "  \"drop_count\": %llu,\n",    (unsigned long long)m->drop_count);
    fprintf(fp, "  \"verify_fail_count\": %llu,\n",
            (unsigned long long)m->verify_fail_count);
    fprintf(fp, "  \"attacks_injected\": %llu,\n",
            (unsigned long long)m->attacks_injected);
    fprintf(fp, "  \"attacks_detected\": %llu,\n",
            (unsigned long long)m->attacks_detected);
    fprintf(fp, "  \"attacks_delivered\": %llu,\n",
            (unsigned long long)m->attacks_delivered);
    fprintf(fp, "  \"tx_bytes_total\": %llu,\n", (unsigned long long)m->tx_bytes_total);
    fprintf(fp, "  \"rx_bytes_total\": %llu,\n", (unsigned long long)m->rx_bytes_total);

    fprintf(fp, "  \"deadline_miss\": {\n");
    for (int i = 1; i <= 6; ++i) {
        fprintf(fp, "    \"D%d\": {\"miss\": %llu, \"total\": %llu}%s\n",
                i,
                (unsigned long long)m->deadline_miss_count[i],
                (unsigned long long)m->deadline_total_count[i],
                (i == 6) ? "" : ",");
    }
    fprintf(fp, "  },\n");

    fprintf(fp, "  \"histograms\": {\n");
    dump_hist(fp, "e2e_latency",   &m->e2e_latency);   fprintf(fp, ",\n");
    dump_hist(fp, "secoc_auth",    &m->secoc_auth);    fprintf(fp, ",\n");
    dump_hist(fp, "secoc_verify",  &m->secoc_verify);  fprintf(fp, ",\n");
    dump_hist(fp, "cantp",         &m->cantp);         fprintf(fp, ",\n");
    dump_hist(fp, "bus_transit",   &m->bus_transit);   fprintf(fp, ",\n");
    dump_hist(fp, "pdu_bytes",     &m->pdu_bytes);     fprintf(fp, ",\n");
    dump_hist(fp, "fragments",     &m->fragments);     fprintf(fp, "\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");
    fclose(fp);
}
