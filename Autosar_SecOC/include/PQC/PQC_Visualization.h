/**
 * @file PQC_Visualization.h
 * @brief Visualization and detailed logging for Post-Quantum Cryptography operations
 * @details Provides comprehensive logging for thesis demonstration of ML-KEM and ML-DSA
 */

#ifndef PQC_VISUALIZATION_H
#define PQC_VISUALIZATION_H

#include "Std_Types.h"
#include <stdio.h>
#include <string.h>

/********************************************************************************************************/
/********************************************Visualization Macros****************************************/
/********************************************************************************************************/

// Enable detailed PQC visualization for thesis
#define PQC_VISUALIZATION_ENABLED    TRUE

// Color codes for terminal output
#define PQC_COLOR_RESET   "\033[0m"
#define PQC_COLOR_RED     "\033[31m"
#define PQC_COLOR_GREEN   "\033[32m"
#define PQC_COLOR_YELLOW  "\033[33m"
#define PQC_COLOR_BLUE    "\033[34m"
#define PQC_COLOR_MAGENTA "\033[35m"
#define PQC_COLOR_CYAN    "\033[36m"
#define PQC_COLOR_BOLD    "\033[1m"

/**
 * @brief Print hex dump of data for visualization
 */
static inline void PQC_PrintHexDump(const char* label, const uint8* data, uint32 length, uint32 max_display)
{
    if(length == 0 || data == NULL) return;

    uint32 display_len = (length > max_display) ? max_display : length;

    printf("%s", PQC_COLOR_CYAN);
    printf("  %s (%u bytes", label, length);
    if(length > max_display) printf(", showing first %u", max_display);
    printf("): ", PQC_COLOR_RESET);

    for(uint32 i = 0; i < display_len; i++)
    {
        printf("%02X ", data[i]);
        if((i + 1) % 16 == 0 && i < display_len - 1)
            printf("\n    ");
    }
    if(length > max_display) printf("...");
    printf("\n");
}

/**
 * @brief Print section header for visualization
 */
static inline void PQC_PrintSectionHeader(const char* title)
{
    printf("\n");
    printf("%s%s", PQC_COLOR_BOLD, PQC_COLOR_BLUE);
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  %-59s  ║\n", title);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("%s", PQC_COLOR_RESET);
}

/**
 * @brief Print sub-step for visualization
 */
static inline void PQC_PrintStep(const char* step_num, const char* description)
{
    printf("%s%s[%s]%s %s\n",
           PQC_COLOR_YELLOW, PQC_COLOR_BOLD, step_num, PQC_COLOR_RESET, description);
}

/**
 * @brief Print success message
 */
static inline void PQC_PrintSuccess(const char* message)
{
    printf("%s✓ SUCCESS:%s %s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET, message);
}

/**
 * @brief Print error message
 */
static inline void PQC_PrintError(const char* message)
{
    printf("%s✗ ERROR:%s %s\n", PQC_COLOR_RED, PQC_COLOR_RESET, message);
}

/**
 * @brief Print warning message
 */
static inline void PQC_PrintWarning(const char* message)
{
    printf("%s⚠ WARNING:%s %s\n", PQC_COLOR_YELLOW, PQC_COLOR_RESET, message);
}

/**
 * @brief Print key information
 */
static inline void PQC_PrintInfo(const char* label, const char* value)
{
    printf("%s  • %s:%s %s\n", PQC_COLOR_CYAN, label, PQC_COLOR_RESET, value);
}

/**
 * @brief Print algorithm information
 */
static inline void PQC_PrintAlgorithmInfo(const char* algorithm, const char* standard, const char* security_level)
{
    printf("\n%s📊 Algorithm Details:%s\n", PQC_COLOR_MAGENTA, PQC_COLOR_RESET);
    PQC_PrintInfo("Algorithm", algorithm);
    PQC_PrintInfo("Standard", standard);
    PQC_PrintInfo("Security Level", security_level);
}

/**
 * @brief Print size comparison
 */
static inline void PQC_PrintSizeComparison(const char* item, uint32 pqc_size, uint32 classical_size)
{
    printf("%s  📏 %s:%s\n", PQC_COLOR_YELLOW, item, PQC_COLOR_RESET);
    printf("     PQC: %u bytes | Classical: %u bytes | Ratio: %.1fx\n",
           pqc_size, classical_size, (float)pqc_size / classical_size);
}

/**
 * @brief Print performance timing
 */
static inline void PQC_PrintTiming(const char* operation, double time_ms)
{
    printf("%s  ⏱ %s:%s %.3f ms",
           PQC_COLOR_CYAN, operation, PQC_COLOR_RESET, time_ms);

    if(time_ms < 1.0)
        printf(" %s(VERY FAST)%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);
    else if(time_ms < 10.0)
        printf(" %s(FAST)%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);
    else if(time_ms < 100.0)
        printf(" %s(ACCEPTABLE)%s\n", PQC_COLOR_YELLOW, PQC_COLOR_RESET);
    else
        printf(" %s(SLOW)%s\n", PQC_COLOR_RED, PQC_COLOR_RESET);
}

/********************************************************************************************************/
/********************************************ML-DSA Visualization****************************************/
/********************************************************************************************************/

/**
 * @brief Visualize ML-DSA signature generation process
 */
static inline void PQC_VisualizeMLDSA_SignStart(const uint8* message, uint32 msg_len)
{
    PQC_PrintSectionHeader("ML-DSA-65 DIGITAL SIGNATURE GENERATION");

    PQC_PrintAlgorithmInfo(
        "ML-DSA-65 (Module-Lattice Digital Signature Algorithm)",
        "NIST FIPS 204 (August 2024)",
        "NIST Level 3 (AES-192 equivalent)"
    );

    printf("\n");
    PQC_PrintStep("1", "Input Message Preparation");
    PQC_PrintHexDump("Message Data", message, msg_len, 64);
    printf("  Message Length: %u bytes\n", msg_len);
}

/**
 * @brief Visualize ML-DSA signature generation completion
 */
static inline void PQC_VisualizeMLDSA_SignComplete(const uint8* signature, uint32 sig_len)
{
    printf("\n");
    PQC_PrintStep("2", "Lattice-Based Signature Computation");
    printf("  Using Module-LWE problem (quantum-resistant)\n");
    printf("  Private key applies lattice transformations\n");

    printf("\n");
    PQC_PrintStep("3", "Signature Output");
    PQC_PrintHexDump("Generated Signature", signature, sig_len, 64);

    printf("\n");
    PQC_PrintSizeComparison("Signature Size", sig_len, 4);

    printf("\n");
    PQC_PrintSuccess("ML-DSA-65 signature generated successfully!");
    printf("%s  🔐 Quantum-Resistant: Protected against Shor's algorithm%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
    printf("%s  🛡️  Security: NIST Level 3 (192-bit classical equivalent)%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
}

/**
 * @brief Visualize ML-DSA signature verification process
 */
static inline void PQC_VisualizeMLDSA_VerifyStart(const uint8* message, uint32 msg_len,
                                                   const uint8* signature, uint32 sig_len)
{
    PQC_PrintSectionHeader("ML-DSA-65 SIGNATURE VERIFICATION");

    printf("\n");
    PQC_PrintStep("1", "Input Preparation");
    PQC_PrintHexDump("Message", message, msg_len, 64);
    PQC_PrintHexDump("Signature to Verify", signature, sig_len, 64);
}

/**
 * @brief Visualize ML-DSA verification completion
 */
static inline void PQC_VisualizeMLDSA_VerifyComplete(boolean is_valid)
{
    printf("\n");
    PQC_PrintStep("2", "Lattice Verification");
    printf("  Checking signature against public key\n");
    printf("  Verifying Module-LWE solution\n");

    printf("\n");
    PQC_PrintStep("3", "Verification Result");

    if(is_valid)
    {
        PQC_PrintSuccess("Signature is VALID ✓");
        printf("%s  ✓ Message authenticity confirmed%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);
        printf("%s  ✓ Sender identity verified%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);
        printf("%s  ✓ Message integrity intact%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);
    }
    else
    {
        PQC_PrintError("Signature is INVALID ✗");
        printf("%s  ✗ Message may be tampered%s\n", PQC_COLOR_RED, PQC_COLOR_RESET);
        printf("%s  ✗ Reject this message%s\n", PQC_COLOR_RED, PQC_COLOR_RESET);
    }
}

/********************************************************************************************************/
/********************************************ML-KEM Visualization****************************************/
/********************************************************************************************************/

/**
 * @brief Visualize ML-KEM key generation
 */
static inline void PQC_VisualizeMLKEM_KeyGen(const uint8* public_key, uint32 pk_len,
                                              const uint8* secret_key, uint32 sk_len)
{
    PQC_PrintSectionHeader("ML-KEM-768 KEY GENERATION");

    PQC_PrintAlgorithmInfo(
        "ML-KEM-768 (Module-Lattice Key Encapsulation Mechanism)",
        "NIST FIPS 203 (August 2024)",
        "NIST Level 3 (AES-192 equivalent)"
    );

    printf("\n");
    PQC_PrintStep("1", "Lattice Parameter Generation");
    printf("  Module dimension: 3\n");
    printf("  Polynomial degree: 256\n");
    printf("  Modulus: 3329\n");

    printf("\n");
    PQC_PrintStep("2", "Key Pair Generated");
    PQC_PrintHexDump("Public Key (for sharing)", public_key, pk_len, 32);
    printf("  Public Key Size: %u bytes\n", pk_len);
    printf("  %s(Can be shared openly)%s\n", PQC_COLOR_GREEN, PQC_COLOR_RESET);

    printf("\n");
    printf("  %sSecret Key%s: %u bytes %s(CONFIDENTIAL - Not displayed)%s\n",
           PQC_COLOR_YELLOW, PQC_COLOR_RESET, sk_len,
           PQC_COLOR_RED, PQC_COLOR_RESET);

    printf("\n");
    PQC_PrintSuccess("ML-KEM-768 key pair generated!");
}

/**
 * @brief Visualize ML-KEM encapsulation (create shared secret)
 */
static inline void PQC_VisualizeMLKEM_Encapsulate(const uint8* public_key, uint32 pk_len,
                                                   const uint8* ciphertext, uint32 ct_len,
                                                   const uint8* shared_secret, uint32 ss_len)
{
    PQC_PrintSectionHeader("ML-KEM-768 ENCAPSULATION (Initiator Side)");

    printf("\n");
    PQC_PrintStep("1", "Input: Peer's Public Key");
    PQC_PrintHexDump("Public Key Received", public_key, pk_len, 32);

    printf("\n");
    PQC_PrintStep("2", "Generate Shared Secret + Ciphertext");
    printf("  Random ephemeral key generated\n");
    printf("  Lattice encryption performed\n");

    printf("\n");
    PQC_PrintStep("3", "Output: Ciphertext (to send to peer)");
    PQC_PrintHexDump("Ciphertext", ciphertext, ct_len, 32);
    printf("  Ciphertext Size: %u bytes\n", ct_len);
    printf("  %s(Send this to peer via network)%s\n", PQC_COLOR_YELLOW, PQC_COLOR_RESET);

    printf("\n");
    PQC_PrintStep("4", "Shared Secret Established");
    PQC_PrintHexDump("Shared Secret", shared_secret, ss_len, 32);
    printf("  %sShared Secret: %u bytes (USE FOR ENCRYPTION)%s\n",
           PQC_COLOR_GREEN, ss_len, PQC_COLOR_RESET);

    printf("\n");
    PQC_PrintSuccess("Key exchange initiated! Send ciphertext to peer.");
}

/**
 * @brief Visualize ML-KEM decapsulation (extract shared secret)
 */
static inline void PQC_VisualizeMLKEM_Decapsulate(const uint8* ciphertext, uint32 ct_len,
                                                   const uint8* shared_secret, uint32 ss_len)
{
    PQC_PrintSectionHeader("ML-KEM-768 DECAPSULATION (Responder Side)");

    printf("\n");
    PQC_PrintStep("1", "Input: Received Ciphertext");
    PQC_PrintHexDump("Ciphertext from Peer", ciphertext, ct_len, 32);

    printf("\n");
    PQC_PrintStep("2", "Decapsulate using Secret Key");
    printf("  Apply secret key to ciphertext\n");
    printf("  Solve lattice problem\n");
    printf("  Extract shared secret\n");

    printf("\n");
    PQC_PrintStep("3", "Shared Secret Recovered");
    PQC_PrintHexDump("Shared Secret", shared_secret, ss_len, 32);
    printf("  %sBoth sides now have SAME shared secret!%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);

    printf("\n");
    PQC_PrintSuccess("Key exchange complete! Ready for secure communication.");
    printf("%s  🔐 Shared secret established without transmitting it!%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
}

/**
 * @brief Visualize complete key exchange flow
 */
static inline void PQC_VisualizeMLKEM_FullFlow(void)
{
    PQC_PrintSectionHeader("ML-KEM-768 KEY EXCHANGE COMPLETE FLOW");

    printf("\n%sAlice (Initiator)%s         %s<========>%s         %sBob (Responder)%s\n",
           PQC_COLOR_CYAN, PQC_COLOR_RESET,
           PQC_COLOR_YELLOW, PQC_COLOR_RESET,
           PQC_COLOR_MAGENTA, PQC_COLOR_RESET);

    printf("\n");
    printf("[1] Bob generates key pair\n");
    printf("    Bob: (pk, sk) = KeyGen()\n");
    printf("    Bob → Alice: pk (public key)\n");

    printf("\n");
    printf("[2] Alice encapsulates\n");
    printf("    Alice: (ct, ss1) = Encapsulate(pk)\n");
    printf("    Alice → Bob: ct (ciphertext)\n");
    printf("    Alice keeps: ss1 (shared secret)\n");

    printf("\n");
    printf("[3] Bob decapsulates\n");
    printf("    Bob: ss2 = Decapsulate(ct, sk)\n");
    printf("    Bob has: ss2 (shared secret)\n");

    printf("\n");
    printf("%s[4] Result: ss1 == ss2 (Shared secret established!)%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
    printf("    %s✓ No eavesdropper can derive the secret%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
    printf("    %s✓ Quantum-resistant key exchange%s\n",
           PQC_COLOR_GREEN, PQC_COLOR_RESET);
}

#endif /* PQC_VISUALIZATION_H */
