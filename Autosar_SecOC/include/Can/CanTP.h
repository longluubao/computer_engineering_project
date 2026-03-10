#ifndef INCLUDE_CANT_TP_H_
#define INCLUDE_CANT_TP_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"


/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define SUCCESS     (1u)
#define FAILED      (0u)
#define TP_TRANSMISSION_STATUS SUCCESS

#define CANTP_BUFFER_SIZE       255
#define BUS_LENGTH              8

#define CANTP_MODULE_ID                     ((uint16)35)
#define CANTP_INSTANCE_ID                   ((uint8)0)

/* Service IDs */
#define CANTP_SID_INIT                      ((uint8)0x01)
#define CANTP_SID_SHUTDOWN                  ((uint8)0x02)
#define CANTP_SID_TRANSMIT                  ((uint8)0x03)
#define CANTP_SID_RX_INDICATION             ((uint8)0x04)
#define CANTP_SID_TX_CONFIRMATION           ((uint8)0x05)
#define CANTP_SID_MAINFUNCTION_TX           ((uint8)0x06)
#define CANTP_SID_MAINFUNCTION_RX           ((uint8)0x07)

/* DET Error Codes */
#define CANTP_E_PARAM_ID                    ((uint8)0x02)
#define CANTP_E_PARAM_POINTER               ((uint8)0x03)
#define CANTP_E_UNINIT                      ((uint8)0x20)

/* ISO 15765-2 N-PDU frame types */
#define CANTP_NPDU_SF                       ((uint8)0x00)  /* Single Frame */
#define CANTP_NPDU_FF                       ((uint8)0x10)  /* First Frame */
#define CANTP_NPDU_CF                       ((uint8)0x20)  /* Consecutive Frame */
#define CANTP_NPDU_FC                       ((uint8)0x30)  /* Flow Control */

/* Flow Control Status */
#define CANTP_FC_CTS                        ((uint8)0x00)  /* Continue To Send */
#define CANTP_FC_WAIT                       ((uint8)0x01)  /* Wait */
#define CANTP_FC_OVFLW                      ((uint8)0x02)  /* Overflow */

/* Default timeout values (in MainFunction cycles) */
#define CANTP_N_AS_TIMEOUT                  ((uint16)100)
#define CANTP_N_BS_TIMEOUT                  ((uint16)200)
#define CANTP_N_CS_TIMEOUT                  ((uint16)100)
#define CANTP_N_AR_TIMEOUT                  ((uint16)100)
#define CANTP_N_BR_TIMEOUT                  ((uint16)200)
#define CANTP_N_CR_TIMEOUT                  ((uint16)200)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    CANTP_STATE_IDLE = 0,
    CANTP_STATE_TX_SF,
    CANTP_STATE_TX_FF,
    CANTP_STATE_TX_CF_WAIT_FC,
    CANTP_STATE_TX_CF,
    CANTP_STATE_RX_FF_RECEIVED,
    CANTP_STATE_RX_CF
} CanTp_ChannelStateType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void CanTp_Init(void);
void CanTp_Shutdown(void);

Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType *CanTpTxInfoPtr);

void CanTp_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

void CanTp_MainFunctionTx(void);
void CanTp_MainFunctionRx(void);

#endif /*INCLUDE_CANTP_H_*/
