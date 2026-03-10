/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "BswM.h"
#include "Det.h"
#include "ComM.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static BswM_StateType BswM_State = BSWM_UNINIT;
static BswM_ModeRequestType BswM_ModeRequests[BSWM_MAX_MODE_REQUESTS];
static uint8 BswM_PendingRequestCount = 0U;

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void BswM_Init(const BswM_ConfigType *ConfigPtr)
{
    uint8 idx;

    (void)ConfigPtr;

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        BswM_ModeRequests[idx].RequesterId = 0U;
        BswM_ModeRequests[idx].RequestedMode = 0U;
        BswM_ModeRequests[idx].IsValid = FALSE;
    }

    BswM_PendingRequestCount = 0U;
    BswM_State = BSWM_INIT;
}

void BswM_Deinit(void)
{
    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_DEINIT, BSWM_E_UNINIT);
        return;
    }

    BswM_PendingRequestCount = 0U;
    BswM_State = BSWM_UNINIT;
}

Std_ReturnType BswM_RequestMode(uint16 RequesterId, BswM_ModeType RequestedMode)
{
    uint8 idx;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }

    /* Look for existing request from this requester */
    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if ((BswM_ModeRequests[idx].IsValid == TRUE) &&
            (BswM_ModeRequests[idx].RequesterId == RequesterId))
        {
            BswM_ModeRequests[idx].RequestedMode = RequestedMode;
            return E_OK;
        }
    }

    /* Add new request */
    if (BswM_PendingRequestCount >= BSWM_MAX_MODE_REQUESTS)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_REQ_MODE_OUT_OF_RANGE);
        return E_NOT_OK;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if (BswM_ModeRequests[idx].IsValid == FALSE)
        {
            BswM_ModeRequests[idx].RequesterId = RequesterId;
            BswM_ModeRequests[idx].RequestedMode = RequestedMode;
            BswM_ModeRequests[idx].IsValid = TRUE;
            BswM_PendingRequestCount++;
            return E_OK;
        }
    }

    return E_NOT_OK;
}

BswM_ModeType BswM_GetCurrentMode(uint16 RequesterId)
{
    uint8 idx;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return (BswM_ModeType)0U;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if ((BswM_ModeRequests[idx].IsValid == TRUE) &&
            (BswM_ModeRequests[idx].RequesterId == RequesterId))
        {
            return BswM_ModeRequests[idx].RequestedMode;
        }
    }

    return (BswM_ModeType)0U;
}

void BswM_MainFunction(void)
{
    if (BswM_State != BSWM_INIT)
    {
        return;
    }

    /* Evaluate mode requests and trigger actions.
     * In a full implementation, this would iterate over configured rules,
     * evaluate conditions, and execute action lists. For now, this is a
     * simplified periodic processing stub that can be extended. */
}
