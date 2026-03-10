/**
 * @file Det.c
 * @brief Implementation of Default Error Tracer (DET)
 * @details This file contains the implementation of the Default Error Tracer module
 *          as defined in AUTOSAR CP R24-11.
 */

#include "Det.h"
#include "Dem.h"
#include <stdio.h>
#include <string.h>

/* --- Global Data --- */

static uint8 Det_Initialized = FALSE;

/* Circular error log buffer */
static Det_ErrorEntryType Det_ErrorLog[DET_ERROR_LOG_SIZE];
static uint8 Det_ErrorLogHead  = 0U;  /* Next write position */
static uint8 Det_ErrorLogCount = 0U;  /* Number of entries stored */

/* --- Internal Functions --- */

static void Det_LogError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId, uint8 ErrorType)
{
    Det_ErrorLog[Det_ErrorLogHead].ModuleId   = ModuleId;
    Det_ErrorLog[Det_ErrorLogHead].InstanceId = InstanceId;
    Det_ErrorLog[Det_ErrorLogHead].ApiId      = ApiId;
    Det_ErrorLog[Det_ErrorLogHead].ErrorId    = ErrorId;
    Det_ErrorLog[Det_ErrorLogHead].ErrorType  = ErrorType;

    Det_ErrorLogHead = (uint8)((Det_ErrorLogHead + 1U) % DET_ERROR_LOG_SIZE);

    if (Det_ErrorLogCount < DET_ERROR_LOG_SIZE)
    {
        Det_ErrorLogCount++;
    }
}

/* --- Function Definitions --- */

void Det_Init(const Det_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;

    (void)memset(Det_ErrorLog, 0, sizeof(Det_ErrorLog));
    Det_ErrorLogHead  = 0U;
    Det_ErrorLogCount = 0U;
    Det_Initialized   = TRUE;

    printf("[DET] Initialized\n");
}

void Det_Start(void)
{
    if (Det_Initialized == TRUE)
    {
        printf("[DET] Started\n");
    }
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    printf("[DET] Dev Error: Module=%u Inst=%u Api=%u Err=%u\n",
           ModuleId, InstanceId, ApiId, ErrorId);

    if (Det_Initialized == TRUE)
    {
        Det_LogError(ModuleId, InstanceId, ApiId, ErrorId, DET_ERROR_TYPE_DEVELOPMENT);
        (void)Dem_ReportDetError(ModuleId, InstanceId, ApiId, ErrorId);
    }

    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    printf("[DET] Runtime Error: Module=%u Inst=%u Api=%u Err=%u\n",
           ModuleId, InstanceId, ApiId, ErrorId);

    if (Det_Initialized == TRUE)
    {
        Det_LogError(ModuleId, InstanceId, ApiId, ErrorId, DET_ERROR_TYPE_RUNTIME);
        (void)Dem_ReportDetError(ModuleId, InstanceId, ApiId, ErrorId);
    }

    return E_OK;
}

Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId)
{
    printf("[DET] Transient Fault: Module=%u Inst=%u Api=%u Fault=%u\n",
           ModuleId, InstanceId, ApiId, FaultId);

    if (Det_Initialized == TRUE)
    {
        Det_LogError(ModuleId, InstanceId, ApiId, FaultId, DET_ERROR_TYPE_TRANSIENT);
    }

    return E_OK;
}

void Det_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL)
    {
        (void)Det_ReportError(DET_MODULE_ID, 0U, DET_SID_GETVERSIONINFO, DET_E_PARAM_POINTER);
        return;
    }

    versioninfo->vendorID         = DET_VENDOR_ID;
    versioninfo->moduleID         = DET_MODULE_ID;
    versioninfo->sw_major_version = DET_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DET_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DET_SW_PATCH_VERSION;
}

uint8 Det_GetErrorCount(void)
{
    return Det_ErrorLogCount;
}

Std_ReturnType Det_GetErrorEntry(uint8 Index, Det_ErrorEntryType* Entry)
{
    uint8 actualIndex;

    if (Entry == NULL)
    {
        return E_NOT_OK;
    }

    if (Index >= Det_ErrorLogCount)
    {
        return E_NOT_OK;
    }

    /* Calculate actual buffer index: oldest entry first */
    if (Det_ErrorLogCount < DET_ERROR_LOG_SIZE)
    {
        actualIndex = Index;
    }
    else
    {
        actualIndex = (uint8)((Det_ErrorLogHead + Index) % DET_ERROR_LOG_SIZE);
    }

    *Entry = Det_ErrorLog[actualIndex];
    return E_OK;
}
