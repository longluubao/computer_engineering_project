/**
 * @file Mcal_Cfg.h
 * @brief MCAL Platform Selection and Configuration
 * @details Selects the target hardware platform for the MCAL layer.
 *          Currently supported targets:
 *          - MCAL_TARGET_PI4:  Raspberry Pi 4 (BCM2711, HPC Gateway)
 *          - MCAL_TARGET_SIM:  Software simulation (desktop testing)
 *
 *          When distributing to other devices, add a new target here
 *          and provide the corresponding source/Mcal/<Target>/ directory.
 */

#ifndef MCAL_CFG_H
#define MCAL_CFG_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* ---- Target identifiers (plain integers for preprocessor #if) ---- */
#define MCAL_TARGET_SIM                 0
#define MCAL_TARGET_PI4                 1

/* ---- Active target selection ----
 * Override via CMake: -DMCAL_TARGET=MCAL_TARGET_PI4
 * Default: PI4 on Linux, SIM on Windows */
#ifndef MCAL_TARGET
    #if defined(LINUX)
        #define MCAL_TARGET             MCAL_TARGET_PI4
    #else
        #define MCAL_TARGET             MCAL_TARGET_SIM
    #endif
#endif

/* ---- Convenience macros ---- */
#define MCAL_IS_PI4()                   (MCAL_TARGET == MCAL_TARGET_PI4)
#define MCAL_IS_SIM()                   (MCAL_TARGET == MCAL_TARGET_SIM)

/* ---- Module enable switches ---- */
#define MCAL_MCU_ENABLED                (STD_ON)
#define MCAL_GPT_ENABLED                (STD_ON)
#define MCAL_WDG_ENABLED                (STD_ON)
#define MCAL_DIO_ENABLED                (STD_ON)
#define MCAL_CAN_HW_ENABLED            (STD_ON)

/* ---- Pi 4 specific configuration ---- */
#if MCAL_IS_PI4()

    /* SocketCAN interface name (e.g. can0, vcan0) */
    #define MCAL_CAN_INTERFACE_NAME     "can0"

    /* Maximum number of SocketCAN controllers */
    #define MCAL_CAN_MAX_CONTROLLERS    ((uint8)2U)

    /* CAN FD support (Pi 4 MCP2515 via SPI typically does not) */
    #define MCAL_CAN_FD_SUPPORT         (STD_OFF)

    /* GPIO chip device for libgpiod / sysfs */
    #define MCAL_DIO_GPIO_CHIP          "/dev/gpiochip0"
    #define MCAL_DIO_MAX_CHANNELS       ((uint8)28U)

    /* Watchdog device */
    #define MCAL_WDG_DEVICE             "/dev/watchdog"
    #define MCAL_WDG_TIMEOUT_SEC        ((uint8)15U)

    /* GPT timer resolution */
    #define MCAL_GPT_TICK_NS            ((uint32)1000000U)  /* 1 ms */

#endif /* MCAL_IS_PI4 */

/* ---- DET integration ---- */
#define MCAL_DEV_ERROR_DETECT           (STD_ON)

/* ---- Module IDs (AUTOSAR BSW module numbering, vendor-specific range 0x80+) ---- */
#define MCU_MODULE_ID                   ((uint16)101U)
#define GPT_MODULE_ID                   ((uint16)100U)
#define WDG_MODULE_ID                   ((uint16)102U)
#define DIO_MODULE_ID                   ((uint16)120U)

#define MCAL_INSTANCE_ID                ((uint8)0x00U)

#endif /* MCAL_CFG_H */
