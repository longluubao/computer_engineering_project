#ifndef INCLUDE_CANNM_H_
#define INCLUDE_CANNM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define CANNM_MODULE_ID                       ((uint16)31)
#define CANNM_INSTANCE_ID                     ((uint8)0)

#define CANNM_SID_INIT                        ((uint8)0x00)
#define CANNM_SID_DEINIT                      ((uint8)0x10)
#define CANNM_SID_NETWORK_REQUEST             ((uint8)0x01)
#define CANNM_SID_NETWORK_RELEASE             ((uint8)0x02)
#define CANNM_SID_GET_STATE                   ((uint8)0x03)
#define CANNM_SID_MAINFUNCTION                ((uint8)0x13)
#define CANNM_SID_TX_CONFIRMATION             ((uint8)0x0F)
#define CANNM_SID_RX_INDICATION               ((uint8)0x11)

#define CANNM_E_UNINIT                        ((uint8)0x01)
#define CANNM_E_PARAM_POINTER                 ((uint8)0x02)
#define CANNM_E_PARAM_NETWORK                 ((uint8)0x03)

/* NM timing configuration (in MainFunction cycles) */
#define CANNM_TIMEOUT_TIME                    ((uint16)500)
#define CANNM_REPEAT_MESSAGE_TIME             ((uint16)100)
#define CANNM_WAIT_BUS_SLEEP_TIME             ((uint16)300)
#define CANNM_MSG_CYCLE_TIME                  ((uint16)50)

/* NM PDU configuration */
#define CANNM_PDU_LENGTH                      ((uint8)8)
#define CANNM_NM_PDU_NID_POSITION             ((uint8)0)
#define CANNM_NM_PDU_CBV_POSITION             ((uint8)1)

/* Control Bit Vector flags */
#define CANNM_CBV_REPEAT_MSG_REQUEST          ((uint8)0x01)
#define CANNM_CBV_NM_COORDINATOR_SLEEP        ((uint8)0x08)
#define CANNM_CBV_ACTIVE_WAKEUP              ((uint8)0x10)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    CANNM_UNINIT = 0,
    CANNM_INIT
} CanNm_InitStateType;

/* NM states per AUTOSAR CanNm SWS */
typedef enum
{
    CANNM_STATE_BUS_SLEEP = 0,
    CANNM_STATE_PREPARE_BUS_SLEEP,
    CANNM_STATE_REPEAT_MESSAGE,
    CANNM_STATE_NORMAL_OPERATION,
    CANNM_STATE_READY_SLEEP
} CanNm_NmStateType;

/* High-level NM mode */
typedef enum
{
    CANNM_MODE_BUS_SLEEP = 0,
    CANNM_MODE_PREPARE_BUS_SLEEP,
    CANNM_MODE_NETWORK
} CanNm_ModeType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void CanNm_Init(void);
void CanNm_DeInit(void);
Std_ReturnType CanNm_NetworkRequest(uint8 NetworkHandle);
Std_ReturnType CanNm_NetworkRelease(uint8 NetworkHandle);
Std_ReturnType CanNm_GetState(uint8 NetworkHandle, CanNm_NmStateType *NmStatePtr);
void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void CanNm_MainFunction(void);

#endif /* INCLUDE_CANNM_H_ */
