/**
 * @file Dio_Pi4.c
 * @brief DIO (GPIO) Driver Implementation for Raspberry Pi 4
 * @details Uses Linux sysfs GPIO interface (/sys/class/gpio).
 *          For HPC gateway role, GPIO is minimal — status LEDs,
 *          CAN transceiver standby pins, etc.
 *
 *          Note: sysfs GPIO is deprecated in favour of libgpiod / chardev,
 *          but sysfs is universally available on all kernel versions
 *          and requires no external library.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Mcal/Dio.h"
#include "Det/Det.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Dio_Initialized = FALSE;
static const Dio_ConfigType* Dio_ConfigPtr = NULL;

/********************************************************************************************************/
/******************************************InternalFunctions*********************************************/
/********************************************************************************************************/

static Std_ReturnType Dio_SysfsWrite(const char* path, const char* value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        return E_NOT_OK;
    }

    (void)write(fd, value, strlen(value));
    (void)close(fd);
    return E_OK;
}

static Std_ReturnType Dio_ExportPin(Dio_ChannelType pin)
{
    char buf[8];
    (void)snprintf(buf, sizeof(buf), "%u", (unsigned int)pin);
    return Dio_SysfsWrite("/sys/class/gpio/export", buf);
}

static Std_ReturnType Dio_UnexportPin(Dio_ChannelType pin)
{
    char buf[8];
    (void)snprintf(buf, sizeof(buf), "%u", (unsigned int)pin);
    return Dio_SysfsWrite("/sys/class/gpio/unexport", buf);
}

static Std_ReturnType Dio_SetDirection(Dio_ChannelType pin, uint8 direction)
{
    char path[64];
    (void)snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction",
                   (unsigned int)pin);

    if (direction == DIO_DIRECTION_OUTPUT)
    {
        return Dio_SysfsWrite(path, "out");
    }
    else
    {
        return Dio_SysfsWrite(path, "in");
    }
}

static boolean Dio_IsChannelConfigured(Dio_ChannelType ChannelId)
{
    uint8 i;

    if (Dio_ConfigPtr == NULL)
    {
        return FALSE;
    }

    for (i = 0U; i < Dio_ConfigPtr->NumChannels; i++)
    {
        if (Dio_ConfigPtr->Channels[i].ChannelId == ChannelId)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/********************************************************************************************************/
/***************************************ForwardDeclarations**********************************************/
/********************************************************************************************************/

void Dio_Init(const Dio_ConfigType* ConfigPtr);
void Dio_DeInit(void);
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Dio_Init(const Dio_ConfigType* ConfigPtr)
{
    uint8 i;

    if (Dio_Initialized != FALSE)
    {
        return;
    }

    Dio_ConfigPtr = ConfigPtr;

    if (ConfigPtr != NULL)
    {
        for (i = 0U; i < ConfigPtr->NumChannels; i++)
        {
            const Dio_ChannelConfigType* chCfg = &ConfigPtr->Channels[i];

            /* Export the pin */
            if (Dio_ExportPin(chCfg->ChannelId) != E_OK)
            {
                /* Pin may already be exported; continue */
            }

            /* Small delay for sysfs to create the gpio directory */
            (void)usleep(50000U);

            /* Set direction */
            (void)Dio_SetDirection(chCfg->ChannelId, chCfg->Direction);

            /* Set initial output level */
            if (chCfg->Direction == DIO_DIRECTION_OUTPUT)
            {
                Dio_WriteChannel(chCfg->ChannelId, chCfg->InitialLevel);
            }
        }
    }

    Dio_Initialized = TRUE;
}

void Dio_DeInit(void)
{
    uint8 i;

    if (Dio_Initialized == FALSE)
    {
        return;
    }

    if (Dio_ConfigPtr != NULL)
    {
        for (i = 0U; i < Dio_ConfigPtr->NumChannels; i++)
        {
            (void)Dio_UnexportPin(Dio_ConfigPtr->Channels[i].ChannelId);
        }
    }

    Dio_ConfigPtr = NULL;
    Dio_Initialized = FALSE;
}

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    char path[64];
    char valueBuf[4];
    int fd;
    ssize_t bytesRead;

    if (Dio_Initialized == FALSE)
    {
        (void)Det_ReportError(DIO_MODULE_ID, MCAL_INSTANCE_ID, DIO_SID_READ_CHANNEL, DIO_E_UNINIT);
        return DIO_LEVEL_LOW;
    }

    if (Dio_IsChannelConfigured(ChannelId) == FALSE)
    {
        (void)Det_ReportError(DIO_MODULE_ID, MCAL_INSTANCE_ID, DIO_SID_READ_CHANNEL, DIO_E_PARAM_INVALID_CHANNEL_ID);
        return DIO_LEVEL_LOW;
    }

    (void)snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value",
                   (unsigned int)ChannelId);

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return DIO_LEVEL_LOW;
    }

    (void)memset(valueBuf, 0, sizeof(valueBuf));
    bytesRead = read(fd, valueBuf, sizeof(valueBuf) - 1U);
    (void)close(fd);

    if ((bytesRead > 0) && (valueBuf[0] == '1'))
    {
        return DIO_LEVEL_HIGH;
    }

    return DIO_LEVEL_LOW;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    char path[64];

    if (Dio_Initialized == FALSE)
    {
        (void)Det_ReportError(DIO_MODULE_ID, MCAL_INSTANCE_ID, DIO_SID_WRITE_CHANNEL, DIO_E_UNINIT);
        return;
    }

    if (Dio_IsChannelConfigured(ChannelId) == FALSE)
    {
        (void)Det_ReportError(DIO_MODULE_ID, MCAL_INSTANCE_ID, DIO_SID_WRITE_CHANNEL, DIO_E_PARAM_INVALID_CHANNEL_ID);
        return;
    }

    (void)snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value",
                   (unsigned int)ChannelId);

    if (Level == DIO_LEVEL_HIGH)
    {
        (void)Dio_SysfsWrite(path, "1");
    }
    else
    {
        (void)Dio_SysfsWrite(path, "0");
    }
}

Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType currentLevel;

    if (Dio_Initialized == FALSE)
    {
        (void)Det_ReportError(DIO_MODULE_ID, MCAL_INSTANCE_ID, DIO_SID_FLIP_CHANNEL, DIO_E_UNINIT);
        return DIO_LEVEL_LOW;
    }

    currentLevel = Dio_ReadChannel(ChannelId);

    if (currentLevel == DIO_LEVEL_HIGH)
    {
        Dio_WriteChannel(ChannelId, DIO_LEVEL_LOW);
        return DIO_LEVEL_LOW;
    }
    else
    {
        Dio_WriteChannel(ChannelId, DIO_LEVEL_HIGH);
        return DIO_LEVEL_HIGH;
    }
}
