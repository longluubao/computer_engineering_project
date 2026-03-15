#ifndef INCLUDE_MEMIF_H_
#define INCLUDE_MEMIF_H_

#include "Std_Types.h"

#define MEMIF_MODULE_ID                 ((uint16)22U)
#define MEMIF_INSTANCE_ID               ((uint8)0U)

#define MEMIF_SID_INIT                  ((uint8)0x00U)
#define MEMIF_SID_SET_MODE              ((uint8)0x01U)
#define MEMIF_SID_READ                  ((uint8)0x02U)
#define MEMIF_SID_WRITE                 ((uint8)0x03U)
#define MEMIF_SID_INVALIDATE_BLOCK      ((uint8)0x04U)
#define MEMIF_SID_ERASE_IMMEDIATE_BLOCK ((uint8)0x05U)
#define MEMIF_SID_GET_STATUS            ((uint8)0x06U)
#define MEMIF_SID_GET_JOB_RESULT        ((uint8)0x07U)
#define MEMIF_SID_MAIN_FUNCTION         ((uint8)0x08U)

#define MEMIF_E_PARAM_DEVICE            ((uint8)0x01U)
#define MEMIF_E_UNINIT                  ((uint8)0x02U)

#define MEMIF_NUMBER_OF_DEVICES         ((uint8)2U)
#define MEMIF_FEE_DEVICE_INDEX          ((uint8)0U)
#define MEMIF_EA_DEVICE_INDEX           ((uint8)1U)

typedef enum
{
    MEMIF_MODE_SLOW = 0,
    MEMIF_MODE_FAST
} MemIf_ModeType;

typedef enum
{
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY,
    MEMIF_BUSY_INTERNAL
} MemIf_StatusType;

typedef enum
{
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_FAILED,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED,
    MEMIF_BLOCK_INCONSISTENT,
    MEMIF_BLOCK_INVALID
} MemIf_JobResultType;

typedef void (*MemIf_JobNotificationType)(void);

void MemIf_Init(void);
void MemIf_MainFunction(void);

Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length);
Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber);
Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex, uint16 BlockNumber);

void MemIf_SetMode(MemIf_ModeType Mode);
MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex);
MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex);
Std_ReturnType MemIf_SetJobNotifications(
    uint8 DeviceIndex,
    MemIf_JobNotificationType JobEndNotificationPtr,
    MemIf_JobNotificationType JobErrorNotificationPtr
);

#endif /* INCLUDE_MEMIF_H_ */
