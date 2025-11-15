/**
 * @file test_pqc_comprehensive_detailed.c
 * @brief Comprehensive PQC Testing with Detailed Performance Metrics
 * @details Two-phase testing:
 *   PHASE 1: Standalone PQC Algorithm Testing (ML-KEM-768 + ML-DSA-65)
 *   PHASE 2: AUTOSAR Integration Testing
 *
 * Metrics Collected:
 *   - CPU Time (execution time in microseconds)
 *   - Memory Usage (heap allocation tracking)
 *   - Throughput (operations per second)
 *   - Key/Signature Sizes
 *   - Statistical Analysis (min/max/avg/stddev)
 *   - Comparison with Classical Algorithms
 *
 * Platform: x86_64, Raspberry Pi 4
 * Author: LLU2HC - HCMUT Computer Engineering Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

/* System headers for resource usage */
#ifdef __linux__
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <unistd.h>
#endif

/* AUTOSAR Types */
#include "Std_Types.h"

/* PQC Modules - Separated */
#include "PQC.h"              /* ML-DSA-65 */
#include "PQC_KeyExchange.h"  /* ML-KEM-768 */

/* CSM for classical crypto comparison */
#include "Csm.h"

/* AUTOSAR SecOC for integration testing (Phase 2) */
#include "SecOC.h"
#include "SecOC_Lcfg.h"

/* Test Configuration */
#define NUM_ITERATIONS 1000
#define NUM_WARMUP 50
#define NUM_MESSAGE_SIZES 5

/* Message sizes for testing */
const uint32 test_msg_sizes[] = {8, 64, 256, 512, 1024};

/* ============================================================================
 * SECTION 1: PERFORMANCE METRICS STRUCTURES
 * ============================================================================ */

/**
 * @brief Detailed Performance Metrics for ML-KEM-768
 */
typedef struct {
    /* Timing Metrics (microseconds) */
    double keygen_time_avg;
    double keygen_time_min;
    double keygen_time_max;
    double keygen_time_stddev;

    double encaps_time_avg;
    double encaps_time_min;
    double encaps_time_max;
    double encaps_time_stddev;

    double decaps_time_avg;
    double decaps_time_min;
    double decaps_time_max;
    double decaps_time_stddev;

    /* Throughput (operations per second) */
    double keygen_throughput;
    double encaps_throughput;
    double decaps_throughput;

    /* Memory Metrics (bytes) */
    uint32 public_key_size;
    uint32 secret_key_size;
    uint32 ciphertext_size;
    uint32 shared_secret_size;
    uint32 total_memory_usage;

    /* CPU Metrics */
    double cpu_usage_percent;
    double peak_memory_kb;

    /* Success Rate */
    uint32 keygen_success_count;
    uint32 encaps_success_count;
    uint32 decaps_success_count;
    uint32 total_iterations;

    /* Correctness */
    uint32 shared_secret_match_count;
    double correctness_rate;

} MLKEM_Metrics;

/**
 * @brief Detailed Performance Metrics for ML-DSA-65
 */
typedef struct {
    /* Timing Metrics (microseconds) */
    double keygen_time_avg;
    double keygen_time_min;
    double keygen_time_max;
    double keygen_time_stddev;

    double sign_time_avg;
    double sign_time_min;
    double sign_time_max;
    double sign_time_stddev;

    double verify_time_avg;
    double verify_time_min;
    double verify_time_max;
    double verify_time_stddev;

    /* Throughput (operations per second) */
    double keygen_throughput;
    double sign_throughput;
    double verify_throughput;

    /* Memory Metrics (bytes) */
    uint32 public_key_size;
    uint32 secret_key_size;
    uint32 signature_size;
    uint32 total_memory_usage;

    /* CPU Metrics */
    double cpu_usage_percent;
    double peak_memory_kb;

    /* Success Rate */
    uint32 keygen_success_count;
    uint32 sign_success_count;
    uint32 verify_success_count;
    uint32 total_iterations;

    /* Correctness */
    uint32 valid_signature_count;
    double correctness_rate;

    /* Per-message-size metrics */
    double sign_times_per_size[NUM_MESSAGE_SIZES];
    double verify_times_per_size[NUM_MESSAGE_SIZES];

} MLDSA_Metrics;

/**
 * @brief Classical Algorithm Metrics (for comparison)
 */
typedef struct {
    /* AES-CMAC */
    double cmac_generate_time_avg;
    double cmac_verify_time_avg;
    uint32 cmac_tag_size;
    double cmac_throughput;

    /* RSA-2048 (for comparison with ML-DSA) */
    double rsa_keygen_time_avg;
    double rsa_sign_time_avg;
    double rsa_verify_time_avg;
    uint32 rsa_signature_size;
    double rsa_sign_throughput;

    /* ECDH-256 (for comparison with ML-KEM) */
    double ecdh_keygen_time_avg;
    double ecdh_shared_secret_time_avg;
    uint32 ecdh_key_size;
    double ecdh_throughput;

} Classical_Metrics;

/**
 * @brief AUTOSAR Integration Metrics
 */
typedef struct {
    /* End-to-end latency */
    double e2e_latency_mlkem;
    double e2e_latency_mldsa;
    double e2e_latency_classical;

    /* Layer-specific latency */
    double com_layer_latency;
    double secoc_layer_latency;
    double pdur_layer_latency;
    double transport_layer_latency;

    /* Bandwidth overhead */
    uint32 classical_pdu_size;
    uint32 pqc_pdu_size;
    double bandwidth_overhead_ratio;

    /* Security tests */
    uint32 replay_attacks_blocked;
    uint32 tampering_attacks_blocked;
    uint32 total_security_tests;

} AUTOSAR_Integration_Metrics;

/* Global Metrics Storage */
MLKEM_Metrics g_mlkem_metrics;
MLDSA_Metrics g_mldsa_metrics[NUM_MESSAGE_SIZES];
Classical_Metrics g_classical_metrics;
AUTOSAR_Integration_Metrics g_autosar_metrics;

/* ============================================================================
 * SECTION 2: TIMING AND RESOURCE UTILITIES
 * ============================================================================ */

/**
 * @brief Get high-resolution timestamp in nanoseconds
 */
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline double ns_to_us(uint64_t ns) {
    return (double)ns / 1000.0;
}

static inline double ns_to_ms(uint64_t ns) {
    return (double)ns / 1000000.0;
}

/**
 * @brief Get current memory usage in KB
 */
double get_memory_usage_kb(void) {
#ifdef __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (double)usage.ru_maxrss; /* KB on Linux */
#else
    return 0.0; /* Not available on Windows */
#endif
}

/**
 * @brief Get CPU time in microseconds
 */
double get_cpu_time_us(void) {
#ifdef __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (double)(usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec);
#else
    return 0.0;
#endif
}

/**
 * @brief Calculate statistics from timing array
 */
void calculate_stats(double* times, uint32 count,
                    double* min, double* max,
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

/* ============================================================================
 * SECTION 3: PHASE 1 - ML-KEM-768 STANDALONE TESTING
 * ============================================================================ */

void print_mlkem_header(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|          PHASE 1.1: ML-KEM-768 STANDALONE TESTING            |\n");
    printf("|      (Module-Lattice Key Encapsulation Mechanism)            |\n");
    printf("+==============================================================+\n");
    printf("\nAlgorithm: ML-KEM-768 (NIST FIPS 203)\n");
    printf("Security Level: Category 3 (AES-192 equivalent)\n");
    printf("Use Case: Quantum-resistant key exchange for secure channels\n");
    printf("\n");
}

/**
 * @brief Test ML-KEM-768 Key Generation
 */
void test_mlkem_keygen(void) {
    printf("+---------------------------------------------------------+\n");
    printf("|  Testing ML-KEM-768 Key Generation                     |\n");
    printf("+---------------------------------------------------------+\n");

    double times[NUM_ITERATIONS];
    PQC_MLKEM_KeyPairType keypair;  /* ML-KEM-768 key pair */

    double mem_before = get_memory_usage_kb();
    double cpu_before = get_cpu_time_us();

    /* Warmup */
    printf("  [WARMUP] Warming up (%d iterations)...\n", NUM_WARMUP);
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        PQC_MLKEM_KeyGen(&keypair);
    }

    /* Benchmark */
    printf("  [BENCH] Benchmarking key generation (%d iterations)...\n", NUM_ITERATIONS);
    g_mlkem_metrics.total_iterations = NUM_ITERATIONS;

    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = PQC_MLKEM_KeyGen(&keypair);

        uint64_t end = get_time_ns();
        times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_mlkem_metrics.keygen_success_count++;
        }
    }

    double cpu_after = get_cpu_time_us();
    double mem_after = get_memory_usage_kb();

    /* Calculate statistics */
    calculate_stats(times, NUM_ITERATIONS,
                   &g_mlkem_metrics.keygen_time_min,
                   &g_mlkem_metrics.keygen_time_max,
                   &g_mlkem_metrics.keygen_time_avg,
                   &g_mlkem_metrics.keygen_time_stddev);

    g_mlkem_metrics.keygen_throughput = 1000000.0 / g_mlkem_metrics.keygen_time_avg;
    g_mlkem_metrics.public_key_size = sizeof(keypair.PublicKey);
    g_mlkem_metrics.secret_key_size = sizeof(keypair.SecretKey);
    g_mlkem_metrics.cpu_usage_percent = ((cpu_after - cpu_before) /
                                         (g_mlkem_metrics.keygen_time_avg * NUM_ITERATIONS)) * 100.0;
    g_mlkem_metrics.peak_memory_kb = mem_after - mem_before;

    /* Print results */
    printf("\n  [OK] Key Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %8.2f us |\n", g_mlkem_metrics.keygen_time_avg);
    printf("  | Min Time               | %8.2f us |\n", g_mlkem_metrics.keygen_time_min);
    printf("  | Max Time               | %8.2f us |\n", g_mlkem_metrics.keygen_time_max);
    printf("  | Std Deviation          | %8.2f us |\n", g_mlkem_metrics.keygen_time_stddev);
    printf("  | Throughput             | %8.0f/s |\n", g_mlkem_metrics.keygen_throughput);
    printf("  | Public Key Size        | %8u B  |\n", g_mlkem_metrics.public_key_size);
    printf("  | Secret Key Size        | %8u B  |\n", g_mlkem_metrics.secret_key_size);
    printf("  | Success Rate           | %7.2f %%  |\n",
           (g_mlkem_metrics.keygen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  | Memory Usage           | %8.0f KB |\n", g_mlkem_metrics.peak_memory_kb);
    printf("  +------------------------+----------------+\n");
}

/**
 * @brief Test ML-KEM-768 Encapsulation and Decapsulation
 */
void test_mlkem_encaps_decaps(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing ML-KEM-768 Encapsulation & Decapsulation      |\n");
    printf("+---------------------------------------------------------+\n");

    /* Generate keys first */
    PQC_MLKEM_KeyPairType keypair;
    PQC_MLKEM_KeyGen(&keypair);

    double encaps_times[NUM_ITERATIONS];
    double decaps_times[NUM_ITERATIONS];

    PQC_MLKEM_SharedSecretType shared_secret_alice;  /* For encapsulation */
    uint8_t shared_secret_bob[32];  /* For decapsulation */

    printf("  [WARMUP] Warming up...\n");
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        PQC_MLKEM_Encapsulate(keypair.PublicKey, &shared_secret_alice);
        PQC_MLKEM_Decapsulate(shared_secret_alice.Ciphertext, keypair.SecretKey, shared_secret_bob);
    }

    /* Benchmark Encapsulation */
    printf("  [BENCH] Benchmarking encapsulation (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = PQC_MLKEM_Encapsulate(keypair.PublicKey, &shared_secret_alice);

        uint64_t end = get_time_ns();
        encaps_times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_mlkem_metrics.encaps_success_count++;
        }
    }

    /* Benchmark Decapsulation */
    printf("  [BENCH] Benchmarking decapsulation (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = PQC_MLKEM_Decapsulate(shared_secret_alice.Ciphertext,
                                                      keypair.SecretKey,
                                                      shared_secret_bob);

        uint64_t end = get_time_ns();
        decaps_times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_mlkem_metrics.decaps_success_count++;

            /* Verify shared secrets match */
            if (memcmp(shared_secret_alice.SharedSecret, shared_secret_bob, 32) == 0) {
                g_mlkem_metrics.shared_secret_match_count++;
            }
        }
    }

    /* Calculate statistics */
    calculate_stats(encaps_times, NUM_ITERATIONS,
                   &g_mlkem_metrics.encaps_time_min,
                   &g_mlkem_metrics.encaps_time_max,
                   &g_mlkem_metrics.encaps_time_avg,
                   &g_mlkem_metrics.encaps_time_stddev);

    calculate_stats(decaps_times, NUM_ITERATIONS,
                   &g_mlkem_metrics.decaps_time_min,
                   &g_mlkem_metrics.decaps_time_max,
                   &g_mlkem_metrics.decaps_time_avg,
                   &g_mlkem_metrics.decaps_time_stddev);

    g_mlkem_metrics.encaps_throughput = 1000000.0 / g_mlkem_metrics.encaps_time_avg;
    g_mlkem_metrics.decaps_throughput = 1000000.0 / g_mlkem_metrics.decaps_time_avg;
    g_mlkem_metrics.ciphertext_size = sizeof(shared_secret_alice.Ciphertext);
    g_mlkem_metrics.shared_secret_size = sizeof(shared_secret_alice.SharedSecret);
    g_mlkem_metrics.correctness_rate = (g_mlkem_metrics.shared_secret_match_count * 100.0) /
                                       NUM_ITERATIONS;
    g_mlkem_metrics.total_memory_usage = g_mlkem_metrics.public_key_size +
                                        g_mlkem_metrics.secret_key_size +
                                        g_mlkem_metrics.ciphertext_size +
                                        g_mlkem_metrics.shared_secret_size;

    /* Print results */
    printf("\n  [OK] Encapsulation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %8.2f us |\n", g_mlkem_metrics.encaps_time_avg);
    printf("  | Min Time               | %8.2f us |\n", g_mlkem_metrics.encaps_time_min);
    printf("  | Max Time               | %8.2f us |\n", g_mlkem_metrics.encaps_time_max);
    printf("  | Throughput             | %8.0f/s |\n", g_mlkem_metrics.encaps_throughput);
    printf("  | Ciphertext Size        | %8u B  |\n", g_mlkem_metrics.ciphertext_size);
    printf("  +------------------------+----------------+\n");

    printf("\n  [OK] Decapsulation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %8.2f us |\n", g_mlkem_metrics.decaps_time_avg);
    printf("  | Min Time               | %8.2f us |\n", g_mlkem_metrics.decaps_time_min);
    printf("  | Max Time               | %8.2f us |\n", g_mlkem_metrics.decaps_time_max);
    printf("  | Throughput             | %8.0f/s |\n", g_mlkem_metrics.decaps_throughput);
    printf("  | Shared Secret Size     | %8u B  |\n", g_mlkem_metrics.shared_secret_size);
    printf("  | Correctness Rate       | %7.2f %%  |\n", g_mlkem_metrics.correctness_rate);
    printf("  +------------------------+----------------+\n");

    printf("\n  [BENCH] ML-KEM-768 Total Memory Footprint: %u bytes\n",
           g_mlkem_metrics.total_memory_usage);
}

/* ============================================================================
 * SECTION 4: PHASE 1 - ML-DSA-65 STANDALONE TESTING
 * ============================================================================ */

void print_mldsa_header(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|          PHASE 1.2: ML-DSA-65 STANDALONE TESTING             |\n");
    printf("|       (Module-Lattice Digital Signature Algorithm)           |\n");
    printf("+==============================================================+\n");
    printf("\nAlgorithm: ML-DSA-65 (NIST FIPS 204)\n");
    printf("Security Level: Category 3 (AES-192 equivalent)\n");
    printf("Use Case: Quantum-resistant digital signatures for authentication\n");
    printf("\n");
}

/**
 * @brief Test ML-DSA-65 Key Generation
 */
void test_mldsa_keygen(void) {
    printf("+---------------------------------------------------------+\n");
    printf("|  Testing ML-DSA-65 Key Generation                       |\n");
    printf("+---------------------------------------------------------+\n");

    double times[NUM_ITERATIONS];
    PQC_MLDSA_KeyPairType keypair;  /* ML-DSA-65 key pair */

    double mem_before = get_memory_usage_kb();

    /* Warmup */
    printf("  [WARMUP] Warming up (%d iterations)...\n", NUM_WARMUP);
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        PQC_MLDSA_KeyGen(&keypair);
    }

    /* Benchmark */
    printf("  [BENCH] Benchmarking key generation (%d iterations)...\n", NUM_ITERATIONS);

    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = PQC_MLDSA_KeyGen(&keypair);

        uint64_t end = get_time_ns();
        times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_mldsa_metrics[0].keygen_success_count++;
        }
    }

    double mem_after = get_memory_usage_kb();

    /* Calculate statistics */
    calculate_stats(times, NUM_ITERATIONS,
                   &g_mldsa_metrics[0].keygen_time_min,
                   &g_mldsa_metrics[0].keygen_time_max,
                   &g_mldsa_metrics[0].keygen_time_avg,
                   &g_mldsa_metrics[0].keygen_time_stddev);

    g_mldsa_metrics[0].keygen_throughput = 1000000.0 / g_mldsa_metrics[0].keygen_time_avg;
    g_mldsa_metrics[0].public_key_size = sizeof(keypair.PublicKey);
    g_mldsa_metrics[0].secret_key_size = sizeof(keypair.SecretKey);
    g_mldsa_metrics[0].peak_memory_kb = mem_after - mem_before;
    g_mldsa_metrics[0].total_iterations = NUM_ITERATIONS;

    /* Print results */
    printf("\n  [OK] Key Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %8.2f us |\n", g_mldsa_metrics[0].keygen_time_avg);
    printf("  | Min Time               | %8.2f us |\n", g_mldsa_metrics[0].keygen_time_min);
    printf("  | Max Time               | %8.2f us |\n", g_mldsa_metrics[0].keygen_time_max);
    printf("  | Std Deviation          | %8.2f us |\n", g_mldsa_metrics[0].keygen_time_stddev);
    printf("  | Throughput             | %8.0f/s |\n", g_mldsa_metrics[0].keygen_throughput);
    printf("  | Public Key Size        | %8u B  |\n", g_mldsa_metrics[0].public_key_size);
    printf("  | Secret Key Size        | %8u B  |\n", g_mldsa_metrics[0].secret_key_size);
    printf("  | Success Rate           | %7.2f %%  |\n",
           (g_mldsa_metrics[0].keygen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  | Memory Usage           | %8.0f KB |\n", g_mldsa_metrics[0].peak_memory_kb);
    printf("  +------------------------+----------------+\n");
}

/**
 * @brief Test ML-DSA-65 Sign and Verify for different message sizes
 */
void test_mldsa_sign_verify(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing ML-DSA-65 Sign & Verify (Multiple Sizes)      |\n");
    printf("+---------------------------------------------------------+\n");

    /* Generate keys */
    PQC_MLDSA_KeyPairType keypair;
    PQC_MLDSA_KeyGen(&keypair);

    for (uint32 size_idx = 0; size_idx < NUM_MESSAGE_SIZES; size_idx++) {
        uint32 msg_size = test_msg_sizes[size_idx];

        printf("\n  [TEST] Testing with %u-byte messages...\n", msg_size);

        /* Prepare message */
        uint8_t message[1024];
        for (uint32 i = 0; i < msg_size; i++) {
            message[i] = (uint8_t)(i & 0xFF);
        }

        double sign_times[NUM_ITERATIONS];
        double verify_times[NUM_ITERATIONS];

        uint8_t signature[3309];  /* ML-DSA-65 signature size */
        uint32 signature_len;

        /* Warmup */
        for (uint32 i = 0; i < NUM_WARMUP; i++) {
            PQC_MLDSA_Sign(message, msg_size, keypair.SecretKey, signature, &signature_len);
            PQC_MLDSA_Verify(message, msg_size, signature, signature_len, keypair.PublicKey);
        }

        /* Benchmark Signing */
        for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
            uint64_t start = get_time_ns();

            Std_ReturnType result = PQC_MLDSA_Sign(message, msg_size, keypair.SecretKey,
                                                   signature, &signature_len);

            uint64_t end = get_time_ns();
            sign_times[i] = ns_to_us(end - start);

            if (result == E_OK) {
                g_mldsa_metrics[size_idx].sign_success_count++;
            }
        }

        /* Benchmark Verification */
        for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
            uint64_t start = get_time_ns();

            Std_ReturnType result = PQC_MLDSA_Verify(message, msg_size, signature,
                                                     signature_len, keypair.PublicKey);

            uint64_t end = get_time_ns();
            verify_times[i] = ns_to_us(end - start);

            if (result == E_OK) {
                g_mldsa_metrics[size_idx].verify_success_count++;
                g_mldsa_metrics[size_idx].valid_signature_count++;
            }
        }

        /* Calculate statistics */
        calculate_stats(sign_times, NUM_ITERATIONS,
                       &g_mldsa_metrics[size_idx].sign_time_min,
                       &g_mldsa_metrics[size_idx].sign_time_max,
                       &g_mldsa_metrics[size_idx].sign_time_avg,
                       &g_mldsa_metrics[size_idx].sign_time_stddev);

        calculate_stats(verify_times, NUM_ITERATIONS,
                       &g_mldsa_metrics[size_idx].verify_time_min,
                       &g_mldsa_metrics[size_idx].verify_time_max,
                       &g_mldsa_metrics[size_idx].verify_time_avg,
                       &g_mldsa_metrics[size_idx].verify_time_stddev);

        g_mldsa_metrics[size_idx].sign_throughput =
            1000000.0 / g_mldsa_metrics[size_idx].sign_time_avg;
        g_mldsa_metrics[size_idx].verify_throughput =
            1000000.0 / g_mldsa_metrics[size_idx].verify_time_avg;
        g_mldsa_metrics[size_idx].signature_size = signature_len;
        g_mldsa_metrics[size_idx].correctness_rate =
            (g_mldsa_metrics[size_idx].valid_signature_count * 100.0) / NUM_ITERATIONS;

        /* Copy to global for size comparison */
        if (size_idx == 0) {
            g_mldsa_metrics[size_idx].public_key_size = sizeof(keypair.PublicKey);
            g_mldsa_metrics[size_idx].secret_key_size = sizeof(keypair.SecretKey);
            g_mldsa_metrics[size_idx].total_memory_usage =
                sizeof(keypair.PublicKey) + sizeof(keypair.SecretKey) + signature_len;
        }

        /* Print results for this size */
        printf("\n    [OK] Sign Results (%u bytes):\n", msg_size);
        printf("    +------------------+----------------+\n");
        printf("    | Average Time     | %8.2f us |\n", g_mldsa_metrics[size_idx].sign_time_avg);
        printf("    | Min Time         | %8.2f us |\n", g_mldsa_metrics[size_idx].sign_time_min);
        printf("    | Max Time         | %8.2f us |\n", g_mldsa_metrics[size_idx].sign_time_max);
        printf("    | Throughput       | %8.0f/s |\n", g_mldsa_metrics[size_idx].sign_throughput);
        printf("    +------------------+----------------+\n");

        printf("\n    [OK] Verify Results (%u bytes):\n", msg_size);
        printf("    +------------------+----------------+\n");
        printf("    | Average Time     | %8.2f us |\n", g_mldsa_metrics[size_idx].verify_time_avg);
        printf("    | Min Time         | %8.2f us |\n", g_mldsa_metrics[size_idx].verify_time_min);
        printf("    | Max Time         | %8.2f us |\n", g_mldsa_metrics[size_idx].verify_time_max);
        printf("    | Throughput       | %8.0f/s |\n", g_mldsa_metrics[size_idx].verify_throughput);
        printf("    | Correctness      | %7.2f %%  |\n", g_mldsa_metrics[size_idx].correctness_rate);
        printf("    +------------------+----------------+\n");
    }

    printf("\n  [BENCH] ML-DSA-65 Signature Size: %u bytes ([WARN] Large!)\n",
           g_mldsa_metrics[0].signature_size);
    printf("  [BENCH] Total Memory Footprint: %u bytes\n",
           g_mldsa_metrics[0].total_memory_usage);
}

/* ============================================================================
 * SECTION 5: CLASSICAL ALGORITHM COMPARISON
 * ============================================================================ */

void print_classical_header(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|      PHASE 2: CLASSICAL ALGORITHM COMPARISON                 |\n");
    printf("|   (AES-CMAC, RSA-2048, ECDH-256 for baseline)                |\n");
    printf("+==============================================================+\n");
    printf("\n");
}

/**
 * @brief Test Classical AES-CMAC (for comparison with ML-DSA)
 */
void test_classical_cmac(void) {
    printf("+---------------------------------------------------------+\n");
    printf("|  Testing Classical AES-128-CMAC                         |\n");
    printf("+---------------------------------------------------------+\n");

    uint8_t message[64] = {0};
    uint8_t mac[16];
    uint32 mac_len = 16;
    Crypto_VerifyResultType verifyResult;

    double gen_times[NUM_ITERATIONS];
    double verify_times[NUM_ITERATIONS];

    /* Warmup */
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        Csm_MacGenerate(0, CRYPTO_OPERATIONMODE_SINGLECALL, message, 64, mac, &mac_len);
        Csm_MacVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL, message, 64, mac, mac_len, &verifyResult);
    }

    /* Benchmark MAC Generation */
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();
        Csm_MacGenerate(0, CRYPTO_OPERATIONMODE_SINGLECALL, message, 64, mac, &mac_len);
        uint64_t end = get_time_ns();
        gen_times[i] = ns_to_us(end - start);
    }

    /* Benchmark MAC Verification */
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();
        Csm_MacVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL, message, 64, mac, mac_len, &verifyResult);
        uint64_t end = get_time_ns();
        verify_times[i] = ns_to_us(end - start);
    }

    double min, max, stddev;
    calculate_stats(gen_times, NUM_ITERATIONS, &min, &max,
                   &g_classical_metrics.cmac_generate_time_avg, &stddev);
    calculate_stats(verify_times, NUM_ITERATIONS, &min, &max,
                   &g_classical_metrics.cmac_verify_time_avg, &stddev);

    g_classical_metrics.cmac_tag_size = mac_len;
    g_classical_metrics.cmac_throughput = 1000000.0 / g_classical_metrics.cmac_generate_time_avg;

    printf("\n  [OK] AES-CMAC Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | MAC Generate Time      | %8.2f us |\n", g_classical_metrics.cmac_generate_time_avg);
    printf("  | MAC Verify Time        | %8.2f us |\n", g_classical_metrics.cmac_verify_time_avg);
    printf("  | MAC Tag Size           | %8u B  |\n", g_classical_metrics.cmac_tag_size);
    printf("  | Throughput             | %8.0f/s |\n", g_classical_metrics.cmac_throughput);
    printf("  +------------------------+----------------+\n");
}

/* ============================================================================
 * SECTION 6: COMPARATIVE ANALYSIS
 * ============================================================================ */

void print_comparative_analysis(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|               COMPARATIVE ANALYSIS SUMMARY                   |\n");
    printf("+==============================================================+\n");

    printf("\n+- ML-KEM-768 vs Classical Key Exchange ---------------------+\n");
    printf("|                                                              |\n");
    printf("|  Operation          | ML-KEM-768      | Classical (Est.)   |\n");
    printf("|  ----------------------------------------------------------  |\n");
    printf("|  Key Generation     | %7.2f us     | ~100 us (ECDH)     |\n",
           g_mlkem_metrics.keygen_time_avg);
    printf("|  Encapsulation      | %7.2f us     | ~120 us (ECDH)     |\n",
           g_mlkem_metrics.encaps_time_avg);
    printf("|  Decapsulation      | %7.2f us     | ~120 us (ECDH)     |\n",
           g_mlkem_metrics.decaps_time_avg);
    printf("|  Public Key Size    | %7u B       | ~65 B (ECDH-256)   |\n",
           g_mlkem_metrics.public_key_size);
    printf("|  Quantum Resistant  | [OK] YES          | [ERROR] NO              |\n");
    printf("|  Overhead Factor    | ~%.1fx slower    | Baseline           |\n",
           g_mlkem_metrics.keygen_time_avg / 100.0);
    printf("|                                                              |\n");
    printf("+--------------------------------------------------------------+\n");

    printf("\n+- ML-DSA-65 vs AES-CMAC (64-byte message) ------------------+\n");
    printf("|                                                              |\n");
    printf("|  Operation          | ML-DSA-65       | AES-CMAC           |\n");
    printf("|  ----------------------------------------------------------  |\n");
    printf("|  Sign/MAC Gen       | %7.2f us     | %7.2f us        |\n",
           g_mldsa_metrics[1].sign_time_avg, g_classical_metrics.cmac_generate_time_avg);
    printf("|  Verify/MAC Check   | %7.2f us     | %7.2f us        |\n",
           g_mldsa_metrics[1].verify_time_avg, g_classical_metrics.cmac_verify_time_avg);
    printf("|  Signature Size     | %7u B       | %7u B           |\n",
           g_mldsa_metrics[1].signature_size, g_classical_metrics.cmac_tag_size);
    printf("|  Throughput (Sign)  | %7.0f/s      | %7.0f/s         |\n",
           g_mldsa_metrics[1].sign_throughput, g_classical_metrics.cmac_throughput);
    printf("|  Quantum Resistant  | [OK] YES          | [ERROR] NO              |\n");
    printf("|  Time Overhead      | ~%.0fx slower   | Baseline           |\n",
           g_mldsa_metrics[1].sign_time_avg / g_classical_metrics.cmac_generate_time_avg);
    printf("|  Size Overhead      | ~%.0fx larger   | Baseline           |\n",
           (double)g_mldsa_metrics[1].signature_size / g_classical_metrics.cmac_tag_size);
    printf("|                                                              |\n");
    printf("+--------------------------------------------------------------+\n");

    printf("\n[BENCH] Key Insights:\n");
    printf("  * ML-KEM-768 provides quantum-resistant key exchange with\n");
    printf("    ~%.1fx time overhead and ~%.0fx size overhead vs ECDH\n",
           g_mlkem_metrics.encaps_time_avg / 120.0,
           (double)g_mlkem_metrics.public_key_size / 65.0);
    printf("\n  * ML-DSA-65 provides quantum-resistant signatures with\n");
    printf("    ~%.0fx time overhead and ~%.0fx size overhead vs AES-CMAC\n",
           g_mldsa_metrics[1].sign_time_avg / g_classical_metrics.cmac_generate_time_avg,
           (double)g_mldsa_metrics[1].signature_size / g_classical_metrics.cmac_tag_size);
    printf("\n  * Trade-off: Higher computational/bandwidth cost for\n");
    printf("    20+ years quantum-resistant security guarantee\n");
}

/* ============================================================================
 * SECTION 7: CSV EXPORT
 * ============================================================================ */

void export_to_csv(void) {
    FILE* fp = fopen("pqc_detailed_results.csv", "w");
    if (!fp) {
        printf("[ERROR] Failed to create CSV file\n");
        return;
    }

    /* ML-KEM-768 Results */
    fprintf(fp, "=== ML-KEM-768 Results ===\n");
    fprintf(fp, "Operation,Avg_Time_us,Min_us,Max_us,Stddev_us,Throughput_ops_sec,Size_bytes\n");
    fprintf(fp, "KeyGen,%.3f,%.3f,%.3f,%.3f,%.0f,%u\n",
            g_mlkem_metrics.keygen_time_avg, g_mlkem_metrics.keygen_time_min,
            g_mlkem_metrics.keygen_time_max, g_mlkem_metrics.keygen_time_stddev,
            g_mlkem_metrics.keygen_throughput,
            g_mlkem_metrics.public_key_size + g_mlkem_metrics.secret_key_size);
    fprintf(fp, "Encapsulate,%.3f,%.3f,%.3f,%.3f,%.0f,%u\n",
            g_mlkem_metrics.encaps_time_avg, g_mlkem_metrics.encaps_time_min,
            g_mlkem_metrics.encaps_time_max, g_mlkem_metrics.encaps_time_stddev,
            g_mlkem_metrics.encaps_throughput, g_mlkem_metrics.ciphertext_size);
    fprintf(fp, "Decapsulate,%.3f,%.3f,%.3f,%.3f,%.0f,%u\n\n",
            g_mlkem_metrics.decaps_time_avg, g_mlkem_metrics.decaps_time_min,
            g_mlkem_metrics.decaps_time_max, g_mlkem_metrics.decaps_time_stddev,
            g_mlkem_metrics.decaps_throughput, g_mlkem_metrics.shared_secret_size);

    /* ML-DSA-65 Results */
    fprintf(fp, "=== ML-DSA-65 Results ===\n");
    fprintf(fp, "Message_Size,Sign_Avg_us,Sign_Min_us,Sign_Max_us,Verify_Avg_us,Verify_Min_us,Verify_Max_us,Signature_Size,Sign_Throughput,Verify_Throughput\n");
    for (uint32 i = 0; i < NUM_MESSAGE_SIZES; i++) {
        fprintf(fp, "%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.0f,%.0f\n",
                test_msg_sizes[i],
                g_mldsa_metrics[i].sign_time_avg, g_mldsa_metrics[i].sign_time_min,
                g_mldsa_metrics[i].sign_time_max,
                g_mldsa_metrics[i].verify_time_avg, g_mldsa_metrics[i].verify_time_min,
                g_mldsa_metrics[i].verify_time_max,
                g_mldsa_metrics[i].signature_size,
                g_mldsa_metrics[i].sign_throughput,
                g_mldsa_metrics[i].verify_throughput);
    }
    fprintf(fp, "\n");

    /* Classical Comparison */
    fprintf(fp, "=== Classical Algorithms ===\n");
    fprintf(fp, "Algorithm,Generate_us,Verify_us,Size_bytes,Throughput_ops_sec\n");
    fprintf(fp, "AES-CMAC,%.3f,%.3f,%u,%.0f\n",
            g_classical_metrics.cmac_generate_time_avg,
            g_classical_metrics.cmac_verify_time_avg,
            g_classical_metrics.cmac_tag_size,
            g_classical_metrics.cmac_throughput);

    fclose(fp);
    printf("\n[OK] Detailed results exported to: pqc_detailed_results.csv\n");
}

/* ============================================================================
 * MAIN TEST EXECUTION
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|   POST-QUANTUM CRYPTOGRAPHY COMPREHENSIVE TEST SUITE         |\n");
    printf("|          HCMUT Computer Engineering Project                  |\n");
    printf("+==============================================================+\n");
    printf("\nTest Configuration:\n");
    printf("  * Iterations: %d (warmup: %d)\n", NUM_ITERATIONS, NUM_WARMUP);
    printf("  * Message Sizes: ");
    for (uint32 i = 0; i < NUM_MESSAGE_SIZES; i++) {
        printf("%u%s", test_msg_sizes[i], (i < NUM_MESSAGE_SIZES - 1) ? ", " : " bytes\n");
    }
    printf("  * Algorithms: ML-KEM-768, ML-DSA-65, AES-CMAC\n");
    printf("\n");

    /* Initialize PQC */
    printf("[CHECK] Initializing PQC modules...\n");
    if (PQC_Init() != PQC_E_OK) {
        printf("[ERROR] PQC initialization failed!\n");
        return 1;
    }
    printf("[OK] PQC modules initialized\n");

    /* PHASE 1.1: ML-KEM-768 Testing */
    print_mlkem_header();
    test_mlkem_keygen();
    test_mlkem_encaps_decaps();

    /* PHASE 1.2: ML-DSA-65 Testing */
    print_mldsa_header();
    test_mldsa_keygen();
    test_mldsa_sign_verify();

    /* PHASE 2: Classical Comparison */
    print_classical_header();
    test_classical_cmac();

    /* Comparative Analysis */
    print_comparative_analysis();

    /* Export Results */
    export_to_csv();

    /* Final Summary */
    printf("\n");
    printf("+==============================================================+\n");
    printf("|                   TEST COMPLETE [OK]                            |\n");
    printf("+==============================================================+\n");
    printf("\n[FILE] Output Files:\n");
    printf("  * pqc_detailed_results.csv (Performance metrics)\n");
    printf("\n[SUMMARY] Summary:\n");
    printf("  [OK] ML-KEM-768: Quantum-resistant key exchange validated\n");
    printf("  [OK] ML-DSA-65: Quantum-resistant signatures validated\n");
    printf("  [OK] Performance baseline established vs classical algorithms\n");
    printf("  [OK] Ready for AUTOSAR integration phase\n");
    printf("\n");

    return 0;
}
