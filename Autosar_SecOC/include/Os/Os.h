/**
 * @file Os.h
 * @brief AUTOSAR OS (OSEK) Module Header
 * @details Cooperative non-preemptive OS with tasks, alarms, counters,
 *          resources, events, and schedule tables. Portable across
 *          Windows and Linux (no pthread/ucontext dependencies).
 */

#ifndef OS_H
#define OS_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Os_Cfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define OS_MODULE_ID                ((uint16)0x01U)
#define OS_INSTANCE_ID              ((uint8)0x00U)

/* Service IDs */
#define OS_SID_INIT                 ((uint8)0x00U)
#define OS_SID_START_OS             ((uint8)0x01U)
#define OS_SID_SHUTDOWN_OS          ((uint8)0x02U)
#define OS_SID_ACTIVATE_TASK        ((uint8)0x03U)
#define OS_SID_TERMINATE_TASK       ((uint8)0x04U)
#define OS_SID_CHAIN_TASK           ((uint8)0x05U)
#define OS_SID_GET_TASK_ID          ((uint8)0x06U)
#define OS_SID_GET_TASK_STATE       ((uint8)0x07U)
#define OS_SID_SET_REL_ALARM        ((uint8)0x08U)
#define OS_SID_SET_ABS_ALARM        ((uint8)0x09U)
#define OS_SID_CANCEL_ALARM         ((uint8)0x0AU)
#define OS_SID_GET_ALARM            ((uint8)0x0BU)
#define OS_SID_GET_ALARM_BASE       ((uint8)0x0CU)
#define OS_SID_GET_RESOURCE         ((uint8)0x0DU)
#define OS_SID_RELEASE_RESOURCE     ((uint8)0x0EU)
#define OS_SID_SET_EVENT            ((uint8)0x0FU)
#define OS_SID_CLEAR_EVENT          ((uint8)0x10U)
#define OS_SID_GET_EVENT            ((uint8)0x11U)
#define OS_SID_WAIT_EVENT           ((uint8)0x12U)
#define OS_SID_INCREMENT_COUNTER    ((uint8)0x13U)
#define OS_SID_GET_COUNTER_VALUE    ((uint8)0x14U)
#define OS_SID_MAIN_FUNCTION        ((uint8)0x15U)
#define OS_SID_START_SCHED_TABLE_REL ((uint8)0x16U)
#define OS_SID_STOP_SCHED_TABLE     ((uint8)0x17U)
#define OS_SID_DISABLE_ALL_INTERRUPTS ((uint8)0x18U)
#define OS_SID_ENABLE_ALL_INTERRUPTS ((uint8)0x19U)
#define OS_SID_SUSPEND_ALL_INTERRUPTS ((uint8)0x1AU)
#define OS_SID_RESUME_ALL_INTERRUPTS ((uint8)0x1BU)
#define OS_SID_SUSPEND_OS_INTERRUPTS ((uint8)0x1CU)
#define OS_SID_RESUME_OS_INTERRUPTS ((uint8)0x1DU)
#define OS_SID_START_SCHED_TABLE_ABS ((uint8)0x1EU)
#define OS_SID_NEXT_SCHED_TABLE      ((uint8)0x1FU)
#define OS_SID_GET_ISR_ID            ((uint8)0x20U)
#define OS_SID_GET_APPLICATION_ID    ((uint8)0x21U)
#define OS_SID_CHECK_OBJECT_ACCESS   ((uint8)0x22U)
#define OS_SID_TERMINATE_APPLICATION ((uint8)0x23U)
#define OS_SID_CALL_TRUSTED_FUNCTION ((uint8)0x24U)
#define OS_SID_ENTER_ISR             ((uint8)0x25U)
#define OS_SID_EXIT_ISR              ((uint8)0x26U)
#define OS_SID_ASSIGN_OBJECT_APP     ((uint8)0x27U)

/* DET Error Codes */
#define OS_E_UNINIT                 ((uint8)0x01U)
#define OS_E_PARAM_ID               ((uint8)0x02U)
#define OS_E_PARAM_POINTER          ((uint8)0x03U)
#define OS_E_STATE                  ((uint8)0x04U)
#define OS_E_LIMIT                  ((uint8)0x05U)
#define OS_E_NOFUNC                 ((uint8)0x06U)
#define OS_E_RESOURCE               ((uint8)0x07U)

/* OSEK Status Codes */
#define E_OS_OK                     ((StatusType)0x00U)
#define E_OS_ACCESS                 ((StatusType)0x01U)
#define E_OS_CALLEVEL               ((StatusType)0x02U)
#define E_OS_ID                     ((StatusType)0x03U)
#define E_OS_LIMIT                  ((StatusType)0x04U)
#define E_OS_NOFUNC                 ((StatusType)0x05U)
#define E_OS_RESOURCE               ((StatusType)0x06U)
#define E_OS_STATE                  ((StatusType)0x07U)
#define E_OS_VALUE                  ((StatusType)0x08U)

/* Invalid task ID sentinel */
#define INVALID_TASK                ((TaskType)0xFFU)
#define INVALID_ISR                 ((ISRType)0xFFU)
#define INVALID_OSAPPLICATION       ((ApplicationType)0xFFU)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef uint8   StatusType;
typedef uint8   TaskType;
typedef uint8   AlarmType;
typedef uint8   CounterType;
typedef uint8   ResourceType;
typedef uint8   AppModeType;
typedef uint8   ScheduleTableType;
typedef uint8   ISRType;
typedef uint8   ApplicationType;
typedef uint8   TrustedFunctionIndexType;
typedef void*   TrustedFunctionParameterRefType;
typedef uint32  TickType;
typedef uint32  EventMaskType;

typedef TaskType*       TaskRefType;
typedef TickType*       TickRefType;
typedef EventMaskType*  EventMaskRefType;
typedef void (*Os_HookFunctionType)(void);
typedef void (*Os_TrustedFunctionType)(TrustedFunctionParameterRefType params);

typedef enum
{
    OS_OBJECT_TASK = 0,
    OS_OBJECT_ALARM,
    OS_OBJECT_COUNTER,
    OS_OBJECT_RESOURCE,
    OS_OBJECT_SCHEDULETABLE
} Os_ObjectType_Type;

typedef uint8 RestartType;
#define NO_RESTART                  ((RestartType)0x00U)
#define RESTART                     ((RestartType)0x01U)

typedef struct
{
    TickType maxallowedvalue;
    TickType ticksperbase;
    TickType mincycle;
} AlarmBaseType;

typedef AlarmBaseType* AlarmBaseRefType;

/* Task States */
typedef enum
{
    OS_TASK_SUSPENDED = 0,
    OS_TASK_READY,
    OS_TASK_RUNNING,
    OS_TASK_WAITING
} Os_TaskStateType;

typedef Os_TaskStateType* TaskStateRefType;

/* Task Type (Basic / Extended) */
typedef enum
{
    OS_TASK_BASIC = 0,
    OS_TASK_EXTENDED
} Os_TaskKindType;

/* Task entry function pointer */
typedef void (*Os_TaskEntryType)(void);

/* Alarm action type */
typedef enum
{
    OS_ALARM_ACTION_ACTIVATE_TASK = 0,
    OS_ALARM_ACTION_CALLBACK
} Os_AlarmActionKindType;

/* Alarm callback type */
typedef void (*Os_AlarmCallbackType)(void);

/* AppMode constants */
#define OSDEFAULTAPPMODE            ((AppModeType)0x00U)

/* DeclareTask / DeclareAlarm macros (OSEK compatibility) */
#define DeclareTask(TaskID)
#define DeclareAlarm(AlarmID)

/********************************************************************************************************/
/*************************************Configuration Structures*******************************************/
/********************************************************************************************************/

/* Task static configuration */
typedef struct
{
    Os_TaskEntryType    Entry;
    uint8               Priority;
    Os_TaskKindType     Kind;
    boolean             Autostart;
} Os_TaskConfigType;

/* Alarm action configuration */
typedef struct
{
    Os_AlarmActionKindType  ActionKind;
    TaskType                TaskID;
    Os_AlarmCallbackType    Callback;
} Os_AlarmActionType;

/* Alarm static configuration */
typedef struct
{
    CounterType             CounterRef;
    Os_AlarmActionType      Action;
} Os_AlarmConfigType;

/* Counter static configuration */
typedef struct
{
    TickType    MaxAllowedValue;
    TickType    TicksPerBase;
    TickType    MinCycle;
} Os_CounterConfigType;

/* Schedule table expiry point */
typedef struct
{
    TickType        Offset;
    TaskType        TaskID;
} Os_ExpiryPointType;

/* Schedule table configuration */
typedef struct
{
    CounterType             CounterRef;
    TickType                Duration;
    uint8                   NumExpiryPoints;
    const Os_ExpiryPointType* ExpiryPoints;
} Os_ScheduleTableConfigType;

/* Full OS configuration */
typedef struct
{
    const Os_TaskConfigType*            Tasks;
    uint8                               NumTasks;
    const Os_AlarmConfigType*           Alarms;
    uint8                               NumAlarms;
    const Os_CounterConfigType*         Counters;
    uint8                               NumCounters;
    const Os_ScheduleTableConfigType*   ScheduleTables;
    uint8                               NumScheduleTables;
} Os_ConfigType;

// cppcheck-suppress misra-c2012-8.4
extern const Os_ConfigType Os_Config;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/* Startup / Shutdown */
void Os_Init(const Os_ConfigType* ConfigPtr);
void StartOS(AppModeType Mode);
void ShutdownOS(StatusType Error);
AppModeType GetActiveApplicationMode(void);
StatusType Schedule(void);

/* Optional hooks */
void Os_SetStartupHook(Os_HookFunctionType Hook);
void Os_SetShutdownHook(Os_HookFunctionType Hook);
void Os_SetErrorHook(Os_HookFunctionType Hook);
void Os_SetPreTaskHook(Os_HookFunctionType Hook);
void Os_SetPostTaskHook(Os_HookFunctionType Hook);
void Os_SetProtectionHook(Os_HookFunctionType Hook);

/* Task Management */
StatusType ActivateTask(TaskType TaskID);
StatusType TerminateTask(void);
StatusType ChainTask(TaskType TaskID);
StatusType GetTaskID(TaskRefType TaskID);
StatusType GetTaskState(TaskType TaskID, TaskStateRefType State);

/* Alarms and Counters */
StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle);
StatusType SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle);
StatusType CancelAlarm(AlarmType AlarmID);
StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick);
StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info);
StatusType IncrementCounter(CounterType CounterID);
StatusType Os_GetCounterValue(CounterType CounterID, TickRefType Value);

/* Resources */
StatusType GetResource(ResourceType ResID);
StatusType ReleaseResource(ResourceType ResID);

/* Events */
StatusType Os_SetEvent(TaskType TaskID, EventMaskType Mask);
StatusType Os_ClearEvent(EventMaskType Mask);
StatusType Os_GetEvent(TaskType TaskID, EventMaskRefType Event);
StatusType Os_WaitEvent(EventMaskType Mask);

/* OSEK-compatible macros (disabled on Windows due to WinAPI conflicts) */
#ifndef WINDOWS
#define SetEvent    Os_SetEvent
#define ClearEvent  Os_ClearEvent
#define GetEvent    Os_GetEvent
#define WaitEvent   Os_WaitEvent
#endif

/* Schedule Tables */
StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset);
StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start);
StatusType NextScheduleTable(ScheduleTableType ScheduleTableID_From, ScheduleTableType ScheduleTableID_To);
StatusType StopScheduleTable(ScheduleTableType ScheduleTableID);

/* Interrupt Control Services */
void DisableAllInterrupts(void);
void EnableAllInterrupts(void);
void SuspendAllInterrupts(void);
void ResumeAllInterrupts(void);
void SuspendOSInterrupts(void);
void ResumeOSInterrupts(void);

/* ISR / OS-Application and Protection Services */
ISRType GetISRID(void);
ApplicationType GetApplicationID(void);
StatusType CheckObjectAccess(ApplicationType ApplID, Os_ObjectType_Type ObjectType, uint32 ObjectID);
StatusType TerminateApplication(ApplicationType Application, RestartType RestartOption);
StatusType CallTrustedFunction(TrustedFunctionIndexType FunctionIndex, TrustedFunctionParameterRefType FunctionParams);
StatusType Os_RegisterTrustedFunction(TrustedFunctionIndexType FunctionIndex, Os_TrustedFunctionType FunctionPtr);
StatusType Os_EnterISR(ISRType IsrID, ApplicationType OwnerApplication);
void Os_ExitISR(void);
StatusType Os_AssignObjectToApplication(Os_ObjectType_Type ObjectType, uint32 ObjectID, ApplicationType ApplID);

/* Main Function (called from main loop) */
void Os_MainFunction(void);

#endif /* OS_H */
