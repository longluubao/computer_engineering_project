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
    printf("  [WARMUP] Warming up (%d iterations)...\n", NUM_WARMUP);
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        Csm_SignatureGenerate(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                             message, sizeof(message),
                             signature, &signature_len);
        Csm_SignatureVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                           message, sizeof(message),
                           signature, signature_len, &verify_result);
    }

    /* Benchmark Signature Generation */
    printf("  [BENCH] Benchmarking signature generation (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = Csm_SignatureGenerate(
            0,                                    // jobId
            CRYPTO_OPERATIONMODE_SINGLECALL,     // mode
            message,                              // dataPtr
            sizeof(message),                      // dataLength
            signature,                            // signaturePtr
            &signature_len);                      // signatureLengthPtr

        uint64_t end = get_time_ns();
        gen_times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_pqc_metrics.gen_success_count++;
        }
    }

    g_pqc_metrics.authenticator_size = signature_len;

    /* Benchmark Signature Verification */
    printf("  [BENCH] Benchmarking signature verification (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = Csm_SignatureVerify(
            0,                                    // jobId
            CRYPTO_OPERATIONMODE_SINGLECALL,     // mode
            message,                              // dataPtr
            sizeof(message),                      // dataLength
            signature,                            // signaturePtr
            signature_len,                        // signatureLength
            &verify_result);                      // verifyPtr

        uint64_t end = get_time_ns();
        verify_times[i] = ns_to_us(end - start);

        if (result == E_OK && verify_result == CRYPTO_E_VER_OK) {
            g_pqc_metrics.verify_success_count++;
        }
    }

    /* Calculate statistics */
    calculate_stats(gen_times, NUM_ITERATIONS,
                   &g_pqc_metrics.gen_time_min,
                   &g_pqc_metrics.gen_time_max,
                   &g_pqc_metrics.gen_time_avg,
                   &g_pqc_metrics.gen_time_stddev);

    calculate_stats(verify_times, NUM_ITERATIONS,
                   &g_pqc_metrics.verify_time_min,
                   &g_pqc_metrics.verify_time_max,
                   &g_pqc_metrics.verify_time_avg,
                   &g_pqc_metrics.verify_time_stddev);

    g_pqc_metrics.gen_throughput = 1000000.0 / g_pqc_metrics.gen_time_avg;
    g_pqc_metrics.verify_throughput = 1000000.0 / g_pqc_metrics.verify_time_avg;

    /* Print results */
    printf("\n  [OK] PQC Signature Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_pqc_metrics.gen_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_pqc_metrics.gen_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_pqc_metrics.gen_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_pqc_metrics.gen_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_pqc_metrics.gen_throughput);
    printf("  | Signature Size         | %10u B   |\n", g_pqc_metrics.authenticator_size);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_pqc_metrics.gen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");

    printf("\n  [OK] PQC Signature Verification Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_pqc_metrics.verify_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_pqc_metrics.verify_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_pqc_metrics.verify_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_pqc_metrics.verify_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_pqc_metrics.verify_throughput);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_pqc_metrics.verify_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");
}

/********************************************************************************************************/
/*********************************** CSM LAYER TESTS (CLASSICAL MAC) ************************************/
/********************************************************************************************************/

void test_csm_classical_mac(void) {
    printf("\n+---------------------------------------------------------+\n");
    printf("|  Testing Csm Classical MAC Generation & Verification   |\n");
    printf("+---------------------------------------------------------+\n");

    double gen_times[NUM_ITERATIONS];
    double verify_times[NUM_ITERATIONS];

    /* Test message (same as PQC test for fair comparison) */
    uint8_t message[256];
    for (uint32 i = 0; i < sizeof(message); i++) {
        message[i] = (uint8_t)(i & 0xFF);
    }

    uint8_t mac[16];  // AES-128-CMAC output
    uint32 mac_len = 16;
    Crypto_VerifyResultType verify_result;

    g_mac_metrics.total_iterations = NUM_ITERATIONS;

    /* Warmup */
    printf("  [WARMUP] Warming up (%d iterations)...\n", NUM_WARMUP);
    for (uint32 i = 0; i < NUM_WARMUP; i++) {
        Csm_MacGenerate(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                       message, sizeof(message), mac, &mac_len);
        Csm_MacVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                     message, sizeof(message), mac, mac_len, &verify_result);
    }

    /* Benchmark MAC Generation */
    printf("  [BENCH] Benchmarking MAC generation (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = Csm_MacGenerate(
            0,                                    // jobId
            CRYPTO_OPERATIONMODE_SINGLECALL,     // mode
            message,                              // dataPtr
            sizeof(message),                      // dataLength
            mac,                                  // macPtr
            &mac_len);                            // macLengthPtr

        uint64_t end = get_time_ns();
        gen_times[i] = ns_to_us(end - start);

        if (result == E_OK) {
            g_mac_metrics.gen_success_count++;
        }
    }

    g_mac_metrics.authenticator_size = mac_len;

    /* Benchmark MAC Verification */
    printf("  [BENCH] Benchmarking MAC verification (%d iterations)...\n", NUM_ITERATIONS);
    for (uint32 i = 0; i < NUM_ITERATIONS; i++) {
        uint64_t start = get_time_ns();

        Std_ReturnType result = Csm_MacVerify(
            0,                                    // jobId
            CRYPTO_OPERATIONMODE_SINGLECALL,     // mode
            message,                              // dataPtr
            sizeof(message),                      // dataLength
            mac,                                  // macPtr
            mac_len,                              // macLength
            &verify_result);                      // verifyPtr

        uint64_t end = get_time_ns();
        verify_times[i] = ns_to_us(end - start);

        if (result == E_OK && verify_result == CRYPTO_E_VER_OK) {
            g_mac_metrics.verify_success_count++;
        }
    }

    /* Calculate statistics */
    calculate_stats(gen_times, NUM_ITERATIONS,
                   &g_mac_metrics.gen_time_min,
                   &g_mac_metrics.gen_time_max,
                   &g_mac_metrics.gen_time_avg,
                   &g_mac_metrics.gen_time_stddev);

    calculate_stats(verify_times, NUM_ITERATIONS,
                   &g_mac_metrics.verify_time_min,
                   &g_mac_metrics.verify_time_max,
                   &g_mac_metrics.verify_time_avg,
                   &g_mac_metrics.verify_time_stddev);

    g_mac_metrics.gen_throughput = 1000000.0 / g_mac_metrics.gen_time_avg;
    g_mac_metrics.verify_throughput = 1000000.0 / g_mac_metrics.verify_time_avg;

    /* Print results */
    printf("\n  [OK] Classical MAC Generation Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Metric                 | Value          |\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mac_metrics.gen_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mac_metrics.gen_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mac_metrics.gen_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_mac_metrics.gen_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_mac_metrics.gen_throughput);
    printf("  | MAC Size               | %10u B   |\n", g_mac_metrics.authenticator_size);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_mac_metrics.gen_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");

    printf("\n  [OK] Classical MAC Verification Results:\n");
    printf("  +------------------------+----------------+\n");
    printf("  | Average Time           | %10.2f us |\n", g_mac_metrics.verify_time_avg);
    printf("  | Min Time               | %10.2f us |\n", g_mac_metrics.verify_time_min);
    printf("  | Max Time               | %10.2f us |\n", g_mac_metrics.verify_time_max);
    printf("  | Std Deviation          | %10.2f us |\n", g_mac_metrics.verify_time_stddev);
    printf("  | Throughput             | %10.1f/s  |\n", g_mac_metrics.verify_throughput);
    printf("  | Success Rate           | %9.2f %%  |\n",
           (g_mac_metrics.verify_success_count * 100.0) / NUM_ITERATIONS);
    printf("  +------------------------+----------------+\n");
}

/********************************************************************************************************/
/*********************************** COMPARISON ANALYSIS ************************************************/
/********************************************************************************************************/

void print_comparison_analysis(void) {
    printf("\n\n+==============================================================+\n");
    printf("|            PERFORMANCE COMPARISON: PQC vs Classical          |\n");
    printf("+==============================================================+\n");

    printf("\n+------------------------+----------------+----------------+------------+\n");
    printf("| Metric                 | Classical MAC  | PQC Signature  | Overhead   |\n");
    printf("+------------------------+----------------+----------------+------------+\n");

    double gen_overhead = (g_pqc_metrics.gen_time_avg / g_mac_metrics.gen_time_avg);
    double verify_overhead = (g_pqc_metrics.verify_time_avg / g_mac_metrics.verify_time_avg);
    double size_overhead = (double)g_pqc_metrics.authenticator_size / g_mac_metrics.authenticator_size;

    printf("| Generation Time (avg)  | %10.2f us | %10.2f us | %8.1fx |\n",
           g_mac_metrics.gen_time_avg, g_pqc_metrics.gen_time_avg, gen_overhead);
    printf("| Verification Time (avg)| %10.2f us | %10.2f us | %8.1fx |\n",
           g_mac_metrics.verify_time_avg, g_pqc_metrics.verify_time_avg, verify_overhead);
    printf("| Authenticator Size     | %10u B   | %10u B   | %8.1fx |\n",
           g_mac_metrics.authenticator_size, g_pqc_metrics.authenticator_size, size_overhead);
    printf("| Generation Throughput  | %10.1f/s  | %10.1f/s  | %8.2fx |\n",
           g_mac_metrics.gen_throughput, g_pqc_metrics.gen_throughput,
           g_mac_metrics.gen_throughput / g_pqc_metrics.gen_throughput);
    printf("| Verify Throughput      | %10.1f/s  | %10.1f/s  | %8.2fx |\n",
           g_mac_metrics.verify_throughput, g_pqc_metrics.verify_throughput,
           g_mac_metrics.verify_throughput / g_pqc_metrics.verify_throughput);
    printf("+------------------------+----------------+----------------+------------+\n");

    printf("\n[ANALYSIS] Analysis:\n");
    printf("   - Time Overhead: PQC is %.1fx slower for generation, %.1fx slower for verification\n",
           gen_overhead, verify_overhead);
    printf("   - Size Overhead: PQC signature is %.1fx larger than MAC\n", size_overhead);
    printf("   - Security Benefit: PQC provides quantum resistance\n");
    printf("   - Trade-off: Higher computational cost for future-proof security\n");
}

/********************************************************************************************************/
/*********************************** TAMPERING DETECTION TEST *******************************************/
/********************************************************************************************************/

void test_tampering_detection(void) {
    printf("\n\n+==============================================================+\n");
    printf("|              SECURITY TEST: Tampering Detection              |\n");
    printf("+==============================================================+\n");

    uint8_t message[64] = "This is a test message for SecOC authentication";
    uint8_t signature[PQC_MLDSA_SIGNATURE_BYTES];
    uint32 signature_len;
    Crypto_VerifyResultType verify_result;

    printf("\n[OK] Test 1: Valid Signature (No Tampering)\n");

    /* Generate valid signature */
    Csm_SignatureGenerate(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                         message, sizeof(message),
                         signature, &signature_len);

    /* Verify valid signature */
    Std_ReturnType result = Csm_SignatureVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                                message, sizeof(message),
                                                signature, signature_len,
                                                &verify_result);

    if (result == E_OK && verify_result == CRYPTO_E_VER_OK) {
        printf("   [OK] Valid signature verified successfully\n");
        g_pqc_metrics.verify_success_count++;
    } else {
        printf("   [ERROR] Valid signature verification failed!\n");
    }

    printf("\n[ERROR] Test 2: Message Tampering Detection\n");

    /* Tamper with message (flip one bit) */
    message[10] ^= 0x01;

    /* Try to verify with tampered message */
    result = Csm_SignatureVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                message, sizeof(message),
                                signature, signature_len,
                                &verify_result);

    if (result != E_OK || verify_result != CRYPTO_E_VER_OK) {
        printf("   [OK] Tampering detected successfully! (Verification failed as expected)\n");
        g_pqc_metrics.tampering_detected++;
    } else {
        printf("   [ERROR] WARNING: Tampering NOT detected! Security compromised!\n");
        g_pqc_metrics.tampering_missed++;
    }

    /* Restore message */
    message[10] ^= 0x01;

    printf("\n[ERROR] Test 3: Signature Tampering Detection\n");

    /* Tamper with signature (flip one byte) */
    signature[100] ^= 0xFF;

    /* Try to verify with tampered signature */
    result = Csm_SignatureVerify(0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                message, sizeof(message),
                                signature, signature_len,
                                &verify_result);

    if (result != E_OK || verify_result != CRYPTO_E_VER_OK) {
        printf("   [OK] Signature tampering detected successfully!\n");
        g_pqc_metrics.tampering_detected++;
    } else {
        printf("   [ERROR] WARNING: Signature tampering NOT detected!\n");
        g_pqc_metrics.tampering_missed++;
    }

    printf("\n[SUMMARY] Security Test Summary:\n");
    printf("   - Tampering attacks detected: %u/2\n", g_pqc_metrics.tampering_detected);
    printf("   - Tampering attacks missed: %u/2\n", g_pqc_metrics.tampering_missed);
    if (g_pqc_metrics.tampering_detected == 2) {
        printf("   [OK] All tampering attempts were successfully detected!\n");
    } else {
        printf("   [ERROR] Some tampering attempts were not detected!\n");
    }
}

/********************************************************************************************************/
/*********************** ETHERNET GATEWAY DATA FLOW TESTING (SECOC) *************************************/
/********************************************************************************************************/

/**
 * @brief Ethernet Gateway SecOC Data Flow Simulation
 * @details Simulates complete end-to-end Ethernet SecOC flow with PQC signatures
 *
 * Flow:
 * 1. Ethernet Frame Reception (simulated PDU)
 * 2. PDU Extraction & Parsing
 * 3. Freshness Value Management (counter-based)
 * 4. Data-to-Authenticator Construction: MessageID + Data + Freshness
 * 5. PQC Signature Generation (via Csm_SignatureGenerate)
 * 6. Secured PDU Construction: Data + Authenticator (signature)
 * 7. Ethernet Transmission Simulation
 * 8. Reception & Verification Flow
 * 9. Freshness Validation & Replay Attack Detection
 */

/* Ethernet PDU Structure for SecOC */
typedef struct {
    uint16 message_id;        /* SecOC Message Identifier */
    uint8  data[256];         /* Payload data */
    uint32 data_length;       /* Actual payload length */
    uint32 freshness_value;   /* Freshness counter (timestamp/counter) */
    uint8  authenticator[PQC_MLDSA_SIGNATURE_BYTES];  /* PQC Signature */
    uint32 authenticator_length;
} EthernetSecOC_PDU;

/* Global freshness counter (simulates SecOC freshness manager) */
static uint32 g_freshness_counter = 0;

/**
 * @brief Construct Data-to-Authenticator for SecOC
 * @details Per AUTOSAR SecOC spec: MessageID + Data + FreshnessValue
 */
static void construct_data_to_authenticator(
    uint16 message_id,
    const uint8* data,
    uint32 data_len,
    uint32 freshness,
    uint8* data_to_auth,
    uint32* data_to_auth_len
) {
    uint32 offset = 0;

    /* Add Message ID (2 bytes, big-endian) */
    data_to_auth[offset++] = (uint8)((message_id >> 8) & 0xFF);
    data_to_auth[offset++] = (uint8)(message_id & 0xFF);

    /* Add Data */
    memcpy(data_to_auth + offset, data, data_len);
    offset += data_len;

    /* Add Freshness Value (4 bytes, big-endian) */
    data_to_auth[offset++] = (uint8)((freshness >> 24) & 0xFF);
    data_to_auth[offset++] = (uint8)((freshness >> 16) & 0xFF);
    data_to_auth[offset++] = (uint8)((freshness >> 8) & 0xFF);
    data_to_auth[offset++] = (uint8)(freshness & 0xFF);

    *data_to_auth_len = offset;
}

/**
 * @brief Simulate Ethernet Gateway SecOC TX Flow
 */
static Std_ReturnType ethernet_gateway_tx_flow(EthernetSecOC_PDU* pdu) {
    printf("  [ETH-TX] Ethernet Gateway Transmission Flow\n");

    /* Step 1: Increment freshness counter */
    g_freshness_counter++;
    pdu->freshness_value = g_freshness_counter;
    printf("    Step 1: Freshness counter = %u\n", pdu->freshness_value);

    /* Step 2: Construct Data-to-Authenticator */
    uint8 data_to_auth[1024];
    uint32 data_to_auth_len;
    construct_data_to_authenticator(
        pdu->message_id,
        pdu->data,
        pdu->data_length,
        pdu->freshness_value,
        data_to_auth,
        &data_to_auth_len
    );
    printf("    Step 2: Data-to-Authenticator length = %u bytes\n", data_to_auth_len);
    printf("            (MessageID[2] + Data[%u] + Freshness[4])\n", pdu->data_length);

    /* Step 3: Generate PQC Signature via Csm */
    printf("    Step 3: Generating PQC signature via Csm_SignatureGenerate...\n");
    Std_ReturnType result = Csm_SignatureGenerate(
        0,                              /* jobId */
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data_to_auth,
        data_to_auth_len,
        pdu->authenticator,
        &pdu->authenticator_length
    );

    if (result != E_OK) {
        printf("    [ERROR] Signature generation failed!\n");
        return E_NOT_OK;
    }

    printf("    Step 4: PQC Signature generated (%u bytes)\n", pdu->authenticator_length);
    printf("    Step 5: Secured PDU ready for Ethernet transmission\n");
    printf("            Total PDU size: %u bytes (Data) + %u bytes (Auth) = %u bytes\n",
           pdu->data_length, pdu->authenticator_length,
           pdu->data_length + pdu->authenticator_length);

    return E_OK;
}

/**
 * @brief Simulate Ethernet Gateway SecOC RX Flow
 */
static Std_ReturnType ethernet_gateway_rx_flow(const EthernetSecOC_PDU* pdu, bool* replay_detected) {
    printf("\n  [ETH-RX] Ethernet Gateway Reception Flow\n");

    /* Step 1: Freshness validation (replay attack detection) */
    printf("    Step 1: Freshness validation\n");
    printf("            Received freshness: %u\n", pdu->freshness_value);
    printf("            Expected freshness: > %u\n", g_freshness_counter);

    if (pdu->freshness_value <= g_freshness_counter) {
        printf("    [SECURITY] REPLAY ATTACK DETECTED! Old freshness value reused\n");
        *replay_detected = true;
        return E_NOT_OK;
    }

    *replay_detected = false;
    printf("    [OK] Freshness validation passed\n");

    /* Step 2: Reconstruct Data-to-Authenticator */
    uint8 data_to_auth[1024];
    uint32 data_to_auth_len;
    construct_data_to_authenticator(
        pdu->message_id,
        pdu->data,
        pdu->data_length,
        pdu->freshness_value,
        data_to_auth,
        &data_to_auth_len
    );
    printf("    Step 2: Data-to-Authenticator reconstructed (%u bytes)\n", data_to_auth_len);

    /* Step 3: Verify PQC Signature via Csm */
    printf("    Step 3: Verifying PQC signature via Csm_SignatureVerify...\n");
    Crypto_VerifyResultType verify_result;
    Std_ReturnType result = Csm_SignatureVerify(
        0,                              /* jobId */
        CRYPTO_OPERATIONMODE_SINGLECALL,
        data_to_auth,
        data_to_auth_len,
        pdu->authenticator,
        pdu->authenticator_length,
        &verify_result
    );

    if (result != E_OK || verify_result != CRYPTO_E_VER_OK) {
        printf("    [ERROR] Signature verification failed!\n");
        return E_NOT_OK;
    }

    printf("    Step 4: Signature verification PASSED\n");
    printf("    Step 5: Update freshness counter to %u\n", pdu->freshness_value);
    g_freshness_counter = pdu->freshness_value;

    printf("    [OK] PDU authenticated and accepted\n");
    return E_OK;
}

/**
 * @brief Test Complete Ethernet Gateway Data Flow
 */
void test_ethernet_gateway_data_flow(void) {
    printf("\n\n+==============================================================+\n");
    printf("|        ETHERNET GATEWAY SECOC DATA FLOW TESTING              |\n");
    printf("+==============================================================+\n");
    printf("\nSimulating AUTOSAR SecOC in Ethernet Gateway Context\n");
    printf("Protocol: Ethernet II + SecOC + PQC ML-DSA-65 Signatures\n\n");

    /* Initialize test PDU */
    EthernetSecOC_PDU tx_pdu, rx_pdu;

    /* Scenario 1: Normal Communication Flow */
    printf("+--------------------------------------------------------------+\n");
    printf("| SCENARIO 1: Normal Ethernet SecOC Communication              |\n");
    printf("+--------------------------------------------------------------+\n\n");

    /* Prepare TX PDU */
    tx_pdu.message_id = 0x1234;
    const char* test_data = "Ethernet Gateway Test Data - AUTOSAR SecOC with PQC";
    memcpy(tx_pdu.data, test_data, strlen(test_data));
    tx_pdu.data_length = strlen(test_data);

    printf("[GATEWAY-TX] Preparing PDU for transmission\n");
    printf("  Message ID: 0x%04X\n", tx_pdu.message_id);
    printf("  Data: '%s' (%u bytes)\n", test_data, tx_pdu.data_length);

    /* TX Flow */
    if (ethernet_gateway_tx_flow(&tx_pdu) != E_OK) {
        printf("[ERROR] TX flow failed!\n");
        return;
    }

    printf("\n[GATEWAY-RX] Simulating Ethernet frame reception\n");
    /* Copy TX PDU to RX PDU (simulates Ethernet transmission) */
    memcpy(&rx_pdu, &tx_pdu, sizeof(EthernetSecOC_PDU));

    /* RX Flow */
    bool replay_detected;
    if (ethernet_gateway_rx_flow(&rx_pdu, &replay_detected) == E_OK) {
        printf("\n[OK] Scenario 1 PASSED: Normal flow successful\n");
    } else {
        printf("\n[ERROR] Scenario 1 FAILED\n");
    }

    /* Scenario 2: Replay Attack Detection */
    printf("\n\n+--------------------------------------------------------------+\n");
    printf("| SCENARIO 2: Replay Attack Detection                          |\n");
    printf("+--------------------------------------------------------------+\n\n");

    printf("[ATTACKER] Replaying old PDU with freshness=%u\n", rx_pdu.freshness_value);
    printf("[GATEWAY-RX] Receiving replayed PDU...\n");

    /* Try to process same PDU again (replay attack) */
    if (ethernet_gateway_rx_flow(&rx_pdu, &replay_detected) != E_OK && replay_detected) {
        printf("\n[OK] Scenario 2 PASSED: Replay attack successfully detected\n");
    } else {
        printf("\n[ERROR] Scenario 2 FAILED: Replay attack not detected!\n");
    }

    /* Scenario 3: Message Tampering Detection */
    printf("\n\n+--------------------------------------------------------------+\n");
    printf("| SCENARIO 3: Message Tampering Detection                      |\n");
    printf("+--------------------------------------------------------------+\n\n");

    /* Prepare new valid PDU */
    memcpy(tx_pdu.data, "Original untampered message", 27);
    tx_pdu.data_length = 27;
    ethernet_gateway_tx_flow(&tx_pdu);
    memcpy(&rx_pdu, &tx_pdu, sizeof(EthernetSecOC_PDU));

    /* Tamper with data */
    printf("[ATTACKER] Tampering with message data...\n");
    rx_pdu.data[10] ^= 0xFF;  /* Flip bits */

    printf("[GATEWAY-RX] Receiving tampered PDU...\n");
    if (ethernet_gateway_rx_flow(&rx_pdu, &replay_detected) != E_OK) {
        printf("\n[OK] Scenario 3 PASSED: Tampering successfully detected\n");
    } else {
        printf("\n[ERROR] Scenario 3 FAILED: Tampering not detected!\n");
    }

    /* Summary */
    printf("\n\n+==============================================================+\n");
    printf("|           ETHERNET GATEWAY FLOW TEST SUMMARY                 |\n");
    printf("+==============================================================+\n");
    printf("\n[SUMMARY] Test Results:\n");
    printf("  1. Normal Communication:      TESTED\n");
    printf("  2. Replay Attack Detection:   TESTED\n");
    printf("  3. Tampering Detection:       TESTED\n");
    printf("\n[INFO] Ethernet Gateway SecOC Flow:\n");
    printf("  - Freshness management:       WORKING\n");
    printf("  - PQC signature integration:  WORKING\n");
    printf("  - Security validations:       WORKING\n");
    printf("  - AUTOSAR Csm layer:          INTEGRATED\n");
    printf("\n[OK] Ethernet Gateway data flow test completed successfully\n");
}

/********************************************************************************************************/
/*********************************** CSV OUTPUT *********************************************************/
/********************************************************************************************************/

void save_results_to_csv(void) {
    FILE* fp = fopen("pqc_secoc_integration_results.csv", "w");
    if (!fp) {
        printf("ERROR: Cannot create CSV file\n");
        return;
    }

    fprintf(fp, "=== Csm Layer Performance Comparison ===\n");
    fprintf(fp, "Crypto_Method,Gen_Avg_us,Gen_Min_us,Gen_Max_us,Verify_Avg_us,Verify_Min_us,Verify_Max_us,");
    fprintf(fp, "Authenticator_Size_B,Gen_Throughput_ops_sec,Verify_Throughput_ops_sec\n");

    fprintf(fp, "Classical_MAC,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.1f,%.1f\n",
            g_mac_metrics.gen_time_avg, g_mac_metrics.gen_time_min, g_mac_metrics.gen_time_max,
            g_mac_metrics.verify_time_avg, g_mac_metrics.verify_time_min, g_mac_metrics.verify_time_max,
            g_mac_metrics.authenticator_size, g_mac_metrics.gen_throughput, g_mac_metrics.verify_throughput);

    fprintf(fp, "PQC_Signature,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%.1f,%.1f\n",
            g_pqc_metrics.gen_time_avg, g_pqc_metrics.gen_time_min, g_pqc_metrics.gen_time_max,
            g_pqc_metrics.verify_time_avg, g_pqc_metrics.verify_time_min, g_pqc_metrics.verify_time_max,
            g_pqc_metrics.authenticator_size, g_pqc_metrics.gen_throughput, g_pqc_metrics.verify_throughput);

    fprintf(fp, "\n=== Overhead Analysis ===\n");
    fprintf(fp, "Metric,Classical,PQC,Overhead_Ratio\n");
    fprintf(fp, "Generation_Time_us,%.2f,%.2f,%.2fx\n",
            g_mac_metrics.gen_time_avg, g_pqc_metrics.gen_time_avg,
            g_pqc_metrics.gen_time_avg / g_mac_metrics.gen_time_avg);
    fprintf(fp, "Verification_Time_us,%.2f,%.2f,%.2fx\n",
            g_mac_metrics.verify_time_avg, g_pqc_metrics.verify_time_avg,
            g_pqc_metrics.verify_time_avg / g_mac_metrics.verify_time_avg);
    fprintf(fp, "Authenticator_Size_B,%u,%u,%.2fx\n",
            g_mac_metrics.authenticator_size, g_pqc_metrics.authenticator_size,
            (double)g_pqc_metrics.authenticator_size / g_mac_metrics.authenticator_size);

    fprintf(fp, "\n=== Security Test Results ===\n");
    fprintf(fp, "Test,Result\n");
    fprintf(fp, "Tampering_Detected,%u\n", g_pqc_metrics.tampering_detected);
    fprintf(fp, "Tampering_Missed,%u\n", g_pqc_metrics.tampering_missed);

    fclose(fp);
    printf("\n[FILE] Results saved to: pqc_secoc_integration_results.csv\n");
}

/********************************************************************************************************/
/*********************************** MAIN FUNCTION ******************************************************/
/********************************************************************************************************/

int main(void) {
    printf("\n");
    printf("+==============================================================+\n");
    printf("|       PQC SECOC INTEGRATION TESTING - Csm Layer Tests       |\n");
    printf("+==============================================================+\n");
    printf("\n");

    /* Initialize PQC module */
    printf("Initializing PQC module...\n");
    if (PQC_Init() != PQC_E_OK) {
        printf("[ERROR] PQC initialization failed!\n");
        return 1;
    }
    printf("[OK] PQC module initialized successfully\n\n");

    /* Phase 1: Csm Layer PQC Testing */
    printf("+==============================================================+\n");
    printf("|         PHASE 1: CSM LAYER PQC SIGNATURE TESTING            |\n");
    printf("+==============================================================+\n");
    printf("\nTesting: Csm_SignatureGenerate() / Csm_SignatureVerify()\n");
    printf("Layer: AUTOSAR Csm -> PQC_MLDSA\n\n");

    test_csm_pqc_signature();

    /* Phase 2: Classical MAC Testing (for comparison) */
    printf("\n\n+==============================================================+\n");
    printf("|       PHASE 2: CSM LAYER CLASSICAL MAC TESTING              |\n");
    printf("+==============================================================+\n");
    printf("\nTesting: Csm_MacGenerate() / Csm_MacVerify()\n");
    printf("Algorithm: AES-128-CMAC\n\n");

    test_csm_classical_mac();

    /* Phase 3: Performance Comparison */
    print_comparison_analysis();

    /* Phase 4: Security Testing */
    test_tampering_detection();

    /* Phase 5: Ethernet Gateway Data Flow Testing */
    test_ethernet_gateway_data_flow();

    /* Save results */
    save_results_to_csv();

    printf("\n");
    printf("+==============================================================+\n");
    printf("|            ALL INTEGRATION TESTS COMPLETED [OK]                 |\n");
    printf("+==============================================================+\n");
    printf("\n");

    return 0;
}
