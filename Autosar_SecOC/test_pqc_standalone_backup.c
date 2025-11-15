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

        if (result == PQC_E_OK) {
            g_mlkem_metrics.keygen_success_count++;
        }
    }

    /* Calculate statistics */
    calculate_stats(times, NUM_ITERATIONS,
                   &g_mlkem_metrics.keygen_time_min,
                   &g_mlkem_metrics.keygen_time_max,
                   &g_mlkem_metrics.keygen_time_avg,
                   &g_mlkem_metrics.keygen_time_stddev);

    g_mlkem_metrics.keygen_throughput = 1000000.0 / g_mlkem_metrics.keygen_time_avg;
    g_mlkem_metrics.public_key_size = PQC_MLKEM_PUBLIC_KEY_BYTES;
    g_mlkem_metrics.secret_key_size = PQC_MLKEM_SECRET_KEY_BYTES;

    /* Print results */
    printf("\n  [OK] Key Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mlkem_metrics.keygen_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mlkem_metrics.keygen_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mlkem_metrics.keygen_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_mlkem_metrics.keygen_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_mlkem_metrics.keygen_throughput);
    printf("  | Public Key Size        | %10u B   |\n", g_mlkem_metrics.public_key_size);
    printf("  | Secret Key Size        | %10u B   |\n", g_mlkem_metrics.secret_key_size);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_mlkem_metrics.keygen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");
}

void test_mlkem_encaps_decaps(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing ML-KEM-768 Encapsulation & Decapsulation      |\n");
    printf("+---------------------------------------------------------+\n");

    /* Generate keys first */
    PQC_MLKEM_KeyPairType keypair;
    PQC_MLKEM_KeyGen(&keypair);

    double encaps_times[NUM_ITERATIONS];
    double decaps_times[NUM_ITERATIONS];

    PQC_MLKEM_SharedSecretType shared_secret_alice;
    uint8_t shared_secret_bob[PQC_MLKEM_SHARED_SECRET_BYTES];

    /* Warmup */
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

        if (result == PQC_E_OK) {
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

        if (result == PQC_E_OK) {
            g_mlkem_metrics.decaps_success_count++;

            /* Verify shared secrets match */
            if (memcmp(shared_secret_alice.SharedSecret, shared_secret_bob,
                      PQC_MLKEM_SHARED_SECRET_BYTES) == 0) {
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
    g_mlkem_metrics.ciphertext_size = PQC_MLKEM_CIPHERTEXT_BYTES;
    g_mlkem_metrics.shared_secret_size = PQC_MLKEM_SHARED_SECRET_BYTES;
    g_mlkem_metrics.correctness_rate = (g_mlkem_metrics.shared_secret_match_count * 100.0) / NUM_ITERATIONS;

    /* Print results */
    printf("\n  [OK] Encapsulation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mlkem_metrics.encaps_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mlkem_metrics.encaps_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mlkem_metrics.encaps_time_max);
    printf("  | Throughput             | %10.1f/s  |\n", g_mlkem_metrics.encaps_throughput);
    printf("  | Ciphertext Size        | %10u B   |\n", g_mlkem_metrics.ciphertext_size);
    printf("  +------------------------+----------------+\n");

    printf("\n  [OK] Decapsulation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mlkem_metrics.decaps_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mlkem_metrics.decaps_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mlkem_metrics.decaps_time_max);
    printf("  | Throughput             | %10.1f/s  |\n", g_mlkem_metrics.decaps_throughput);
    printf("  | Shared Secret Size     | %10u B   |\n", g_mlkem_metrics.shared_secret_size);
    printf("  | Correctness Rate       | %9.2f %%  |\n", g_mlkem_metrics.correctness_rate);
    printf("  +------------------------+----------------+\n");
}

/********************************************************************************************************/
/*********************************** ML-DSA-65 TESTS ****************************************************/
/********************************************************************************************************/

void test_mldsa_keygen(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing ML-DSA-65 Key Generation                       |\n");
    printf("+---------------------------------------------------------+\n");

    double times[NUM_ITERATIONS];
    PQC_MLDSA_KeyPairType keypair;

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

        if (result == PQC_E_OK) {
            g_mldsa_metrics[0].keygen_success_count++;
        }
    }

    /* Calculate statistics */
    calculate_stats(times, NUM_ITERATIONS,
                   &g_mldsa_metrics[0].keygen_time_min,
                   &g_mldsa_metrics[0].keygen_time_max,
                   &g_mldsa_metrics[0].keygen_time_avg,
                   &g_mldsa_metrics[0].keygen_time_stddev);

    g_mldsa_metrics[0].keygen_throughput = 1000000.0 / g_mldsa_metrics[0].keygen_time_avg;
    g_mldsa_metrics[0].public_key_size = PQC_MLDSA_PUBLIC_KEY_BYTES;
    g_mldsa_metrics[0].secret_key_size = PQC_MLDSA_SECRET_KEY_BYTES;
    g_mldsa_metrics[0].total_iterations = NUM_ITERATIONS;

    /* Print results */
    printf("\n  [OK] Key Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mldsa_metrics[0].keygen_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mldsa_metrics[0].keygen_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mldsa_metrics[0].keygen_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_mldsa_metrics[0].keygen_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_mldsa_metrics[0].keygen_throughput);
    printf("  | Public Key Size        | %10u B   |\n", g_mldsa_metrics[0].public_key_size);
    printf("  | Secret Key Size        | %10u B   |\n", g_mldsa_metrics[0].secret_key_size);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_mldsa_metrics[0].keygen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");
}

void test_mldsa_sign_verify(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing ML-DSA-65 Sign & Verify (Multiple Sizes)      |\n");
    printf("+---------------------------------------------------------+\n");

    /* Generate keys */
    PQC_MLDSA_KeyPairType keypair;
    PQC_MLDSA_KeyGen(&keypair);

    for (uint32 size_idx = 0; size_idx < NUM_MESSAGE_SIZES; size_idx++) {
        uint32 msg_size = test_msg_sizes[size_idx];
        g_mldsa_metrics[size_idx].message_size = msg_size;
        g_mldsa_metrics[size_idx].total_iterations = NUM_ITERATIONS;

        printf("\n  [TEST] Testing with %u-byte messages...\n", msg_size);

        /* Prepare message */
        uint8_t message[1024];
        for (uint32 i = 0; i < msg_size; i++) {
            message[i] = (uint8_t)(i & 0xFF);
        }

        double sign_times[NUM_ITERATIONS];
        double verify_times[NUM_ITERATIONS];

        uint8_t signature[PQC_MLDSA_SIGNATURE_BYTES];
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

            if (result == PQC_E_OK) {
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

            if (result == PQC_E_OK) {
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

        g_mldsa_metrics[size_idx].sign_throughput = 1000000.0 / g_mldsa_metrics[size_idx].sign_time_avg;
        g_mldsa_metrics[size_idx].verify_throughput = 1000000.0 / g_mldsa_metrics[size_idx].verify_time_avg;
        g_mldsa_metrics[size_idx].signature_size = signature_len;
        g_mldsa_metrics[size_idx].correctness_rate =
            (g_mldsa_metrics[size_idx].valid_signature_count * 100.0) / NUM_ITERATIONS;

        /* Copy key sizes (same for all message sizes) */
        if (size_idx == 0) {
            g_mldsa_metrics[size_idx].public_key_size = PQC_MLDSA_PUBLIC_KEY_BYTES;
            g_mldsa_metrics[size_idx].secret_key_size = PQC_MLDSA_SECRET_KEY_BYTES;
        }

        /* Print results for this size */
        printf("\n    [OK] Sign Results (%u bytes):\n", msg_size);
        printf("    +------------------+----------------+\n");
        printf("    | Average Time     | %10.2f us |\n", g_mldsa_metrics[size_idx].sign_time_avg);
        printf("    | Min Time         | %10.2f us |\n", g_mldsa_metrics[size_idx].sign_time_min);
        printf("    | Max Time         | %10.2f us |\n", g_mldsa_metrics[size_idx].sign_time_max);
        printf("    | Throughput       | %10.1f/s  |\n", g_mldsa_metrics[size_idx].sign_throughput);
        printf("    | Signature Size   | %10u B   |\n", g_mldsa_metrics[size_idx].signature_size);
        printf("    +------------------+----------------+\n");

        printf("\n    [OK] Verify Results (%u bytes):\n", msg_size);
        printf("    +------------------+----------------+\n");
        printf("    | Average Time     | %10.2f us |\n", g_mldsa_metrics[size_idx].verify_time_avg);
        printf("    | Min Time         | %10.2f us |\n", g_mldsa_metrics[size_idx].verify_time_min);
        printf("    | Max Time         | %10.2f us |\n", g_mldsa_metrics[size_idx].verify_time_max);
        printf("    | Throughput       | %10.1f/s  |\n", g_mldsa_metrics[size_idx].verify_throughput);
        printf("    | Correctness      | %9.2f %%  |\n", g_mldsa_metrics[size_idx].correctness_rate);
        printf("    +------------------+----------------+\n");
    }
}

/********************************************************************************************************/
/*********************************** CSV OUTPUT *********************************************************/
/********************************************************************************************************/

void save_results_to_csv(void) {
    FILE* fp = fopen("pqc_standalone_results.csv", "w");
    if (!fp) {
        printf("ERROR: Cannot create CSV file\n");
        return;
    }

    fprintf(fp, "=== ML-KEM-768 Results ===\n");
    fprintf(fp, "Operation,Avg_Time_us,Min_us,Max_us,Stddev_us,Throughput_ops_sec,Size_bytes,Success_Rate\n");
    fprintf(fp, "KeyGen,%.2f,%.2f,%.2f,%.2f,%.1f,%u,%.2f\n",
            g_mlkem_metrics.keygen_time_avg, g_mlkem_metrics.keygen_time_min,
            g_mlkem_metrics.keygen_time_max, g_mlkem_metrics.keygen_time_stddev,
            g_mlkem_metrics.keygen_throughput,
            g_mlkem_metrics.public_key_size + g_mlkem_metrics.secret_key_size,
            (g_mlkem_metrics.keygen_success_count * 100.0) / NUM_ITERATIONS);

    fprintf(fp, "Encapsulate,%.2f,%.2f,%.2f,%.2f,%.1f,%u,%.2f\n",
            g_mlkem_metrics.encaps_time_avg, g_mlkem_metrics.encaps_time_min,
            g_mlkem_metrics.encaps_time_max, g_mlkem_metrics.encaps_time_stddev,
            g_mlkem_metrics.encaps_throughput, g_mlkem_metrics.ciphertext_size,
            (g_mlkem_metrics.encaps_success_count * 100.0) / NUM_ITERATIONS);

    fprintf(fp, "Decapsulate,%.2f,%.2f,%.2f,%.2f,%.1f,%u,%.2f\n",
            g_mlkem_metrics.decaps_time_avg, g_mlkem_metrics.decaps_time_min,
            g_mlkem_metrics.decaps_time_max, g_mlkem_metrics.decaps_time_stddev,
            g_mlkem_metrics.decaps_throughput, g_mlkem_metrics.shared_secret_size,
            g_mlkem_metrics.correctness_rate);

    fprintf(fp, "\n=== ML-DSA-65 Results ===\n");
    fprintf(fp, "Message_Size,Sign_Avg_us,Sign_Min_us,Sign_Max_us,Verify_Avg_us,Verify_Min_us,Verify_Max_us,");
    fprintf(fp, "Signature_Size,Sign_Throughput,Verify_Throughput,Correctness\n");

    for (uint32 i = 0; i < NUM_MESSAGE_SIZES; i++) {
        fprintf(fp, "%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.1f,%.1f,%.2f\n",
                g_mldsa_metrics[i].message_size,
                g_mldsa_metrics[i].sign_time_avg, g_mldsa_metrics[i].sign_time_min,
                g_mldsa_metrics[i].sign_time_max,
                g_mldsa_metrics[i].verify_time_avg, g_mldsa_metrics[i].verify_time_min,
                g_mldsa_metrics[i].verify_time_max,
                g_mldsa_metrics[i].signature_size,
                g_mldsa_metrics[i].sign_throughput,
                g_mldsa_metrics[i].verify_throughput,
                g_mldsa_metrics[i].correctness_rate);
    }

    fclose(fp);
    printf("\n[FILE] Results saved to: pqc_standalone_results.csv\n");
}

/********************************************************************************************************/
/*********************************** MAIN FUNCTION ******************************************************/
/********************************************************************************************************/

int main(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|       PQC STANDALONE TESTING - ML-KEM-768 & ML-DSA-65       |\n");
    printf("+==============================================================+\n");
    printf("\n");

    /* Initialize PQC module */
    printf("Initializing PQC module...\n");
    if (PQC_Init() != PQC_E_OK) {
        printf("[ERROR] PQC initialization failed!\n");
        return 1;
    }
    printf("[OK] PQC module initialized successfully\n\n");

    /* Phase 1: ML-KEM-768 Testing */
    printf("+==============================================================+\n");
    printf("|          PHASE 1: ML-KEM-768 STANDALONE TESTING              |\n");
    printf("+==============================================================+\n");
    printf("\nAlgorithm: ML-KEM-768 (NIST FIPS 203)\n");
    printf("Security Level: Category 3 (AES-192 equivalent)\n\n");

    test_mlkem_keygen();
    test_mlkem_encaps_decaps();

    /* Phase 2: ML-DSA-65 Testing */
    printf("\n\n+==============================================================+\n");
    printf("|          PHASE 2: ML-DSA-65 STANDALONE TESTING               |\n");
    printf("+==============================================================+\n");
    printf("\nAlgorithm: ML-DSA-65 (NIST FIPS 204)\n");
    printf("Security Level: Category 3 (AES-192 equivalent)\n\n");

    test_mldsa_keygen();
    test_mldsa_sign_verify();

    /* Save results */
    save_results_to_csv();

    printf("\n");
    printf("+==============================================================+\n");
    printf("|                  ALL TESTS COMPLETED [OK]                       |\n");
    printf("+==============================================================+\n");
    printf("\n");

    return 0;
}
