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
#define DET_SW_MINOR_VERSION     (0U)
#define DET_SW_PATCH_VERSION     (0U)

/* --- Service IDs --- */
#define DET_SID_INIT             (0x00U)
#define DET_SID_REPORTERROR      (0x01U)
#define DET_SID_START            (0x02U)
#define DET_SID_REPORTRUNTIMEERROR (0x04U)
#define DET_SID_REPORTTRANSIENTFAULT (0x05U)

/* --- Type Definitions --- */

/**
 * @brief Configuration type for DET
 * @implements [SWS_Det_00042]
 */
typedef struct {
    /* Implementation specific configuration parameters */
    uint8 dummy;
} Det_ConfigType;

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
 * @param[in] ModuleId Module ID of calling module.
 * @param[in] InstanceId The identifier of the index based instance of a module.
 * @param[in] ApiId ID of API service in which error is detected.
 * @param[in] ErrorId ID of detected development error.
 * @return Std_ReturnType returns always E_OK.
 * @implements [SWS_Det_00009]
 */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/**
 * @brief Service to report runtime errors.
 * @param[in] ModuleId Module ID of calling module.
 * @param[in] InstanceId The identifier of the index based instance of a module.
 * @param[in] ApiId ID of API service in which error is detected.
 * @param[in] ErrorId ID of detected runtime error.
 * @return Std_ReturnType returns always E_OK.
 * @implements [SWS_Det_01001]
 */
Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

#endif /* DET_H */
