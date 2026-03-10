#ifndef INCLUDE_DEM_H_
#define INCLUDE_DEM_H_

/********************************************************************************************************/
/************************************************INCLUDES***********************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define DEM_MODULE_ID                        ((uint16)54U)
#define DEM_INSTANCE_ID                      ((uint8)0U)

/* Service IDs */
#define DEM_SID_INIT                         ((uint8)0x01U)
#define DEM_SID_REPORT_ERROR_STATUS          ((uint8)0x02U)
#define DEM_SID_GET_LAST_EVENT               ((uint8)0x03U)
#define DEM_SID_SET_EVENT_STATUS             ((uint8)0x04U)
#define DEM_SID_GET_EVENT_STATUS             ((uint8)0x05U)
#define DEM_SID_CLEAR_DTC                    ((uint8)0x06U)
#define DEM_SID_MAIN_FUNCTION                ((uint8)0x07U)
#define DEM_SID_GET_DTC_BY_INDEX             ((uint8)0x08U)
#define DEM_SID_GET_NUMBER_OF_DTCS           ((uint8)0x09U)
#define DEM_SID_GETVERSIONINFO               ((uint8)0x0AU)
#define DEM_SID_SHUTDOWN                     ((uint8)0x0BU)

/* DET Error Codes */
#define DEM_E_UNINIT                         ((uint8)0x01U)
#define DEM_E_PARAM_POINTER                  ((uint8)0x02U)
#define DEM_E_PARAM_DATA                     ((uint8)0x03U)
#define DEM_E_PARAM_LENGTH                   ((uint8)0x04U)

/* DTC Configuration */
#define DEM_MAX_NUMBER_OF_EVENTS             (32U)
#define DEM_MAX_NUMBER_OF_DTCS               (32U)

/* DTC Status Bits per ISO 14229 */
#define DEM_DTC_STATUS_TEST_FAILED                  ((uint8)0x01U)
#define DEM_DTC_STATUS_TEST_FAILED_THIS_OP_CYCLE    ((uint8)0x02U)
#define DEM_DTC_STATUS_PENDING_DTC                  ((uint8)0x04U)
#define DEM_DTC_STATUS_CONFIRMED_DTC                ((uint8)0x08U)
#define DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LC  ((uint8)0x10U)
#define DEM_DTC_STATUS_TEST_FAILED_SINCE_LC         ((uint8)0x20U)
#define DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OC   ((uint8)0x40U)
#define DEM_DTC_STATUS_WARNING_INDICATOR_REQUESTED   ((uint8)0x80U)

/* Debounce thresholds */
#define DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD   (3)
#define DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD   (-3)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef uint16 Dem_EventIdType;

typedef enum
{
    DEM_EVENT_STATUS_PASSED = 0,
    DEM_EVENT_STATUS_FAILED = 1,
    DEM_EVENT_STATUS_PREPASSED = 2,
    DEM_EVENT_STATUS_PREFAILED = 3
} Dem_EventStatusType;

typedef struct
{
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
    Dem_EventStatusType EventStatus;
    uint8 IsValid;
} Dem_EventRecordType;

/** DTC record stored in memory */
typedef struct
{
    uint32             DtcNumber;       /* 3-byte DTC value (stored in lower 24 bits) */
    uint8              StatusMask;      /* UDS DTC status byte */
    Dem_EventIdType    EventId;         /* Associated event ID */
    sint8              DebounceCounter; /* Counter-based debounce value */
    uint8              IsUsed;          /* Slot in use flag */
} Dem_DtcRecordType;

/** DTC filter for Dcm queries */
typedef struct
{
    uint8  StatusMask;
    uint16 CurrentIndex;
} Dem_DtcFilterType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Dem_Init(void);
void Dem_Shutdown(void);
void Dem_MainFunction(void);

Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
Std_ReturnType Dem_GetLastEvent(Dem_EventRecordType *EventRecordPtr);
Std_ReturnType Dem_ReportDetError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);

/** Set event status with debouncing support */
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);

/** Get the UDS status byte for an event */
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, uint8* EventStatusPtr);

/** Clear all DTCs or a specific DTC group */
Std_ReturnType Dem_ClearDTC(uint32 DTC);

/** Get number of stored DTCs matching a status mask */
Std_ReturnType Dem_GetNumberOfFilteredDTC(uint8 StatusMask, uint16* NumberOfDTCs);

/** Set DTC filter for sequential reading */
Std_ReturnType Dem_SetDTCFilter(uint8 StatusMask);

/** Get next filtered DTC */
Std_ReturnType Dem_GetNextFilteredDTC(uint32* DTC, uint8* DTCStatus);

/** Map event ID to DTC number */
Std_ReturnType Dem_GetDTCOfEvent(Dem_EventIdType EventId, uint32* DtcNumber);

#endif /* INCLUDE_DEM_H_ */
