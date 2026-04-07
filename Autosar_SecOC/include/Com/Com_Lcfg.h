#ifndef INCLUDE_COM_LCFG_H_
#define INCLUDE_COM_LCFG_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Com_PBcfg.h"

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

struct Com_ConfigTypeTag
{
    uint16 ComNumOfSignals;
    uint16 ComNumOfSignalGroups;
    uint16 ComNumOfTxIpdu;
    uint16 ComNumOfRxIpdu;
    const Com_SignalConfigType* ComSignalConfigPtr;
    const Com_SignalGroupConfigType* ComSignalGroupConfigPtr;
    const Com_TxIpduConfigType* ComTxIpduConfigPtr;
    const Com_RxIpduConfigType* ComRxIpduConfigPtr;
};

typedef struct Com_ConfigTypeTag Com_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

// cppcheck-suppress misra-c2012-8.4
extern const Com_ConfigType Com_Config;

#endif
