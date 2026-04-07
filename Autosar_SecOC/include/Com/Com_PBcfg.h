#ifndef INCLUDE_COM_PBCFG_H_
#define INCLUDE_COM_PBCFG_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"
#include "SecOC/SecOC_Cfg.h"

/* Array size macros for Com post-build configuration */
#define COM_PB_NUM_OF_SIGNALS              ((uint16)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_PB_NUM_OF_SIGNAL_GROUPS        ((uint16)2U)
#define COM_PB_NUM_OF_TX_IPDU              ((PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_PB_NUM_OF_RX_IPDU              ((PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef struct
{
    PduIdType ComTxPduId;
    PduIdType ComRxPduId;
    uint16 ComSignalPosition;
    uint16 ComSignalLength;
    uint16 ComSignalMaxLength;
    uint16 ComIpduGroupId;
    uint8 ComInvalidValue;
    uint8 ComSignalFilterMask;
} Com_SignalConfigType;

typedef struct
{
    uint16 ComFirstSignalId;
    uint16 ComSignalCount;
    uint16 ComIpduGroupId;
    uint16 ComSignalGroupArrayLength;
} Com_SignalGroupConfigType;

typedef struct
{
    uint16 ComIpduGroupId;
    uint16 ComTxDeadlineLimit;
} Com_TxIpduConfigType;

typedef struct
{
    uint16 ComIpduGroupId;
    uint16 ComRxDeadlineLimit;
} Com_RxIpduConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

extern const Com_SignalConfigType Com_PbSignalConfig[COM_PB_NUM_OF_SIGNALS];
extern const Com_SignalGroupConfigType Com_PbSignalGroupConfig[COM_PB_NUM_OF_SIGNAL_GROUPS];
extern const Com_TxIpduConfigType Com_PbTxIpduConfig[COM_PB_NUM_OF_TX_IPDU];
extern const Com_RxIpduConfigType Com_PbRxIpduConfig[COM_PB_NUM_OF_RX_IPDU];

#endif
