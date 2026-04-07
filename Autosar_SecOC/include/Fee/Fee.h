#ifndef INCLUDE_FEE_H_
#define INCLUDE_FEE_H_

#include "Std_Types.h"
#include "MemIf/MemIf.h"

#define FEE_MODULE_ID                ((uint16)21U)
#define FEE_INSTANCE_ID              ((uint8)0U)

#define FEE_SID_INIT                 ((uint8)0x00U)
#define FEE_SID_READ                 ((uint8)0x01U)
#define FEE_SID_WRITE                ((uint8)0x02U)
#define FEE_SID_CANCEL               ((uint8)0x03U)
#define FEE_SID_GET_STATUS           ((uint8)0x04U)
#define FEE_SID_GET_JOB_RESULT       ((uint8)0x05U)
#define FEE_SID_INVALIDATE_BLOCK     ((uint8)0x06U)
#define FEE_SID_ERASE_BLOCK          ((uint8)0x07U)
#define FEE_SID_MAIN_FUNCTION        ((uint8)0x08U)

#define FEE_E_UNINIT                 ((uint8)0x01U)
#define FEE_E_PARAM_POINTER          ((uint8)0x02U)
#define FEE_E_PARAM_BLOCK_NUMBER     ((uint8)0x03U)
#define FEE_E_BUSY                   ((uint8)0x04U)

#define FEE_MAX_BLOCKS               ((uint16)32U)
#define FEE_MAX_BLOCK_LENGTH         ((uint16)512U)

typedef void (*Fee_JobNotificationType)(void);

void Fee_Init(void);
void Fee_MainFunction(void);

Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber);
Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber);

void Fee_SetMode(MemIf_ModeType Mode);
MemIf_StatusType Fee_GetStatus(void);
MemIf_JobResultType Fee_GetJobResult(void);
void Fee_SetJobEndNotification(Fee_JobNotificationType NotificationPtr);
void Fee_SetJobErrorNotification(Fee_JobNotificationType NotificationPtr);

#endif /* INCLUDE_FEE_H_ */
