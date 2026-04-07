/********************************************************************************************************/
/************************************************INCLUDES***********************************************/
/********************************************************************************************************/

#include "Dem/Dem.h"
#include "Det/Det.h"
#include "NvM/NvM.h"
#include <stdio.h>
#include <string.h>

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr);
extern Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);

/* External API declarations (MISRA 8.4 visibility). */
void Dem_Init(void);
void Dem_Shutdown(void);
void Dem_MainFunction(void);
Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
Std_ReturnType Dem_GetLastEvent(Dem_EventRecordType *EventRecordPtr);
Std_ReturnType Dem_ReportDetError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, uint8* EventStatusPtr);
Std_ReturnType Dem_ClearDTC(uint32 DTC);
Std_ReturnType Dem_GetNumberOfFilteredDTC(uint8 StatusMask, uint16* NumberOfDTCs);
Std_ReturnType Dem_SetDTCFilter(uint8 StatusMask);
Std_ReturnType Dem_GetNextFilteredDTC(uint32* DTC, uint8* DTCStatus);
Std_ReturnType Dem_GetDTCOfEvent(Dem_EventIdType EventId, uint32* DtcNumber);

/********************************************************************************************************/
/******************************************GlobalVariables**********************************************/
/********************************************************************************************************/

static uint8 Dem_Initialized = FALSE;
static Dem_EventRecordType Dem_LastEvent = {0};

/* DTC storage */
Dem_DtcRecordType Dem_DtcStorage[DEM_MAX_NUMBER_OF_DTCS];
static Dem_DtcFilterType Dem_CurrentFilter = {0U, 0U};
static uint8 Dem_NvMWritePending = FALSE;
static uint8 Dem_NvMJobPending = FALSE;

/********************************************************************************************************/
/******************************************InternalFunctions********************************************/
/********************************************************************************************************/

/**
 * @brief Find a DTC record by event ID, or allocate a new slot.
 * @return Pointer to the record, or NULL if storage is full.
 */
static Dem_DtcRecordType* Dem_FindOrAllocateDtc(Dem_EventIdType EventId)
{
    uint16 i;
    uint16 freeSlot = DEM_MAX_NUMBER_OF_DTCS; /* Invalid sentinel */

    for (i = 0U; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if ((Dem_DtcStorage[i].IsUsed == TRUE) && (Dem_DtcStorage[i].EventId == EventId))
        {
            return &Dem_DtcStorage[i];
        }
        if ((Dem_DtcStorage[i].IsUsed == FALSE) && (freeSlot == DEM_MAX_NUMBER_OF_DTCS))
        {
            freeSlot = i;
        }
    }

    if (freeSlot < DEM_MAX_NUMBER_OF_DTCS)
    {
        Dem_DtcStorage[freeSlot].IsUsed   = TRUE;
        Dem_DtcStorage[freeSlot].EventId  = EventId;
        /* DTC number derived from event ID: 0xC00000 | EventId */
        Dem_DtcStorage[freeSlot].DtcNumber = 0x00C00000UL | (uint32)EventId;
        Dem_DtcStorage[freeSlot].StatusMask = DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OC
                                            | DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LC;
        Dem_DtcStorage[freeSlot].DebounceCounter = 0;
        return &Dem_DtcStorage[freeSlot];
    }

    return NULL;
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Dem_Init(void)
{
    NvM_RequestResultType DemNvMResult = NVM_REQ_NOT_OK;

    (void)memset(&Dem_LastEvent, 0, sizeof(Dem_LastEvent));
    (void)memset(&Dem_CurrentFilter, 0, sizeof(Dem_CurrentFilter));
    if ((NvM_GetErrorStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, &DemNvMResult) != E_OK) ||
        (DemNvMResult != NVM_REQ_OK))
    {
        (void)memset(Dem_DtcStorage, 0, sizeof(Dem_DtcStorage));
    }
    Dem_LastEvent.IsValid = FALSE;
    Dem_NvMWritePending = FALSE;
    Dem_NvMJobPending = FALSE;
    Dem_Initialized = TRUE;
}

void Dem_Shutdown(void)
{
    if (Dem_Initialized != FALSE)
    {
        (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, Dem_NvMWritePending);
    }
    Dem_Initialized = FALSE;
}

void Dem_MainFunction(void)
{
    if (Dem_Initialized == FALSE)
    {
        return;
    }

    if ((Dem_NvMWritePending == TRUE) && (Dem_NvMJobPending == FALSE))
    {
        if (NvM_WriteBlock(NVM_BLOCK_ID_DEM_DTC_STORAGE, Dem_DtcStorage) == E_OK)
        {
            Dem_NvMJobPending = TRUE;
        }
    }
    else if (Dem_NvMJobPending == TRUE)
    {
        NvM_RequestResultType DemNvMResult = NVM_REQ_PENDING;
        if (NvM_GetErrorStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, &DemNvMResult) == E_OK)
        {
            if (DemNvMResult == NVM_REQ_OK)
            {
                Dem_NvMWritePending = FALSE;
                Dem_NvMJobPending = FALSE;
            }
            else if (DemNvMResult != NVM_REQ_PENDING)
            {
                /* Retry in next main cycle on non-pending failure states. */
                Dem_NvMJobPending = FALSE;
            }
            else
            {
                /* No action required */
            }
        }
    }
    else
    {
        /* No action required */
    }
}

Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    if (Dem_Initialized == FALSE)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_REPORT_ERROR_STATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    Dem_LastEvent.ModuleId = EventId;
    Dem_LastEvent.InstanceId = 0U;
    Dem_LastEvent.ApiId = 0U;
    Dem_LastEvent.ErrorId = 0U;
    Dem_LastEvent.EventStatus = EventStatus;
    Dem_LastEvent.IsValid = TRUE;

    return E_OK;
}

Std_ReturnType Dem_GetLastEvent(Dem_EventRecordType *EventRecordPtr)
{
    if (Dem_Initialized == FALSE)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_GET_LAST_EVENT, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EventRecordPtr == NULL)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_GET_LAST_EVENT, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Dem_LastEvent.IsValid == FALSE)
    {
        return E_NOT_OK;
    }

    *EventRecordPtr = Dem_LastEvent;
    return E_OK;
}

Std_ReturnType Dem_ReportDetError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    if (Dem_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    Dem_LastEvent.ModuleId = ModuleId;
    Dem_LastEvent.InstanceId = InstanceId;
    Dem_LastEvent.ApiId = ApiId;
    Dem_LastEvent.ErrorId = ErrorId;
    Dem_LastEvent.EventStatus = DEM_EVENT_STATUS_FAILED;
    Dem_LastEvent.IsValid = TRUE;

    #ifdef DET_DEBUG
        (void)printf("[DEM] Logged DET error: M=%u I=%u A=%u E=%u\n", ModuleId, InstanceId, ApiId, ErrorId);
    #endif

    return E_OK;
}

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    Dem_DtcRecordType* pDtc;

    if (Dem_Initialized == FALSE)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_SET_EVENT_STATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    pDtc = Dem_FindOrAllocateDtc(EventId);
    if (pDtc == NULL)
    {
        return E_NOT_OK;
    }

    /* Clear "not completed" bits since we are processing */
    pDtc->StatusMask &= (uint8)(~DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OC);
    pDtc->StatusMask &= (uint8)(~DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LC);

    switch (EventStatus)
    {
        case DEM_EVENT_STATUS_FAILED:
            pDtc->DebounceCounter = (sint8)DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
            pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED;
            pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE;
            pDtc->StatusMask |= DEM_DTC_STATUS_PENDING_DTC;
            pDtc->StatusMask |= DEM_DTC_STATUS_CONFIRMED_DTC;
            pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED_SINCE_LC;
            break;

        case DEM_EVENT_STATUS_PASSED:
            pDtc->DebounceCounter = (sint8)DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
            pDtc->StatusMask &= (uint8)(~DEM_DTC_STATUS_TEST_FAILED);
            break;

        case DEM_EVENT_STATUS_PREFAILED:
            if (pDtc->DebounceCounter < (sint8)DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
            {
                pDtc->DebounceCounter++;
            }
            if (pDtc->DebounceCounter >= (sint8)DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
            {
                pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED;
                pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE;
                pDtc->StatusMask |= DEM_DTC_STATUS_PENDING_DTC;
                pDtc->StatusMask |= DEM_DTC_STATUS_CONFIRMED_DTC;
                pDtc->StatusMask |= DEM_DTC_STATUS_TEST_FAILED_SINCE_LC;
            }
            break;

        case DEM_EVENT_STATUS_PREPASSED:
            if (pDtc->DebounceCounter > (sint8)DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
            {
                pDtc->DebounceCounter--;
            }
            if (pDtc->DebounceCounter <= (sint8)DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
            {
                pDtc->StatusMask &= (uint8)(~DEM_DTC_STATUS_TEST_FAILED);
            }
            break;

        default:
            (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_SET_EVENT_STATUS, DEM_E_PARAM_DATA);
            return E_NOT_OK;
    }

    Dem_NvMWritePending = TRUE;
    (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, TRUE);
    return E_OK;
}

Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, uint8* EventStatusPtr)
{
    uint16 i;

    if (Dem_Initialized == FALSE)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_GET_EVENT_STATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EventStatusPtr == NULL)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_GET_EVENT_STATUS, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    for (i = 0U; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if ((Dem_DtcStorage[i].IsUsed == TRUE) && (Dem_DtcStorage[i].EventId == EventId))
        {
            *EventStatusPtr = Dem_DtcStorage[i].StatusMask;
            return E_OK;
        }
    }

    return E_NOT_OK;
}

Std_ReturnType Dem_ClearDTC(uint32 DTC)
{
    uint16 i;

    if (Dem_Initialized == FALSE)
    {
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, DEM_SID_CLEAR_DTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    /* DTC == 0xFFFFFF means clear all */
    for (i = 0U; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if (Dem_DtcStorage[i].IsUsed == TRUE)
        {
            if ((DTC == 0x00FFFFFFUL) || (Dem_DtcStorage[i].DtcNumber == DTC))
            {
                Dem_DtcStorage[i].StatusMask      = 0x00U;
                Dem_DtcStorage[i].DebounceCounter  = 0;
                Dem_DtcStorage[i].IsUsed           = FALSE;
                Dem_NvMWritePending = TRUE;
            }
        }
    }

    if (Dem_NvMWritePending == TRUE)
    {
        (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, TRUE);
    }
    return E_OK;
}

Std_ReturnType Dem_GetNumberOfFilteredDTC(uint8 StatusMask, uint16* NumberOfDTCs)
{
    uint16 i;
    uint16 count = 0U;

    if (Dem_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if (NumberOfDTCs == NULL)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if ((Dem_DtcStorage[i].IsUsed == TRUE) &&
            ((Dem_DtcStorage[i].StatusMask & StatusMask) != 0U))
        {
            count++;
        }
    }

    *NumberOfDTCs = count;
    return E_OK;
}

Std_ReturnType Dem_SetDTCFilter(uint8 StatusMask)
{
    if (Dem_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    Dem_CurrentFilter.StatusMask   = StatusMask;
    Dem_CurrentFilter.CurrentIndex = 0U;
    return E_OK;
}

Std_ReturnType Dem_GetNextFilteredDTC(uint32* DTC, uint8* DTCStatus)
{
    uint16 i;

    if (Dem_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if ((DTC == NULL) || (DTCStatus == NULL))
    {
        return E_NOT_OK;
    }

    for (i = Dem_CurrentFilter.CurrentIndex; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if ((Dem_DtcStorage[i].IsUsed == TRUE) &&
            ((Dem_DtcStorage[i].StatusMask & Dem_CurrentFilter.StatusMask) != 0U))
        {
            *DTC       = Dem_DtcStorage[i].DtcNumber;
            *DTCStatus = Dem_DtcStorage[i].StatusMask;
            Dem_CurrentFilter.CurrentIndex = i + 1U;
            return E_OK;
        }
    }

    return E_NOT_OK; /* No more matching DTCs */
}

Std_ReturnType Dem_GetDTCOfEvent(Dem_EventIdType EventId, uint32* DtcNumber)
{
    uint16 i;

    if (Dem_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if (DtcNumber == NULL)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < DEM_MAX_NUMBER_OF_DTCS; i++)
    {
        if ((Dem_DtcStorage[i].IsUsed == TRUE) && (Dem_DtcStorage[i].EventId == EventId))
        {
            *DtcNumber = Dem_DtcStorage[i].DtcNumber;
            return E_OK;
        }
    }

    return E_NOT_OK;
}
