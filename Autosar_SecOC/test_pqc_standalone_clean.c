/********************************************************************************************************/
/****************************************** PQC STANDALONE TEST ******************************************/
/********************************************************************************************************/
/**
 * @file test_pqc_standalone.c
 * @brief Standalone PQC Algorithm Testing (ML-KEM-768 & ML-DSA-65)
 * @details Tests post-quantum cryptography algorithms in isolation WITHOUT AUTOSAR integration
 *
 * Test Coverage:
 * [OK] Phase 1: ML-KEM-768 (Key Encapsulation Mechanism)
 *    - Key generation (1000 iterations)
 *    - Encapsulation (1000 iterations)
 *    - Decapsulation (1000 iterations)
 *    - Shared secret correctness verification
 *
 * [OK] Phase 2: ML-DSA-65 (Digital Signature Algorithm)
 *    - Key generation (1000 iterations)
 *    - Signing with 5 message sizes: 8, 64, 256, 512, 1024 bytes
 *    - Verification (1000 iterations per size)
 *    - Signature validity testing
 *
 * [OK] Metrics Collected:
 *    - Execution time (min/max/avg/stddev in microseconds)
 *    - Throughput (operations per second)
 *    - Memory usage (key sizes, signature sizes)
 *    - Correctness rate (%)
 *    - Success/failure counts
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
#include "PQC_KeyExchange.h"

/********************************************************************************************************/
/********************************************* CONSTANTS ************************************************/
/********************************************************************************************************/

#define NUM_ITERATIONS   1000      // Benchmark iterations
#define NUM_WARMUP       50        // Warmup iterations
#define NUM_MESSAGE_SIZES 5        // Different message sizes for ML-DSA

/********************************************************************************************************/
/********************************************* METRICS STRUCTURES ***************************************/
/********************************************************************************************************/

typedef struct {
    // ML-KEM-768 Key Generation Metrics
    double keygen_time_avg;
    double keygen_time_min;
    double keygen_time_max;
    double keygen_time_stddev;
    double keygen_throughput;
    uint32 keygen_success_count;

    // ML-KEM-768 Encapsulation Metrics
    double encaps_time_avg;
    double encaps_time_min;
    double encaps_time_max;
    double encaps_time_stddev;
    double encaps_throughput;
    uint32 encaps_success_count;

    // ML-KEM-768 Decapsulation Metrics
    double decaps_time_avg;
    double decaps_time_min;
    double decaps_time_max;
    double decaps_time_stddev;
    double decaps_throughput;
    uint32 decaps_success_count;

    // Size Metrics
    uint32 public_key_size;
    uint32 secret_key_size;
    uint32 ciphertext_size;
    uint32 shared_secret_size;

    // Correctness Metrics
    uint32 shared_secret_match_count;
    double correctness_rate;
    uint32 total_iterations;
} MLKEM_Metrics;

typedef struct {
    uint32 message_size;

    // Key Generation (measured once)
    double keygen_time_avg;
    double keygen_time_min;
    double keygen_time_max;
    double keygen_time_stddev;
    double keygen_throughput;
    uint32 keygen_success_count;

    // Signing Metrics
    double sign_time_avg;
    double sign_time_min;
    double sign_time_max;
    double sign_time_stddev;
    double sign_throughput;
    uint32 sign_success_count;

    // Verification Metrics
    double verify_time_avg;
    double verify_time_min;
    double verify_time_max;
    double verify_time_stddev;
    double verify_throughput;
    uint32 verify_success_count;

    // Size Metrics
    uint32 public_key_size;
    uint32 secret_key_size;
    uint32 signature_size;

    // Correctness Metrics
    uint32 valid_signature_count;
    double correctness_rate;
    uint32 total_iterations;
} MLDSA_Metrics;

/********************************************************************************************************/
/********************************************* GLOBAL VARIABLES *****************************************/
/********************************************************************************************************/

static MLKEM_Metrics g_mlkem_metrics = {0};
static MLDSA_Metrics g_mldsa_metrics[NUM_MESSAGE_SIZES] = {0};

static const uint32 test_msg_sizes[NUM_MESSAGE_SIZES] = {8, 64, 256, 512, 1024};

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

    // Calculate standard deviation
    double variance = 0.0;
    for (uint32 i = 0; i < count; i++) {
        double diff = times[i] - *avg;
        variance += diff * diff;
    }
    *stddev = sqrt(variance / count);
}

/********************************************************************************************************/
/*********************************** ML-KEM-768 TESTS ***************************************************/
/********************************************************************************************************/

void test_mlkem_keygen(void) {
    printf("+-----------------------------------------------------------+\n");
    printf("|  Testing ML-KEM-768 Key Generation                       |\n");
    printf("+-----------------------------------------------------------+\n");

    double times[NUM_ITERATIONS];
    PQC_MLKEM_KeyPairType keypair;

    /* Warmup */
    printf("  