/**
 * @file Det.h
 * @brief Specification of Default Error Tracer (DET)
 * @details This file contains the API definitions for the Default Error Tracer module
 *          as defined in AUTOSAR CP R24-11.
 */

#ifndef DET_H
#define DET_H

#include "Std_Types.h"

/* --- Module Identification --- */
#define DET_MODULE_ID            (15U)
#define DET_VENDOR_ID            (0U)
#define DET_AR_RELEASE_MAJOR_VERSION (4U)
#define DET_AR_RELEASE_MINOR_VERSION (0U)
#define DET_AR_RELEASE_REVISION_VERSION (3U)
#define DET_SW_MAJOR_VERSION     (1U)
#define DET_SW_MINOR_VERSION     (1U)
#define DET_SW_PATCH_VERSION     (0U)

/* --- Configuration --- */
#define DET_ERROR_LOG_SIZE       (32U)

/* --- Service IDs --- */
#define DET_SID_INIT                   (0x00U)
#define DET_SID_REPORTERROR            (0x01U)
#define DET_SID_START                  (0x02U)
#define DET_SID_GETVERSIONINFO         (0x03U)
#define DET_SID_REPORTRUNTIMEERROR     (0x04U)
#define DET_SID_REPORTTRANSIENTFAULT   (0x05U)

/* --- DET Error Codes --- */
#define DET_E_PARAM_POINTER      (0x01U)

/* --- Type Definitions --- */

/**
 * @brief Configuration type for DET
 * @implements [SWS_Det_00042]
 */
typedef struct {
    uint8 dummy;
} Det_ConfigType;

/**
 * @brief Error log entry for circular buffer
 */
typedef struct {
    uint16 ModuleId;
    uint8  InstanceId;
    uint8  ApiId;
    uint8  ErrorId;
    uint8  ErrorType;   /* 0=Dev, 1=Runtime, 2=Transient */
} Det_ErrorEntryType;

/* Error type constants */
#define DET_ERROR_TYPE_DEVELOPMENT  (0U)
#define DET_ERROR_TYPE_RUNTIME      (1U)
#define DET_ERROR_TYPE_TRANSIENT    (2U)

/* --- Function Prototypes --- */

/**
 * @brief Service to initialize the Default Error Tracer.
 * @param[in] ConfigPtr Pointer to the selected configuration set.
 * @implements [SWS_Det_00008]
 */
void Det_Init(const Det_ConfigType* ConfigPtr);

/**
 * @brief Service to start the Default Error Tracer.
 * @implements [SWS_Det_00010]
 */
void Det_Start(void);

/**
 * @brief Service to report development errors.
 * @implements [SWS_Det_00009]
 */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/**
 * @brief Service to report runtime errors.
 * @implements [SWS_Det_01001]
 */
Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/**
 * @brief Service to report transient faults.
 * @implements [SWS_Det_01003]
 */
Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId);

/**
 * @brief Return the version information of the DET module.
 * @param[out] versioninfo Pointer to version info structure.
 * @implements [SWS_Det_00011]
 */
void Det_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Get the number of logged errors.
 * @return Number of errors currently in the log buffer.
 */
uint8 Det_GetErrorCount(void);

/**
 * @brief Get an error entry from the log buffer.
 * @param[in]  Index Index into the log (0 = oldest available).
 * @param[out] Entry Pointer to store the error entry.
 * @return E_OK if valid, E_NOT_OK if index out of range.
 */
Std_ReturnType Det_GetErrorEntry(uint8 Index, Det_ErrorEntryType* Entry);

#endif /* DET_H */
