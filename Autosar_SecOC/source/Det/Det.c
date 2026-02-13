/**
 * @file Det.c
 * @brief Implementation of Default Error Tracer (DET)
 * @details This file contains the implementation of the Default Error Tracer module
 *          as defined in AUTOSAR CP R24-11.
 */

#include "Det.h"
#include <stdio.h>

/* --- Global Data --- */

static uint8 Det_Initialized = 0;

/* --- Function Definitions --- */

/**
 * @brief Service to initialize the Default Error Tracer.
 */
void Det_Init(const Det_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    Det_Initialized = 1;
    printf("[DET] Initialized\n");
}

/**
 * @brief Service to start the Default Error Tracer.
 */
void Det_Start(void)
{
    if (Det_Initialized == 1)
    {
        printf("[DET] Started\n");
    }
}

/**
 * @brief Service to report development errors.
 */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    /* [SWS_Det_00009] Service to report development errors. */
    printf("[DET] Development Error Reported:\n");
    printf("      ModuleId:   %u\n", ModuleId);
    printf("      InstanceId: %u\n", InstanceId);
    printf("      ApiId:      %u\n", ApiId);
    printf("      ErrorId:    %u\n", ErrorId);

    /* In a real embedded system, this might trigger a breakpoint or ECU reset */
    return E_OK;
}

/**
 * @brief Service to report runtime errors.
 */
Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    /* [SWS_Det_01001] Service to report runtime errors. */
    printf("[DET] Runtime Error Reported:\n");
    printf("      ModuleId:   %u\n", ModuleId);
    printf("      InstanceId: %u\n", InstanceId);
    printf("      ApiId:      %u\n", ApiId);
    printf("      ErrorId:    %u\n", ErrorId);

    return E_OK;
}
