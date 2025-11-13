/**
 * @file test_performance.c
 * @brief Performance benchmark for MAC vs PQC signatures
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "PQC.h"
#include "Csm.h"
#include "encrypt.h"

#define NUM_ITERATIONS 100

double get_time_ms(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

int main(void)
{
    printf("========================================\n");
    printf("SecOC Performance: MAC vs PQC\n");
    printf("========================================\n\n");

    const char* test_data = "AUTOSAR SecOC Test Data for Performance Benchmarking";
    uint32 data_len = strlen(test_data);

    /* Initialize PQC */
    PQC_Init();

    printf("Test Configuration:\n");
    printf("  Data size: %u bytes\n", data_len);
    printf("  Iterations: %d\n\n", NUM_ITERATIONS);

    /* ===== MAC Performance ===== */
    printf("Classic MAC (HMAC) Performance:\n");

    uint8 mac[32];
    uint32 mac_len;
    double mac_gen_start = get_time_ms();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Csm_MacGenerate(0, 0, (const uint8*)test_data, data_len, mac, &mac_len);
    }

    double mac_gen_time = get_time_ms() - mac_gen_start;
    double mac_gen_avg = mac_gen_time / NUM_ITERATIONS;

    Crypto_VerifyResultType verify_result;
    double mac_ver_start = get_time_ms();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Csm_MacVerify(0, 0, (const uint8*)test_data, data_len, mac, mac_len * 8, &verify_result);
    }

    double mac_ver_time = get_time_ms() - mac_ver_start;
    double mac_ver_avg = mac_ver_time / NUM_ITERATIONS;

    printf("  MAC Generation:   %.3f ms/op (Total: %.2f ms)\n", mac_gen_avg, mac_gen_time);
    printf("  MAC Verification: %.3f ms/op (Total: %.2f ms)\n", mac_ver_avg, mac_ver_time);
    printf("  MAC Size: %u bytes\n\n", mac_len);

    /* ===== PQC Performance ===== */
    printf("Post-Quantum (ML-DSA-65) Performance:\n");

    uint8 signature[4096];
    uint32 sig_len;
    double pqc_gen_start = get_time_ms();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Csm_SignatureGenerate(0, 0, (const uint8*)test_data, data_len, signature, &sig_len);
    }

    double pqc_gen_time = get_time_ms() - pqc_gen_start;
    double pqc_gen_avg = pqc_gen_time / NUM_ITERATIONS;

    double pqc_ver_start = get_time_ms();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Csm_SignatureVerify(0, 0, (const uint8*)test_data, data_len, signature, sig_len, &verify_result);
    }

    double pqc_ver_time = get_time_ms() - pqc_ver_start;
    double pqc_ver_avg = pqc_ver_time / NUM_ITERATIONS;

    printf("  Signature Generation:   %.3f ms/op (Total: %.2f ms)\n", pqc_gen_avg, pqc_gen_time);
    printf("  Signature Verification: %.3f ms/op (Total: %.2f ms)\n", pqc_ver_avg, pqc_ver_time);
    printf("  Signature Size: %u bytes\n\n", sig_len);

    /* ===== Comparison ===== */
    printf("========================================\n");
    printf("Performance Comparison:\n");
    printf("========================================\n");
    printf("  Generation:\n");
    printf("    PQC is %.1fx slower than MAC\n", pqc_gen_avg / mac_gen_avg);
    printf("  Verification:\n");
    printf("    PQC is %.1fx slower than MAC\n", pqc_ver_avg / mac_ver_avg);
    printf("  Size:\n");
    printf("    PQC signature is %.1fx larger than MAC\n", (double)sig_len / mac_len);
    printf("\n");

    /* ===== Throughput ===== */
    printf("Throughput Analysis:\n");
    printf("  MAC:  %.0f ops/sec (gen), %.0f ops/sec (verify)\n",
           1000.0 / mac_gen_avg, 1000.0 / mac_ver_avg);
    printf("  PQC:  %.0f ops/sec (gen), %.0f ops/sec (verify)\n",
           1000.0 / pqc_gen_avg, 1000.0 / pqc_ver_avg);
    printf("\n");

    printf("========================================\n");
    printf("Conclusion:\n");
    printf("========================================\n");
    if (pqc_gen_avg < 10.0) {
        printf("  PQC performance is ACCEPTABLE for Ethernet Gateway\n");
        printf("  (< 10ms per signature operation)\n");
    } else if (pqc_gen_avg < 50.0) {
        printf("  PQC performance is MARGINAL\n");
        printf("  (10-50ms per signature operation)\n");
    } else {
        printf("  PQC performance may be TOO SLOW\n");
        printf("  (> 50ms per signature operation)\n");
    }

    return 0;
}
