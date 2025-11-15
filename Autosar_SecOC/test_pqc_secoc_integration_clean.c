/********************************************************************************************************/
/************************************** PQC SECOC INTEGRATION TEST **************************************/
/********************************************************************************************************/
/**
 * @file test_pqc_secoc_integration.c
 * @brief PQC Integration with AUTOSAR SecOC Testing
 * @details Tests post-quantum cryptography integrated with AUTOSAR Secure Onboard Communication
 *
 * Test Coverage:
 * [OK] Phase 1: Csm Layer Integration
 *    - Csm_SignatureGenerate (wraps PQC_MLDSA_Sign)
 *    - Csm_SignatureVerify (wraps PQC_MLDSA_Verify)
 *    - Csm_MacGenerate (classical AES-CMAC)
 *    - Csm_MacVerify (classical verification)
 *
 * [OK] Phase 2: Performance Comparison
 *    - PQC Signature vs Classical MAC
 *    - Time overhead analysis
 *    - Size overhead analysis
 *    - Throughput comparison
 *
 * [OK] Phase 3: Security Attack Simulation
 *    - Tampering detection with PQC
 *    - Replay attack detection
 *    - Freshness validation
 *
 * [OK] Metrics Collected:
 *    - End-to-end latency through Csm layer
 *    - Signature generation time
 *    - Verification time
 *    - Secured PDU size overhead
 *    - Correctness and security validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
    #include <unistd.h>
#endif

#include "Std_Types.h"
#include "PQC.h"
#include "Csm.h"

/********************************************************************************************************/
/********************************************* CONSTANTS ************************************************/
/********************************************************************************************************/

#define NUM_ITERATIONS   100       // Integration test iterations (less than standalone)
#define NUM_WARMUP       10        // Warmup iterations
#define MAX_MESSAGE_SIZE 1024      // Maximum message size

/********************************************************************************************************/
/********************************************* METRICS STRUCTURES ***************************************/
/********************************************************************************************************/

typedef struct {
    // Generation Metrics
    double gen_time_avg;
    double gen_time_min;
    double gen_time_max;
    double gen_time_stddev;
    double gen_throughput;
    uint32 gen_success_count;

    // Verification Metrics
    double verify_time_avg;
    double verify_time_min;
    double verify_time_max;
    double verify_time_stddev;
    double verify_throughput;
    uint32 verify_success_count;

    // Size Metrics
    uint32 authenticator_size;
    uint32 total_iterations;

    // Security Metrics
    uint32 tampering_detected;
    uint32 tampering_missed;
} CryptoMetrics;

/********************************************************************************************************/
/********************************************* GLOBAL VARIABLES *****************************************/
/********************************************************************************************************/

static CryptoMetrics g_pqc_metrics = {0};
static CryptoMetrics g_mac_metrics = {0};

/********************************************************************************************************/
/********************************************* TIME MEASUREMENT *****************************************/
/********************************************************************************************************/

static inline uint64_t get_time_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

static inline double ns_to_us(uint64_t ns) {
    return (double)ns / 1000.0;
}

/********************************************************************************************************/
/********************************************* STATISTICS ***********************************************/
/********************************************************************************************************/

void calculate_stats(double* times, uint32 count, double* min, double* max,
                    double* avg, double* stddev) {
    *min = times[0];
    *max = times[0];
    double sum = 0.0;

    for (uint32 i = 0; i < count; i++) {
        if (times[i] < *min) *min = times[i];
        if (times[i] > *max) *max = times[i];
        sum += times[i];
    }

    *avg = sum / count;

    double variance = 0.0;
    for (uint32 i = 0; i < count; i++) {
        double diff = times[i] - *avg;
        variance += diff * diff;
    }
    *stddev = sqrt(variance / count);
}

/********************************************************************************************************/
/*********************************** CSM LAYER TESTS (PQC) **********************************************/
/********************************************************************************************************/

void test_csm_pqc_signature(void) {
    printf("+---------------------------------------------------------+\n");
    printf("|  Testing Csm PQC Signature Generation & Verification   |\n");
    printf("+---------------------------------------------------------+\n");

    double gen_times[NUM_ITERATIONS];
    double verify_times[NUM_ITERATIONS];

    /* Test message (typical SecOC Data-to-Authenticator) */
    uint8_t message[256];
    for (uint32 i = 0; i < sizeof(message); i++) {
        message[i] = (uint8_t)(i & 0xFF);
    }

    uint8_t signature[PQC_MLDSA_SIGNATURE_BYTES];
    uint32 signature_len;
    Crypto_VerifyResultType verify_result;

    g_pqc_metrics.total_iterations = NUM_ITERATIONS;

    /* Warmup */
    printf("  