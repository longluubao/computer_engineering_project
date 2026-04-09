/**
 * @file Dio.h
 * @brief AUTOSAR MCAL Digital I/O (GPIO) Driver API
 * @details Provides GPIO read/write for status LEDs, CAN transceiver
 *          control pins, and other digital signals.
 *          On Pi 4: uses /sys/class/gpio sysfs interface.
 *
 *          Note: For HPC gateway role, GPIO usage is minimal
 *          (status LEDs, transceiver standby pins). Heavy GPIO
 *          manipulation is not expected at the central node.
 */

#ifndef DIO_H
#define DIO_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Mcal_Cfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Service IDs */
#define DIO_SID_INIT                    ((uint8)0x10U)
#define DIO_SID_READ_CHANNEL            ((uint8)0x00U)
#define DIO_SID_WRITE_CHANNEL           ((uint8)0x01U)
#define DIO_SID_FLIP_CHANNEL            ((uint8)0x11U)

/* DET Error Codes */
#define DIO_E_UNINIT                    ((uint8)0x01U)
#define DIO_E_PARAM_INVALID_CHANNEL_ID  ((uint8)0x0AU)
#define DIO_E_PARAM_INVALID_GROUP_ID    ((uint8)0x1FU)

/* GPIO directions */
#define DIO_DIRECTION_INPUT             ((uint8)0U)
#define DIO_DIRECTION_OUTPUT            ((uint8)1U)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

/** @brief DIO channel (GPIO pin number) */
typedef uint8 Dio_ChannelType;

/** @brief DIO level */
typedef uint8 Dio_LevelType;

#define DIO_LEVEL_LOW                   ((Dio_LevelType)0U)
#define DIO_LEVEL_HIGH                  ((Dio_LevelType)1U)

/** @brief DIO channel configuration */
typedef struct
{
    Dio_ChannelType ChannelId;   /**< GPIO pin number */
    uint8 Direction;             /**< DIO_DIRECTION_INPUT or DIO_DIRECTION_OUTPUT */
    Dio_LevelType InitialLevel;  /**< Initial output level (ignored for inputs) */
} Dio_ChannelConfigType;

/** @brief DIO module configuration */
typedef struct
{
    const Dio_ChannelConfigType* Channels;
    uint8 NumChannels;
} Dio_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize DIO channels (export and configure direction)
 * @param ConfigPtr Configuration pointer
 */
void Dio_Init(const Dio_ConfigType* ConfigPtr);

/**
 * @brief De-initialize DIO channels (unexport)
 */
void Dio_DeInit(void);

/**
 * @brief Read a DIO channel
 * @param ChannelId GPIO pin number
 * @return DIO_LEVEL_LOW or DIO_LEVEL_HIGH
 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);

/**
 * @brief Write a DIO channel
 * @param ChannelId GPIO pin number
 * @param Level DIO_LEVEL_LOW or DIO_LEVEL_HIGH
 */
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);

/**
 * @brief Flip (toggle) a DIO channel
 * @param ChannelId GPIO pin number
 * @return New level after flip
 */
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);

#endif /* DIO_H */
