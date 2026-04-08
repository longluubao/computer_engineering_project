/**
 * @file Wdg_Pi4.c
 * @brief Watchdog Driver Implementation for Raspberry Pi 4
 * @details Uses the Linux watchdog API (/dev/watchdog) backed by the
 *          BCM2835 hardware watchdog on the Pi 4.
 *          Requires: kernel CONFIG_WATCHDOG=y, CONFIG_BCM2835_WDT=y.
 *
 *          If /dev/watchdog is not available (e.g. in development),
 *          the driver operates in software-only fallback mode.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Mcal/Wdg.h"
#include "Det/Det.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Wdg_Initialized = FALSE;
static Wdg_ModeType Wdg_CurrentMode = WDG_MODE_OFF;
static int Wdg_Fd = -1;
static uint8 Wdg_SlowTimeout = 15U;
static uint8 Wdg_FastTimeout = 5U;

/********************************************************************************************************/
/***************************************ForwardDeclarations**********************************************/
/********************************************************************************************************/

void Wdg_Init(const Wdg_ConfigType* ConfigPtr);
void Wdg_DeInit(void);
Std_ReturnType Wdg_SetMode(Wdg_ModeType Mode);
void Wdg_Trigger(void);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Wdg_Init(const Wdg_ConfigType* ConfigPtr)
{
    if (Wdg_Initialized != FALSE)
    {
        (void)Det_ReportError(WDG_MODULE_ID, MCAL_INSTANCE_ID, WDG_SID_INIT, WDG_E_DRIVER_STATE);
        return;
    }

    if (ConfigPtr != NULL)
    {
        Wdg_SlowTimeout = ConfigPtr->SlowTimeoutSec;
        Wdg_FastTimeout = ConfigPtr->FastTimeoutSec;
    }
    else
    {
        Wdg_SlowTimeout = MCAL_WDG_TIMEOUT_SEC;
        Wdg_FastTimeout = 5U;
    }

    /* Attempt to open the hardware watchdog device */
    Wdg_Fd = open(MCAL_WDG_DEVICE, O_RDWR);
    if (Wdg_Fd < 0)
    {
        /* Hardware watchdog not available; continue in software-only mode.
         * This is normal during development or in containers. */
        (void)printf("[Wdg] /dev/watchdog not available, running in software-only mode\n");
    }

    Wdg_CurrentMode = WDG_MODE_OFF;
    Wdg_Initialized = TRUE;

    /* Apply initial mode if specified */
    if ((ConfigPtr != NULL) && (ConfigPtr->InitialMode != WDG_MODE_OFF))
    {
        (void)Wdg_SetMode(ConfigPtr->InitialMode);
    }
}

void Wdg_DeInit(void)
{
    if (Wdg_Initialized == FALSE)
    {
        (void)Det_ReportError(WDG_MODULE_ID, MCAL_INSTANCE_ID, WDG_SID_DEINIT, WDG_E_UNINIT);
        return;
    }

    if (Wdg_Fd >= 0)
    {
        /* Write magic close character 'V' to disable watchdog on close */
        (void)write(Wdg_Fd, "V", 1);
        (void)close(Wdg_Fd);
        Wdg_Fd = -1;
    }

    Wdg_CurrentMode = WDG_MODE_OFF;
    Wdg_Initialized = FALSE;
}

Std_ReturnType Wdg_SetMode(Wdg_ModeType Mode)
{
    int timeout;

    if (Wdg_Initialized == FALSE)
    {
        (void)Det_ReportError(WDG_MODULE_ID, MCAL_INSTANCE_ID, WDG_SID_SET_MODE, WDG_E_UNINIT);
        return E_NOT_OK;
    }

    if (Mode > WDG_MODE_FAST)
    {
        (void)Det_ReportError(WDG_MODULE_ID, MCAL_INSTANCE_ID, WDG_SID_SET_MODE, WDG_E_PARAM_MODE);
        return E_NOT_OK;
    }

    if (Mode == WDG_MODE_OFF)
    {
        /* Disable the watchdog by writing the magic close character */
        if (Wdg_Fd >= 0)
        {
            (void)write(Wdg_Fd, "V", 1);
            (void)close(Wdg_Fd);
            Wdg_Fd = -1;
        }
        Wdg_CurrentMode = WDG_MODE_OFF;
        return E_OK;
    }

    /* Re-open if needed */
    if (Wdg_Fd < 0)
    {
        Wdg_Fd = open(MCAL_WDG_DEVICE, O_RDWR);
        if (Wdg_Fd < 0)
        {
            return E_NOT_OK;
        }
    }

    /* Set the timeout based on mode */
    if (Mode == WDG_MODE_SLOW)
    {
        timeout = (int)Wdg_SlowTimeout;
    }
    else
    {
        timeout = (int)Wdg_FastTimeout;
    }

    if (ioctl(Wdg_Fd, WDIOC_SETTIMEOUT, &timeout) != 0)
    {
        return E_NOT_OK;
    }

    Wdg_CurrentMode = Mode;
    return E_OK;
}

void Wdg_Trigger(void)
{
    if (Wdg_Initialized == FALSE)
    {
        return;
    }

    if ((Wdg_CurrentMode != WDG_MODE_OFF) && (Wdg_Fd >= 0))
    {
        /* Kick the watchdog by writing any data */
        (void)write(Wdg_Fd, "\0", 1);
    }
}
