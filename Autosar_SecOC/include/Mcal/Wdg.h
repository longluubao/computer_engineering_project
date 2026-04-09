/**
 * @file Wdg.h
 * @brief AUTOSAR MCAL Watchdog Driver API
 * @details Provides watchdog timer control for system reliability.
 *          On Pi 4: uses /dev/watchdog (BCM2835 watchdog hardware).
 *          Requires CONFIG_WATCHDOG=y in kernel config.
 */

#ifndef WDG_H
#define WDG_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Mcal_Cfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Service IDs */
#define WDG_SID_INIT                    ((uint8)0x00U)
#define WDG_SID_DEINIT                  ((uint8)0x01U)
#define WDG_SID_SET_MODE                ((uint8)0x02U)
#define WDG_SID_TRIGGER                 ((uint8)0x03U)

/* DET Error Codes */
#define WDG_E_UNINIT                    ((uint8)0x01U)
#define WDG_E_PARAM_MODE                ((uint8)0x02U)
#define WDG_E_DRIVER_STATE              ((uint8)0x03U)
#define WDG_E_INIT_FAILED               ((uint8)0x04U)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

/** @brief Watchdog mode */
typedef enum
{
    WDG_MODE_OFF = 0U,
    WDG_MODE_SLOW,              /**< Long timeout (e.g. 30 s) */
    WDG_MODE_FAST               /**< Short timeout (e.g. 5 s) */
} Wdg_ModeType;

/** @brief Watchdog configuration */
typedef struct
{
    Wdg_ModeType InitialMode;
    uint8 SlowTimeoutSec;       /**< Timeout in SLOW mode */
    uint8 FastTimeoutSec;       /**< Timeout in FAST mode */
} Wdg_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the watchdog driver
 * @param ConfigPtr Configuration pointer
 */
void Wdg_Init(const Wdg_ConfigType* ConfigPtr);

/**
 * @brief De-initialize the watchdog (disables watchdog)
 */
void Wdg_DeInit(void);

/**
 * @brief Switch watchdog mode
 * @param Mode Target mode (OFF, SLOW, FAST)
 * @return E_OK on success
 */
Std_ReturnType Wdg_SetMode(Wdg_ModeType Mode);

/**
 * @brief Trigger (kick) the watchdog to prevent reset
 * @details Must be called periodically from the OS main loop.
 */
void Wdg_Trigger(void);

#endif /* WDG_H */
