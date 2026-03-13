#ifndef INCLUDE_COM_PBCFG_H_
#define INCLUDE_COM_PBCFG_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"

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

extern const Com_SignalConfigType Com_PbSignalConfig[];
extern const Com_SignalGroupConfigType Com_PbSignalGroupConfig[];
extern const Com_TxIpduConfigType Com_PbTxIpduConfig[];
extern const Com_RxIpduConfigType Com_PbRxIpduConfig[];

#endif
