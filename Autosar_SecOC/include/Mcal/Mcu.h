/**
 * @file Mcu.h
 * @brief AUTOSAR MCAL MCU Driver API
 * @details Provides microcontroller initialization, clock information,
 *          reset control, and system identification.
 *          On Pi 4: reads /proc/cpuinfo, manages system clock queries.
 */

#ifndef MCU_H
#define MCU_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Mcal_Cfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Service IDs */
#define MCU_SID_INIT                    ((uint8)0x00U)
#define MCU_SID_DEINIT                  ((uint8)0x01U)
#define MCU_SID_GET_RESET_REASON        ((uint8)0x05U)
#define MCU_SID_PERFORM_RESET           ((uint8)0x07U)
#define MCU_SID_GET_VERSION_INFO        ((uint8)0x09U)

/* DET Error Codes */
#define MCU_E_UNINIT                    ((uint8)0x01U)
#define MCU_E_PARAM_POINTER             ((uint8)0x02U)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

/** @brief MCU reset reason */
typedef enum
{
    MCU_RESET_UNDEFINED = 0U,
    MCU_POWER_ON_RESET,
    MCU_WATCHDOG_RESET,
    MCU_SW_RESET,
    MCU_RESET_REASON_COUNT
} Mcu_ResetType;

/** @brief MCU clock status */
typedef enum
{
    MCU_PLL_LOCKED = 0U,
    MCU_PLL_UNLOCKED,
    MCU_PLL_STATUS_UNDEFINED
} Mcu_PllStatusType;

/** @brief Hardware identification */
typedef struct
{
    uint32 CpuFreqHz;              /**< CPU frequency in Hz */
    uint32 RamSizeBytes;           /**< Total RAM in bytes */
    uint8  CpuCoreCount;           /**< Number of CPU cores */
    char   HardwareRevision[16];   /**< Hardware revision string */
    char   SerialNumber[20];       /**< Board serial number */
} Mcu_HwInfoType;

/** @brief MCU configuration (post-build) */
typedef struct
{
    uint8 Placeholder;             /**< Reserved for future use */
} Mcu_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the MCU driver
 * @param ConfigPtr Configuration pointer (may be NULL for default)
 */
void Mcu_Init(const Mcu_ConfigType* ConfigPtr);

/**
 * @brief De-initialize the MCU driver
 */
void Mcu_DeInit(void);

/**
 * @brief Get the reason for the last reset
 * @return Reset reason
 */
Mcu_ResetType Mcu_GetResetReason(void);

/**
 * @brief Get PLL lock status
 * @return PLL status
 */
Mcu_PllStatusType Mcu_GetPllStatus(void);

/**
 * @brief Get hardware identification information
 * @param HwInfoPtr Pointer to store hardware info
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType Mcu_GetHwInfo(Mcu_HwInfoType* HwInfoPtr);

/**
 * @brief Perform a software reset
 */
void Mcu_PerformReset(void);

/**
 * @brief Get version information
 * @param VersionInfoPtr Pointer to store version info
 */
void Mcu_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

#endif /* MCU_H */
