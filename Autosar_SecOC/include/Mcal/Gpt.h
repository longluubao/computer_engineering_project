/**
 * @file Gpt.h
 * @brief AUTOSAR MCAL General Purpose Timer Driver API
 * @details Provides hardware timer abstraction for OS tick generation
 *          and elapsed-time measurement.
 *          On Pi 4: uses Linux timerfd for precise periodic ticks
 *          and CLOCK_MONOTONIC for timestamp queries.
 */

#ifndef GPT_H
#define GPT_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Mcal_Cfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Service IDs */
#define GPT_SID_INIT                    ((uint8)0x01U)
#define GPT_SID_DEINIT                  ((uint8)0x02U)
#define GPT_SID_START_TIMER             ((uint8)0x05U)
#define GPT_SID_STOP_TIMER              ((uint8)0x06U)
#define GPT_SID_GET_TIME_ELAPSED        ((uint8)0x03U)
#define GPT_SID_ENABLE_NOTIFICATION     ((uint8)0x07U)
#define GPT_SID_DISABLE_NOTIFICATION    ((uint8)0x08U)

/* DET Error Codes */
#define GPT_E_UNINIT                    ((uint8)0x0AU)
#define GPT_E_PARAM_CHANNEL             ((uint8)0x14U)
#define GPT_E_PARAM_VALUE               ((uint8)0x15U)
#define GPT_E_ALREADY_INITIALIZED       ((uint8)0x0DU)
#define GPT_E_BUSY                      ((uint8)0x0BU)

/* Maximum number of GPT channels */
#define GPT_MAX_CHANNELS                ((uint8)4U)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

/** @brief GPT channel identifier */
typedef uint8 Gpt_ChannelType;

/** @brief GPT tick value (nanosecond-based internally) */
typedef uint32 Gpt_ValueType;

/** @brief GPT channel mode */
typedef enum
{
    GPT_CH_MODE_ONESHOT = 0U,
    GPT_CH_MODE_CONTINUOUS
} Gpt_ChannelModeType;

/** @brief Notification callback type */
typedef void (*Gpt_NotificationType)(void);

/** @brief Per-channel configuration */
typedef struct
{
    Gpt_ChannelModeType Mode;           /**< One-shot or continuous */
    Gpt_NotificationType Notification;  /**< Expiry callback (may be NULL) */
    uint32 TickDurationNs;              /**< Tick duration in nanoseconds */
} Gpt_ChannelConfigType;

/** @brief GPT module configuration */
typedef struct
{
    const Gpt_ChannelConfigType* Channels;
    uint8 NumChannels;
} Gpt_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Gpt_Init(const Gpt_ConfigType* ConfigPtr);
void Gpt_DeInit(void);

void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value);
void Gpt_StopTimer(Gpt_ChannelType Channel);

Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel);

void Gpt_EnableNotification(Gpt_ChannelType Channel);
void Gpt_DisableNotification(Gpt_ChannelType Channel);

/**
 * @brief Get a monotonic timestamp in microseconds
 * @return Microseconds since system boot
 */
uint64 Gpt_GetTimestampUs(void);

#endif /* GPT_H */
