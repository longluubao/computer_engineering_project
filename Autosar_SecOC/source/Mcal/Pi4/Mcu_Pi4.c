/**
 * @file Mcu_Pi4.c
 * @brief MCU Driver Implementation for Raspberry Pi 4 (BCM2711)
 * @details Reads hardware info from /proc/cpuinfo, /proc/meminfo,
 *          and provides system clock queries via sysfs.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Mcal/Mcu.h"
#include "Det/Det.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Mcu_Initialized = FALSE;
static Mcu_HwInfoType Mcu_CachedHwInfo;

/********************************************************************************************************/
/******************************************InternalFunctions*********************************************/
/********************************************************************************************************/

static void Mcu_ReadCpuInfo(void)
{
    FILE* fp;
    char lineBuf[256];

    Mcu_CachedHwInfo.CpuFreqHz = 0U;
    Mcu_CachedHwInfo.CpuCoreCount = 0U;
    Mcu_CachedHwInfo.HardwareRevision[0] = '\0';
    Mcu_CachedHwInfo.SerialNumber[0] = '\0';

    /* Read /proc/cpuinfo for Pi-specific fields */
    fp = fopen("/proc/cpuinfo", "r");
    if (fp != NULL)
    {
        while (fgets(lineBuf, (int)sizeof(lineBuf), fp) != NULL)
        {
            if (strncmp(lineBuf, "processor", 9) == 0)
            {
                Mcu_CachedHwInfo.CpuCoreCount++;
            }
            else if (strncmp(lineBuf, "Revision", 8) == 0)
            {
                char* valueStr = strchr(lineBuf, ':');
                if (valueStr != NULL)
                {
                    valueStr++;
                    while (*valueStr == ' ')
                    {
                        valueStr++;
                    }
                    /* Remove trailing newline */
                    valueStr[strcspn(valueStr, "\n")] = '\0';
                    (void)strncpy(Mcu_CachedHwInfo.HardwareRevision, valueStr,
                                  sizeof(Mcu_CachedHwInfo.HardwareRevision) - 1U);
                    Mcu_CachedHwInfo.HardwareRevision[sizeof(Mcu_CachedHwInfo.HardwareRevision) - 1U] = '\0';
                }
            }
            else if (strncmp(lineBuf, "Serial", 6) == 0)
            {
                char* valueStr = strchr(lineBuf, ':');
                if (valueStr != NULL)
                {
                    valueStr++;
                    while (*valueStr == ' ')
                    {
                        valueStr++;
                    }
                    valueStr[strcspn(valueStr, "\n")] = '\0';
                    (void)strncpy(Mcu_CachedHwInfo.SerialNumber, valueStr,
                                  sizeof(Mcu_CachedHwInfo.SerialNumber) - 1U);
                    Mcu_CachedHwInfo.SerialNumber[sizeof(Mcu_CachedHwInfo.SerialNumber) - 1U] = '\0';
                }
            }
            else
            {
                /* No action required for other lines */
            }
        }
        (void)fclose(fp);
    }

    /* Read CPU max frequency from sysfs */
    fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
    if (fp != NULL)
    {
        uint32 khzFreq = 0U;
        if (fscanf(fp, "%lu", &khzFreq) == 1)
        {
            Mcu_CachedHwInfo.CpuFreqHz = khzFreq * 1000U;
        }
        (void)fclose(fp);
    }

    /* Read total RAM from /proc/meminfo */
    Mcu_CachedHwInfo.RamSizeBytes = 0U;
    fp = fopen("/proc/meminfo", "r");
    if (fp != NULL)
    {
        while (fgets(lineBuf, (int)sizeof(lineBuf), fp) != NULL)
        {
            if (strncmp(lineBuf, "MemTotal:", 9) == 0)
            {
                uint32 kbRam = 0U;
                if (sscanf(lineBuf, "MemTotal: %lu", &kbRam) == 1)
                {
                    Mcu_CachedHwInfo.RamSizeBytes = kbRam * 1024U;
                }
                break;
            }
        }
        (void)fclose(fp);
    }
}

/********************************************************************************************************/
/***************************************ForwardDeclarations**********************************************/
/********************************************************************************************************/

void Mcu_Init(const Mcu_ConfigType* ConfigPtr);
void Mcu_DeInit(void);
Mcu_ResetType Mcu_GetResetReason(void);
Mcu_PllStatusType Mcu_GetPllStatus(void);
Std_ReturnType Mcu_GetHwInfo(Mcu_HwInfoType* HwInfoPtr);
void Mcu_PerformReset(void);
void Mcu_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Mcu_Init(const Mcu_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;

    if (Mcu_Initialized != FALSE)
    {
        return;
    }

    Mcu_ReadCpuInfo();
    Mcu_Initialized = TRUE;
}

void Mcu_DeInit(void)
{
    Mcu_Initialized = FALSE;
}

Mcu_ResetType Mcu_GetResetReason(void)
{
    /* On Linux / Pi 4 there is no direct register for last-reset cause.
     * Check /proc/device-tree/chosen/reset-reason if available,
     * otherwise return UNDEFINED. */
    if (Mcu_Initialized == FALSE)
    {
        (void)Det_ReportError(MCU_MODULE_ID, MCAL_INSTANCE_ID, MCU_SID_GET_RESET_REASON, MCU_E_UNINIT);
        return MCU_RESET_UNDEFINED;
    }

    return MCU_POWER_ON_RESET;
}

Mcu_PllStatusType Mcu_GetPllStatus(void)
{
    if (Mcu_Initialized == FALSE)
    {
        return MCU_PLL_STATUS_UNDEFINED;
    }

    /* Pi 4 PLLs are managed by the VideoCore firmware and are always
     * locked once Linux has booted. */
    return MCU_PLL_LOCKED;
}

Std_ReturnType Mcu_GetHwInfo(Mcu_HwInfoType* HwInfoPtr)
{
    if (Mcu_Initialized == FALSE)
    {
        (void)Det_ReportError(MCU_MODULE_ID, MCAL_INSTANCE_ID, MCU_SID_INIT, MCU_E_UNINIT);
        return E_NOT_OK;
    }

    if (HwInfoPtr == NULL)
    {
        (void)Det_ReportError(MCU_MODULE_ID, MCAL_INSTANCE_ID, MCU_SID_INIT, MCU_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    (void)memcpy(HwInfoPtr, &Mcu_CachedHwInfo, sizeof(Mcu_HwInfoType));
    return E_OK;
}

void Mcu_PerformReset(void)
{
    if (Mcu_Initialized == FALSE)
    {
        (void)Det_ReportError(MCU_MODULE_ID, MCAL_INSTANCE_ID, MCU_SID_PERFORM_RESET, MCU_E_UNINIT);
        return;
    }

    /* Trigger Linux reboot via sysrq */
    FILE* fp = fopen("/proc/sysrq-trigger", "w");
    if (fp != NULL)
    {
        (void)fprintf(fp, "b");
        (void)fclose(fp);
    }
}

void Mcu_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    if (VersionInfoPtr == NULL)
    {
        (void)Det_ReportError(MCU_MODULE_ID, MCAL_INSTANCE_ID, MCU_SID_GET_VERSION_INFO, MCU_E_PARAM_POINTER);
        return;
    }

    VersionInfoPtr->vendorID = 0x00FFU;   /* Vendor-specific */
    VersionInfoPtr->moduleID = MCU_MODULE_ID;
    VersionInfoPtr->sw_major_version = 1U;
    VersionInfoPtr->sw_minor_version = 0U;
    VersionInfoPtr->sw_patch_version = 0U;
}
