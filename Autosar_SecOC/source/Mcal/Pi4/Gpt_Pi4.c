/**
 * @file Gpt_Pi4.c
 * @brief GPT Driver Implementation for Raspberry Pi 4
 * @details Uses Linux timerfd for periodic timer channels and
 *          CLOCK_MONOTONIC for high-resolution timestamps.
 *          Each channel creates a timerfd; notifications fire
 *          when the timer expires (polled from Gpt main function
 *          or via OS integration).
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Mcal/Gpt.h"
#include "Det/Det.h"
#include <time.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <string.h>
#include <stdint.h>

/********************************************************************************************************/
/******************************************InternalTypes*************************************************/
/********************************************************************************************************/

typedef enum
{
    GPT_CH_STATE_STOPPED = 0U,
    GPT_CH_STATE_RUNNING
} Gpt_ChannelStateType;

typedef struct
{
    Gpt_ChannelStateType State;
    int TimerFd;                        /**< Linux timerfd descriptor */
    Gpt_ValueType TargetTicks;          /**< Configured tick count */
    uint64 StartTimeNs;                 /**< Monotonic start time */
    boolean NotificationEnabled;
    const Gpt_ChannelConfigType* Config;
} Gpt_ChannelControlType;

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Gpt_Initialized = FALSE;
static Gpt_ChannelControlType Gpt_Channels[GPT_MAX_CHANNELS];
static const Gpt_ConfigType* Gpt_ConfigPtr = NULL;

/********************************************************************************************************/
/******************************************InternalFunctions*********************************************/
/********************************************************************************************************/

static uint64 Gpt_GetMonotonicNs(void)
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64)ts.tv_sec * 1000000000ULL) + (uint64)ts.tv_nsec;
}

/********************************************************************************************************/
/***************************************ForwardDeclarations**********************************************/
/********************************************************************************************************/

void Gpt_Init(const Gpt_ConfigType* ConfigPtr);
void Gpt_DeInit(void);
void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value);
void Gpt_StopTimer(Gpt_ChannelType Channel);
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel);
void Gpt_EnableNotification(Gpt_ChannelType Channel);
void Gpt_DisableNotification(Gpt_ChannelType Channel);
uint64 Gpt_GetTimestampUs(void);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
{
    uint8 ch;

    if (Gpt_Initialized != FALSE)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_INIT, GPT_E_ALREADY_INITIALIZED);
        return;
    }

    for (ch = 0U; ch < GPT_MAX_CHANNELS; ch++)
    {
        Gpt_Channels[ch].State = GPT_CH_STATE_STOPPED;
        Gpt_Channels[ch].TimerFd = -1;
        Gpt_Channels[ch].TargetTicks = 0U;
        Gpt_Channels[ch].StartTimeNs = 0U;
        Gpt_Channels[ch].NotificationEnabled = FALSE;
        Gpt_Channels[ch].Config = NULL;
    }

    Gpt_ConfigPtr = ConfigPtr;

    if (ConfigPtr != NULL)
    {
        uint8 numCh = ConfigPtr->NumChannels;
        if (numCh > GPT_MAX_CHANNELS)
        {
            numCh = GPT_MAX_CHANNELS;
        }

        for (ch = 0U; ch < numCh; ch++)
        {
            Gpt_Channels[ch].Config = &ConfigPtr->Channels[ch];
            Gpt_Channels[ch].TimerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        }
    }

    Gpt_Initialized = TRUE;
}

void Gpt_DeInit(void)
{
    uint8 ch;

    if (Gpt_Initialized == FALSE)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_DEINIT, GPT_E_UNINIT);
        return;
    }

    for (ch = 0U; ch < GPT_MAX_CHANNELS; ch++)
    {
        if (Gpt_Channels[ch].TimerFd >= 0)
        {
            (void)close(Gpt_Channels[ch].TimerFd);
            Gpt_Channels[ch].TimerFd = -1;
        }
        Gpt_Channels[ch].State = GPT_CH_STATE_STOPPED;
    }

    Gpt_ConfigPtr = NULL;
    Gpt_Initialized = FALSE;
}

void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)
{
    struct itimerspec its;
    uint32 tickNs;
    uint64 totalNs;

    if (Gpt_Initialized == FALSE)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_START_TIMER, GPT_E_UNINIT);
        return;
    }

    if (Channel >= GPT_MAX_CHANNELS)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_START_TIMER, GPT_E_PARAM_CHANNEL);
        return;
    }

    if (Gpt_Channels[Channel].State == GPT_CH_STATE_RUNNING)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_START_TIMER, GPT_E_BUSY);
        return;
    }

    /* Determine tick duration */
    tickNs = MCAL_GPT_TICK_NS;
    if ((Gpt_Channels[Channel].Config != NULL) && (Gpt_Channels[Channel].Config->TickDurationNs > 0U))
    {
        tickNs = Gpt_Channels[Channel].Config->TickDurationNs;
    }

    totalNs = (uint64)Value * (uint64)tickNs;

    (void)memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = (time_t)(totalNs / 1000000000ULL);
    its.it_value.tv_nsec = (long)(totalNs % 1000000000ULL);

    /* Set interval for continuous mode */
    if ((Gpt_Channels[Channel].Config != NULL) &&
        (Gpt_Channels[Channel].Config->Mode == GPT_CH_MODE_CONTINUOUS))
    {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }

    Gpt_Channels[Channel].TargetTicks = Value;
    Gpt_Channels[Channel].StartTimeNs = Gpt_GetMonotonicNs();
    Gpt_Channels[Channel].State = GPT_CH_STATE_RUNNING;

    if (Gpt_Channels[Channel].TimerFd >= 0)
    {
        (void)timerfd_settime(Gpt_Channels[Channel].TimerFd, 0, &its, NULL);
    }
}

void Gpt_StopTimer(Gpt_ChannelType Channel)
{
    struct itimerspec its;

    if (Gpt_Initialized == FALSE)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_STOP_TIMER, GPT_E_UNINIT);
        return;
    }

    if (Channel >= GPT_MAX_CHANNELS)
    {
        (void)Det_ReportError(GPT_MODULE_ID, MCAL_INSTANCE_ID, GPT_SID_STOP_TIMER, GPT_E_PARAM_CHANNEL);
        return;
    }

    /* Disarm the timer */
    (void)memset(&its, 0, sizeof(its));
    if (Gpt_Channels[Channel].TimerFd >= 0)
    {
        (void)timerfd_settime(Gpt_Channels[Channel].TimerFd, 0, &its, NULL);
    }

    Gpt_Channels[Channel].State = GPT_CH_STATE_STOPPED;
}

Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)
{
    uint64 nowNs;
    uint64 elapsedNs;
    uint32 tickNs;

    if ((Gpt_Initialized == FALSE) || (Channel >= GPT_MAX_CHANNELS))
    {
        return 0U;
    }

    if (Gpt_Channels[Channel].State != GPT_CH_STATE_RUNNING)
    {
        return 0U;
    }

    nowNs = Gpt_GetMonotonicNs();
    elapsedNs = nowNs - Gpt_Channels[Channel].StartTimeNs;

    tickNs = MCAL_GPT_TICK_NS;
    if ((Gpt_Channels[Channel].Config != NULL) && (Gpt_Channels[Channel].Config->TickDurationNs > 0U))
    {
        tickNs = Gpt_Channels[Channel].Config->TickDurationNs;
    }

    return (Gpt_ValueType)(elapsedNs / (uint64)tickNs);
}

void Gpt_EnableNotification(Gpt_ChannelType Channel)
{
    if ((Gpt_Initialized == FALSE) || (Channel >= GPT_MAX_CHANNELS))
    {
        return;
    }

    Gpt_Channels[Channel].NotificationEnabled = TRUE;
}

void Gpt_DisableNotification(Gpt_ChannelType Channel)
{
    if ((Gpt_Initialized == FALSE) || (Channel >= GPT_MAX_CHANNELS))
    {
        return;
    }

    Gpt_Channels[Channel].NotificationEnabled = FALSE;
}

uint64 Gpt_GetTimestampUs(void)
{
    return Gpt_GetMonotonicNs() / 1000ULL;
}
