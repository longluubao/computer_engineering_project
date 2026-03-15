#ifndef INCLUDE_EA_H_
#define INCLUDE_EA_H_

#include "Std_Types.h"
#include "MemIf.h"

#define EA_MODULE_ID                 ((uint16)40U)
#define EA_INSTANCE_ID               ((uint8)0U)

#define EA_SID_INIT                  ((uint8)0x00U)
#define EA_SID_READ                  ((uint8)0x01U)
#define EA_SID_WRITE                 ((uint8)0x02U)
#define EA_SID_CANCEL                ((uint8)0x03U)
#define EA_SID_GET_STATUS            ((uint8)0x04U)
#define EA_SID_GET_JOB_RESULT        ((uint8)0x05U)
#define EA_SID_INVALIDATE_BLOCK      ((uint8)0x06U)
#define EA_SID_ERASE_BLOCK           ((uint8)0x07U)
#define EA_SID_MAIN_FUNCTION         ((uint8)0x08U)

#define EA_E_UNINIT                  ((uint8)0x01U)
#define EA_E_PARAM_POINTER           ((uint8)0x02U)
#define EA_E_PARAM_BLOCK_NUMBER      ((uint8)0x03U)
#define EA_E_BUSY                    ((uint8)0x04U)

#define EA_MAX_BLOCKS                ((uint16)32U)
#define EA_MAX_BLOCK_LENGTH          ((uint16)256U)

typedef void (*Ea_JobNotificationType)(void);

void Ea_Init(void);
void Ea_MainFunction(void);

Std_ReturnType Ea_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Ea_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Ea_InvalidateBlock(uint16 BlockNumber);
Std_ReturnType Ea_EraseImmediateBlock(uint16 BlockNumber);

void Ea_SetMode(MemIf_ModeType Mode);
MemIf_StatusType Ea_GetStatus(void);
MemIf_JobResultType Ea_GetJobResult(void);
void Ea_SetJobEndNotification(Ea_JobNotificationType NotificationPtr);
void Ea_SetJobErrorNotification(Ea_JobNotificationType NotificationPtr);

#endif /* INCLUDE_EA_H_ */
