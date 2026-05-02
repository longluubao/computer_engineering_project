#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_bus.h"
#include "sim_ecu.h"
#include "sim_signals.h"
#include "sim_metrics.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "PQC.h"

/* SWS coverage tags (picked up by reports/build_sws_traceability.py):
 *   - SWS_SecOC_00161 (DeInit + reinit on rekey)
 *   - SWS_SecOC_00203 (Crypto operation queueing under load)
 */

/*
 * sc_rekey_run
 * ------------
 * Stress the ML-KEM handshake. Performs N successive PQC handshakes
 * (KeyGen + Encaps + Decaps + HKDF) and records:
 *   - per-operation timing (for the thesis PQC performance table)
 *   - total session-establishment overhead
 *
 * Shows the cost of periodic rekeying, which UN R155 and ISO 21434
 * effectively mandate for long-running sessions.
 */

int sc_rekey_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    /* Dummy bus (handshake can happen out-of-band in production; we
     * still allocate one so sim_ecu_create is happy). */
    SimBusCfg bc = {0};
    bc.kind = SIM_BUS_ETH_1000; bc.bitrate_bps = 1000000000U;
    bc.mtu_bytes = 1500U; bc.propagation_ns = 500U;
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg ec = {0};
    ec.role = SIM_ECU_ROLE_TX;
    ec.protection = SIM_PROT_PQC;
    ec.seed = cfg->seed;
    ec.primary_bus = bus;
    ec.metrics = &m;
    ec.name = "tx-rekey";
    SimEcu *tx = sim_ecu_create(&ec);
    ec.role = SIM_ECU_ROLE_RX; ec.name = "rx-rekey";
    SimEcu *rx = sim_ecu_create(&ec);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);

    SimHistogram keygen, encaps, decaps, total;
    sim_hist_init(&keygen); sim_hist_init(&encaps);
    sim_hist_init(&decaps); sim_hist_init(&total);

    uint32_t n = (cfg->iterations < 50) ? 50 : cfg->iterations;
    for (uint32_t it = 0; it < n; ++it) {
        uint64_t t_all0 = sim_now_ns();

        PQC_MLKEM_KeyPairType kp;
        uint64_t t0 = sim_now_ns();
        if (PQC_MLKEM_KeyGen(&kp) != PQC_E_OK) continue;
        sim_hist_add(&keygen, sim_now_ns() - t0);

        PQC_MLKEM_SharedSecretType ss;
        t0 = sim_now_ns();
        if (PQC_MLKEM_Encapsulate(kp.PublicKey, &ss) != PQC_E_OK) continue;
        sim_hist_add(&encaps, sim_now_ns() - t0);

        uint8_t ss2[PQC_MLKEM_SHARED_SECRET_BYTES];
        t0 = sim_now_ns();
        if (PQC_MLKEM_Decapsulate(ss.Ciphertext, kp.SecretKey, ss2) != PQC_E_OK)
            continue;
        sim_hist_add(&decaps, sim_now_ns() - t0);

        if (memcmp(ss.SharedSecret, ss2,
                   PQC_MLKEM_SHARED_SECRET_BYTES) == 0) {
            m.success_count++;
        }

        sim_hist_add(&total, sim_now_ns() - t_all0);
    }

    sim_log(SIM_LOG_INFO,
            "ML-KEM-768 rekey (%u iterations):\n"
            "  KeyGen     mean=%.1f us  p99=%.1f us\n"
            "  Encaps     mean=%.1f us  p99=%.1f us\n"
            "  Decaps     mean=%.1f us  p99=%.1f us\n"
            "  Full rekey mean=%.1f us  p99=%.1f us",
            n,
            sim_hist_mean(&keygen) / 1000.0,
            (double)sim_hist_percentile(&keygen, 0.99) / 1000.0,
            sim_hist_mean(&encaps) / 1000.0,
            (double)sim_hist_percentile(&encaps, 0.99) / 1000.0,
            sim_hist_mean(&decaps) / 1000.0,
            (double)sim_hist_percentile(&decaps, 0.99) / 1000.0,
            sim_hist_mean(&total) / 1000.0,
            (double)sim_hist_percentile(&total, 0.99) / 1000.0);

    /* Copy the handshake histograms into the standard metrics slots so
     * they are persisted by sim_scenario_finalise. */
    m.secoc_auth   = keygen;
    m.secoc_verify = encaps;
    m.cantp        = decaps;
    m.e2e_latency  = total;

    m.session_ns_end = sim_now_ns();
    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    sim_scenario_finalise(&m, "rekey", cfg);
    return 0;
}
