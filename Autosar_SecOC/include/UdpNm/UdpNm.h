#ifndef INCLUDE_UDPNM_H_
#define INCLUDE_UDPNM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Com/ComStack_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Module identification (AUTOSAR SWS_UDPNetworkManagement module ID = 33) */
#define UDPNM_MODULE_ID                        ((uint16)33)
#define UDPNM_INSTANCE_ID                      ((uint8)0)

/*
 * Service IDs for DET reporting
 * Per AUTOSAR SWS_UDPNetworkManagement (R21-11, Table 8-1)
 */
#define UDPNM_SID_INIT                         ((uint8)0x01)
#define UDPNM_SID_PASSIVE_STARTUP              ((uint8)0x0E)
#define UDPNM_SID_NETWORK_REQUEST              ((uint8)0x02)
#define UDPNM_SID_NETWORK_RELEASE              ((uint8)0x03)
#define UDPNM_SID_SET_USER_DATA                ((uint8)0x04)
#define UDPNM_SID_GET_USER_DATA                ((uint8)0x05)
#define UDPNM_SID_GET_NODE_IDENTIFIER          ((uint8)0x06)
#define UDPNM_SID_GET_LOCAL_NODE_IDENTIFIER    ((uint8)0x07)
#define UDPNM_SID_REPEAT_MESSAGE_REQUEST       ((uint8)0x08)
#define UDPNM_SID_GET_PDU_DATA                 ((uint8)0x0A)
#define UDPNM_SID_GET_STATE                    ((uint8)0x0B)
#define UDPNM_SID_DEINIT                       ((uint8)0x10)
#define UDPNM_SID_TX_CONFIRMATION              ((uint8)0x0F)
#define UDPNM_SID_RX_INDICATION                ((uint8)0x10)  /* SoAdIfRxIndication */
#define UDPNM_SID_TRANSMIT                     ((uint8)0x15)
#define UDPNM_SID_MAINFUNCTION                 ((uint8)0x13)

/* DET error codes */
#define UDPNM_E_UNINIT                         ((uint8)0x01)
#define UDPNM_E_PARAM_POINTER                  ((uint8)0x02)
#define UDPNM_E_PARAM_NETWORK                  ((uint8)0x03)
#define UDPNM_E_INIT_FAILED                    ((uint8)0x04)
#define UDPNM_E_NODE_DETECTION_DISABLED        ((uint8)0x05)

/*
 * NM timing configuration (in MainFunction cycles, assuming 1 ms per cycle)
 * UdpNmTimeoutTime         > UdpNmMsgCycleTime  (own TX keeps network alive)
 * UdpNmRepeatMessageTime   = time to stay in Repeat Message State
 * UdpNmWaitBusSleepTime    = calm-down time in Prepare-Bus-Sleep
 * UdpNmMsgCycleTime        = periodic TX interval in Network Mode
 * UDPNM_IMMEDIATE_NM_TX    = number of fast TXs on Network Mode entry (0 = disabled)
 */
#define UDPNM_TIMEOUT_TIME                     ((uint16)300)
#define UDPNM_REPEAT_MESSAGE_TIME              ((uint16)100)
#define UDPNM_WAIT_BUS_SLEEP_TIME              ((uint16)200)
#define UDPNM_MSG_CYCLE_TIME                   ((uint16)20)
#define UDPNM_IMMEDIATE_NM_CYCLE_TIME          ((uint16)5)
#define UDPNM_IMMEDIATE_NM_TRANSMISSIONS       ((uint8)3)

/* NM PDU configuration */
#define UDPNM_PDU_LENGTH                       ((uint8)12)
#define UDPNM_TX_PDU_ID                        ((PduIdType)2U)
#define UDPNM_RX_PDU_ID                        ((PduIdType)2U)

/*
 * PDU byte positions (UdpNmPduNidPosition / UdpNmPduCbvPosition)
 * Standard layout: Byte 0 = NID, Byte 1 = CBV, Byte 2..11 = User Data
 */
#define UDPNM_PDU_NID_POSITION                 ((uint8)0)
#define UDPNM_PDU_CBV_POSITION                 ((uint8)1)
#define UDPNM_PDU_USERDATA_OFFSET              ((uint8)2)
#define UDPNM_USER_DATA_LENGTH                 ((uint8)10)

/* Local node identifier */
#define UDPNM_NODE_ID                          ((uint8)0x01)

/*
 * Control Bit Vector (CBV) bit masks
 * Per AUTOSAR PRS_NetworkManagementProtocol (bit positions 0..7, LSB = bit 0)
 *   Bit 0 (0x01): Repeat Message Request (RMR)
 *   Bit 1 (0x02): Reserved
 *   Bit 2 (0x04): Reserved
 *   Bit 3 (0x08): NM Coordinator Sleep Bit (CSB)
 *   Bit 4 (0x10): Active Wakeup Bit (AWB)
 *   Bit 5 (0x20): Reserved
 *   Bit 6 (0x40): Partial Network Information Bit (PNI)
 *   Bit 7 (0x80): Reserved
 */
#define UDPNM_CBV_REPEAT_MSG_REQUEST           ((uint8)0x01)
#define UDPNM_CBV_NM_COORDINATOR_SLEEP         ((uint8)0x08)
#define UDPNM_CBV_ACTIVE_WAKEUP                ((uint8)0x10)
#define UDPNM_CBV_PARTIAL_NETWORK_INFO         ((uint8)0x40)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    UDPNM_UNINIT = 0,
    UDPNM_INIT
} UdpNm_InitStateType;

/* NM states per AUTOSAR UdpNm SWS */
typedef enum
{
    UDPNM_STATE_BUS_SLEEP = 0,
    UDPNM_STATE_PREPARE_BUS_SLEEP,
    UDPNM_STATE_REPEAT_MESSAGE,
    UDPNM_STATE_NORMAL_OPERATION,
    UDPNM_STATE_READY_SLEEP
} UdpNm_NmStateType;

/* High-level NM mode */
typedef enum
{
    UDPNM_MODE_BUS_SLEEP = 0,
    UDPNM_MODE_PREPARE_BUS_SLEEP,
    UDPNM_MODE_NETWORK
} UdpNm_ModeType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void             UdpNm_Init(void);
void             UdpNm_DeInit(void);

Std_ReturnType   UdpNm_PassiveStartUp(uint8 NetworkHandle);
Std_ReturnType   UdpNm_NetworkRequest(uint8 NetworkHandle);
Std_ReturnType   UdpNm_NetworkRelease(uint8 NetworkHandle);

Std_ReturnType   UdpNm_SetUserData(uint8 NetworkHandle, const uint8 *NmUserDataPtr);
Std_ReturnType   UdpNm_GetUserData(uint8 NetworkHandle, uint8 *NmUserDataPtr);
Std_ReturnType   UdpNm_GetPduData(uint8 NetworkHandle, uint8 *NmPduDataPtr);

Std_ReturnType   UdpNm_GetNodeIdentifier(uint8 NetworkHandle, uint8 *NmNodeIdPtr);
Std_ReturnType   UdpNm_GetLocalNodeIdentifier(uint8 NetworkHandle, uint8 *NmNodeIdPtr);

Std_ReturnType   UdpNm_GetState(uint8 NetworkHandle, UdpNm_NmStateType *NmStatePtr,
                                 UdpNm_ModeType *NmModePtr);

Std_ReturnType   UdpNm_RepeatMessageRequest(uint8 NetworkHandle);

Std_ReturnType   UdpNm_Transmit(PduIdType UdpNmSrcPduId,
                                 const PduInfoType *UdpNmSrcPduInfoPtr);

void             UdpNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void             UdpNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

void             UdpNm_MainFunction(void);

#endif /* INCLUDE_UDPNM_H_ */
