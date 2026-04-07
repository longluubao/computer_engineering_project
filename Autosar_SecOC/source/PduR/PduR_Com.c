/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "PduR_Com.h"
#include "SecOC.h"
#include "SecOC_Debug.h"


/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern Std_ReturnType PduR_ComTransmit(PduIdType PduID, const PduInfoType *PduInfo);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/****************************************************
 *          * Function Info *                       *
 *                                                  *
 * Function_Name        : CanIf_Transmit            *
 * Function_Index       : 8.3.6 [SWS_CANIF_00005]   *
 * Function_File        : SWS of COM                *
 * Function_Descripton  : Transmits message to PDuR *
 *                                                  *
 ***************************************************/
Std_ReturnType PduR_ComTransmit(PduIdType PduID, const PduInfoType *PduInfo)
{
    #ifdef PDUR_DEBUG
        printf("######## in PduR_ComTransmit \n");
    #endif
    if ((PduInfo == NULL) || (PduID >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING))
    {
        return E_NOT_OK;
    }

    /* When SecOC is used, PduR routes the message to the SecOC */
    return SecOC_IfTransmit(PduID, PduInfo);
}