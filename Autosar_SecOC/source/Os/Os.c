/**
 * @file Os.c
 * @brief AUTOSAR OS (OSEK) Module Implementation
 * @details Cooperative non-preemptive OS. Portable across Windows and Linux.
 *          No pthread/ucontext dependencies. All static allocation.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Os.h"
#include "Det.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

/********************************************************************************************************/
/********************************************InternalTypes************************************************/
/********************************************************************************************************/

typedef struct
{
    Os_TaskStateType    State;
    uint8               Priority;
    Os_TaskKindType     Kind;
    Os_TaskEntryType    Entry;
    EventMaskType       EventsSet;
    EventMaskType       EventsWaiting;
    uint8               ActivationCount;
    uint8               ResourceCount;
} Os_TaskControlType;

typedef struct
{
    boolean     Active;
    TickType    Expiry;
    TickType    Cycle;
    CounterType CounterRef;
    Os_AlarmActionType Action;
} Os_AlarmControlType;

typedef struct
{
    TickType    Value;
    TickType    MaxAllowedValue;
    TickType    TicksPerBase;
    TickType    MinCycle;
} Os_CounterControlType;

typedef enum
{
    OS_SCHED_TABLE_STOPPED = 0,
    OS_SCHED_TABLE_RUNNING
} Os_SchedTableStateType;

typedef struct
{
    Os_SchedTableStateType  State;
    TickType                StartTick;
    boolean                 OffsetElapsed;
    uint8                   NextExpiryIndex;
    boolean                 NextPending;
    ScheduleTableType       NextTableID;
    const Os_ScheduleTableConfigType* Config;
} Os_SchedTableControlType;

typedef struct
{
    ResourceType    OwnerTask;
    boolean         Occupied;
    uint8           CeilingPriority;
    uint8           SavedPriority;
} Os_ResourceControlType;

/********************************************************************************************************/
/********************************************GlobalVariables*********************************************/
/********************************************************************************************************/

static boolean              Os_Initialized = FALSE;
static boolean              Os_Started = FALSE;
static TaskType             Os_CurrentTask = INVALID_TASK;
static const Os_ConfigType* Os_ConfigPtr = NULL;
static AppModeType          Os_ActiveAppMode = OSDEFAULTAPPMODE;
static Os_HookFunctionType  Os_StartupHook = NULL;
static Os_HookFunctionType  Os_ShutdownHook = NULL;
static Os_HookFunctionType  Os_ErrorHook = NULL;
static Os_HookFunctionType  Os_PreTaskHook = NULL;
static Os_HookFunctionType  Os_PostTaskHook = NULL;
static Os_HookFunctionType  Os_ProtectionHook = NULL;
static ISRType              Os_CurrentIsr = INVALID_ISR;
static ApplicationType      Os_CurrentApplication = (ApplicationType)0U;

static Os_TaskControlType       Os_Tasks[OS_MAX_TASKS];
static Os_AlarmControlType      Os_Alarms[OS_MAX_ALARMS];
static Os_CounterControlType    Os_Counters[OS_MAX_COUNTERS];
static Os_ResourceControlType   Os_Resources[OS_MAX_RESOURCES];
static Os_SchedTableControlType Os_SchedTables[OS_MAX_SCHEDULE_TABLES];
static ApplicationType          Os_TaskOwnerApp[OS_MAX_TASKS];
static ApplicationType          Os_AlarmOwnerApp[OS_MAX_ALARMS];
static ApplicationType          Os_CounterOwnerApp[OS_MAX_COUNTERS];
static ApplicationType          Os_ResourceOwnerApp[OS_MAX_RESOURCES];
static ApplicationType          Os_SchedTableOwnerApp[OS_MAX_SCHEDULE_TABLES];
static Os_TrustedFunctionType   Os_TrustedFunctions[OS_MAX_TRUSTED_FUNCTIONS];

static uint32 Os_LastTickMs = 0U;
static uint8 Os_AllInterruptSuspendNesting = 0U;
static uint8 Os_OsInterruptSuspendNesting = 0U;
static boolean Os_GlobalInterruptDisabled = FALSE;
static uint8 Os_TrustedCallNesting = 0U;

static void Os_ReportProtectionViolation(void)
{
    if (Os_ProtectionHook != NULL)
    {
        Os_ProtectionHook();
    }
}

static StatusType Os_CheckCurrentApplicationObjectAccess(Os_ObjectType_Type ObjectType, uint32 ObjectID)
{
    StatusType accessStatus;

    if ((Os_CurrentTask == INVALID_TASK) && (Os_CurrentIsr == INVALID_ISR))
    {
        return E_OS_OK;
    }

    accessStatus = CheckObjectAccess(Os_CurrentApplication, ObjectType, ObjectID);
    if (accessStatus != E_OS_OK)
    {
        Os_ReportProtectionViolation();
    }
    return accessStatus;
}

/********************************************************************************************************/
/******************************************InternalFunctions*********************************************/
/********************************************************************************************************/

static uint32 Os_GetSystemTimeMs(void)
{
#ifdef _WIN32
    return (uint32)GetTickCount();
#else
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32)((uint32)ts.tv_sec * 1000U + (uint32)(ts.tv_nsec / 1000000));
#endif
}

static TickType Os_TickAdd(TickType Current, TickType Delta, TickType MaxAllowedValue)
{
    TickType remaining = MaxAllowedValue - Current;
    if (Delta > remaining)
    {
        return Delta - remaining - 1U;
    }
    return Current + Delta;
}

static TickType Os_TickDistance(TickType Start, TickType End, TickType MaxAllowedValue)
{
    if (End >= Start)
    {
        return End - Start;
    }
    return (MaxAllowedValue - Start) + End + 1U;
}

static void Os_RecalculateTaskPriority(TaskType TaskID)
{
    uint8 effectivePriority;
    ResourceType i;

    effectivePriority = Os_ConfigPtr->Tasks[TaskID].Priority;

    for (i = 0U; i < OS_MAX_RESOURCES; i++)
    {
        if ((Os_Resources[i].Occupied != FALSE) && (Os_Resources[i].OwnerTask == TaskID))
        {
            if (Os_Resources[i].CeilingPriority > effectivePriority)
            {
                effectivePriority = Os_Resources[i].CeilingPriority;
            }
        }
    }

    Os_Tasks[TaskID].Priority = effectivePriority;
}

static void Os_ForceReleaseTaskResources(TaskType TaskID)
{
    ResourceType i;

    for (i = 0U; i < OS_MAX_RESOURCES; i++)
    {
        if ((Os_Resources[i].Occupied != FALSE) && (Os_Resources[i].OwnerTask == TaskID))
        {
            Os_Resources[i].Occupied = FALSE;
            Os_Resources[i].OwnerTask = INVALID_TASK;
            Os_Resources[i].SavedPriority = 0U;
        }
    }

    Os_Tasks[TaskID].ResourceCount = 0U;
    if ((Os_ConfigPtr != NULL) && (TaskID < Os_ConfigPtr->NumTasks))
    {
        Os_Tasks[TaskID].Priority = Os_ConfigPtr->Tasks[TaskID].Priority;
    }
}

static void Os_ReportDetError(uint8 ServiceId, uint8 ErrorCode)
{
    (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, ServiceId, ErrorCode);
    if (Os_ErrorHook != NULL)
    {
        Os_ErrorHook();
    }
}

static void Os_ProcessAlarms(CounterType CounterID)
{
    uint8 i;

    for (i = 0U; i < OS_MAX_ALARMS; i++)
    {
        if ((Os_Alarms[i].Active != FALSE) && (Os_Alarms[i].CounterRef == CounterID))
        {
            if (Os_Counters[CounterID].Value == Os_Alarms[i].Expiry)
            {
                /* Alarm expired - execute action */
                if (Os_Alarms[i].Action.ActionKind == OS_ALARM_ACTION_ACTIVATE_TASK)
                {
                    (void)ActivateTask(Os_Alarms[i].Action.TaskID);
                }
                else if (Os_Alarms[i].Action.Callback != NULL)
                {
                    Os_Alarms[i].Action.Callback();
                }
                else
                {
                    /* No action configured */
                }

                /* Handle cyclic alarm */
                if (Os_Alarms[i].Cycle > 0U)
                {
                    Os_Alarms[i].Expiry = Os_TickAdd(Os_Counters[CounterID].Value,
                                                     Os_Alarms[i].Cycle,
                                                     Os_Counters[CounterID].MaxAllowedValue);
                }
                else
                {
                    Os_Alarms[i].Active = FALSE;
                }
            }
        }
    }
}

static void Os_ProcessScheduleTables(CounterType CounterID)
{
    uint8 i;

    for (i = 0U; i < OS_MAX_SCHEDULE_TABLES; i++)
    {
        if ((Os_SchedTables[i].State == OS_SCHED_TABLE_RUNNING) &&
            (Os_SchedTables[i].Config != NULL) &&
            (Os_SchedTables[i].Config->CounterRef == CounterID))
        {
            TickType elapsed;
            uint8 idx = Os_SchedTables[i].NextExpiryIndex;
            TickType currentTick = Os_Counters[CounterID].Value;
            TickType maxValue = Os_Counters[CounterID].MaxAllowedValue;

            if (Os_SchedTables[i].OffsetElapsed == FALSE)
            {
                if (currentTick == Os_SchedTables[i].StartTick)
                {
                    Os_SchedTables[i].OffsetElapsed = TRUE;
                }
                else
                {
                    continue;
                }
            }

            elapsed = Os_TickDistance(Os_SchedTables[i].StartTick, currentTick, maxValue);

            while (idx < Os_SchedTables[i].Config->NumExpiryPoints)
            {
                if (elapsed >= Os_SchedTables[i].Config->ExpiryPoints[idx].Offset)
                {
                    (void)ActivateTask(Os_SchedTables[i].Config->ExpiryPoints[idx].TaskID);
                    idx++;
                    Os_SchedTables[i].NextExpiryIndex = idx;
                }
                else
                {
                    break;
                }
            }

            /* Check if table completed */
            if (elapsed >= Os_SchedTables[i].Config->Duration)
            {
                ScheduleTableType nextId = Os_SchedTables[i].NextTableID;
                boolean startNext = Os_SchedTables[i].NextPending;
                CounterType nextCtr = 0U;

                Os_SchedTables[i].State = OS_SCHED_TABLE_STOPPED;
                Os_SchedTables[i].OffsetElapsed = FALSE;
                Os_SchedTables[i].NextExpiryIndex = 0U;
                Os_SchedTables[i].NextPending = FALSE;
                Os_SchedTables[i].NextTableID = (ScheduleTableType)0U;

                if (startNext != FALSE)
                {
                    if ((nextId < OS_MAX_SCHEDULE_TABLES) &&
                        (Os_SchedTables[nextId].Config != NULL))
                    {
                        nextCtr = Os_SchedTables[nextId].Config->CounterRef;
                        if ((Os_ConfigPtr != NULL) && (nextCtr < Os_ConfigPtr->NumCounters))
                        {
                            Os_SchedTables[nextId].StartTick = Os_Counters[nextCtr].Value;
                            Os_SchedTables[nextId].OffsetElapsed = TRUE;
                            Os_SchedTables[nextId].NextExpiryIndex = 0U;
                            Os_SchedTables[nextId].State = OS_SCHED_TABLE_RUNNING;
                        }
                    }
                }
            }
        }
    }
}

static void Os_DispatchTasks(void)
{
    uint8 i;
    TaskType bestTask = INVALID_TASK;
    uint8 bestPriority = 0U;

    /* Find highest priority READY task */
    for (i = 0U; i < Os_ConfigPtr->NumTasks; i++)
    {
        if (Os_Tasks[i].State == OS_TASK_READY)
        {
            if ((bestTask == INVALID_TASK) || (Os_Tasks[i].Priority > bestPriority))
            {
                bestTask = (TaskType)i;
                bestPriority = Os_Tasks[i].Priority;
            }
        }
    }

    if (bestTask != INVALID_TASK)
    {
        ApplicationType prevApplication = Os_CurrentApplication;
        Os_Tasks[bestTask].State = OS_TASK_RUNNING;
        Os_CurrentTask = bestTask;
        Os_CurrentApplication = Os_TaskOwnerApp[bestTask];

        if (Os_PreTaskHook != NULL)
        {
            Os_PreTaskHook();
        }

        /* Run to completion */
        if (Os_Tasks[bestTask].Entry != NULL)
        {
            Os_Tasks[bestTask].Entry();
        }

        if (Os_PostTaskHook != NULL)
        {
            Os_PostTaskHook();
        }

        /* If task did not call TerminateTask, auto-terminate */
        if (Os_Tasks[bestTask].State == OS_TASK_RUNNING)
        {
            /* Robust fallback for cooperative tasks that return without TerminateTask. */
            Os_ForceReleaseTaskResources(bestTask);
            Os_Tasks[bestTask].State = OS_TASK_SUSPENDED;
            if (Os_Tasks[bestTask].ActivationCount > 0U)
            {
                Os_Tasks[bestTask].ActivationCount--;
                if (Os_Tasks[bestTask].ActivationCount > 0U)
                {
                    Os_Tasks[bestTask].State = OS_TASK_READY;
                }
            }
        }

        Os_CurrentTask = INVALID_TASK;
        Os_CurrentApplication = prevApplication;
    }
}

/********************************************************************************************************/
/*****************************************PublicFunctions*************************************************/
/********************************************************************************************************/

void Os_Init(const Os_ConfigType* ConfigPtr)
{
    uint8 i;
    uint8 highestTaskPriority = 0U;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL)
    {
        Os_ReportDetError(OS_SID_INIT, OS_E_PARAM_POINTER);
        return;
    }
    if (ConfigPtr->NumTasks > OS_MAX_TASKS)
    {
        Os_ReportDetError(OS_SID_INIT, OS_E_LIMIT);
        return;
    }
    if (ConfigPtr->NumAlarms > OS_MAX_ALARMS)
    {
        Os_ReportDetError(OS_SID_INIT, OS_E_LIMIT);
        return;
    }
    if (ConfigPtr->NumCounters > OS_MAX_COUNTERS)
    {
        Os_ReportDetError(OS_SID_INIT, OS_E_LIMIT);
        return;
    }
#endif

    Os_ConfigPtr = ConfigPtr;

    /* Initialize tasks */
    for (i = 0U; i < OS_MAX_TASKS; i++)
    {
        Os_Tasks[i].State = OS_TASK_SUSPENDED;
        Os_Tasks[i].Priority = 0U;
        Os_Tasks[i].Kind = OS_TASK_BASIC;
        Os_Tasks[i].Entry = NULL;
        Os_Tasks[i].EventsSet = 0U;
        Os_Tasks[i].EventsWaiting = 0U;
        Os_Tasks[i].ActivationCount = 0U;
        Os_Tasks[i].ResourceCount = 0U;
        Os_TaskOwnerApp[i] = (ApplicationType)0U;
    }

    for (i = 0U; i < ConfigPtr->NumTasks; i++)
    {
        Os_Tasks[i].Entry = ConfigPtr->Tasks[i].Entry;
        Os_Tasks[i].Priority = ConfigPtr->Tasks[i].Priority;
        Os_Tasks[i].Kind = ConfigPtr->Tasks[i].Kind;
        if (ConfigPtr->Tasks[i].Priority > highestTaskPriority)
        {
            highestTaskPriority = ConfigPtr->Tasks[i].Priority;
        }
    }

    /* Initialize counters */
    for (i = 0U; i < OS_MAX_COUNTERS; i++)
    {
        Os_Counters[i].Value = 0U;
        Os_Counters[i].MaxAllowedValue = OS_SYSTEM_COUNTER_MAX_VALUE;
        Os_Counters[i].TicksPerBase = OS_SYSTEM_COUNTER_TICKS_PER_BASE;
        Os_Counters[i].MinCycle = OS_SYSTEM_COUNTER_MIN_CYCLE;
        Os_CounterOwnerApp[i] = (ApplicationType)0U;
    }

    for (i = 0U; i < ConfigPtr->NumCounters; i++)
    {
        Os_Counters[i].MaxAllowedValue = ConfigPtr->Counters[i].MaxAllowedValue;
        Os_Counters[i].TicksPerBase = ConfigPtr->Counters[i].TicksPerBase;
        Os_Counters[i].MinCycle = ConfigPtr->Counters[i].MinCycle;
    }

    /* Initialize alarms */
    for (i = 0U; i < OS_MAX_ALARMS; i++)
    {
        Os_Alarms[i].Active = FALSE;
        Os_Alarms[i].Expiry = 0U;
        Os_Alarms[i].Cycle = 0U;
        Os_AlarmOwnerApp[i] = (ApplicationType)0U;
    }

    for (i = 0U; i < ConfigPtr->NumAlarms; i++)
    {
        Os_Alarms[i].CounterRef = ConfigPtr->Alarms[i].CounterRef;
        Os_Alarms[i].Action = ConfigPtr->Alarms[i].Action;
    }

    /* Initialize resources */
    for (i = 0U; i < OS_MAX_RESOURCES; i++)
    {
        Os_Resources[i].Occupied = FALSE;
        Os_Resources[i].OwnerTask = INVALID_TASK;
        Os_Resources[i].CeilingPriority = highestTaskPriority;
        Os_Resources[i].SavedPriority = 0U;
        Os_ResourceOwnerApp[i] = (ApplicationType)0U;
    }

    /* Initialize schedule tables */
    for (i = 0U; i < OS_MAX_SCHEDULE_TABLES; i++)
    {
        Os_SchedTables[i].State = OS_SCHED_TABLE_STOPPED;
        Os_SchedTables[i].StartTick = 0U;
        Os_SchedTables[i].OffsetElapsed = FALSE;
        Os_SchedTables[i].NextExpiryIndex = 0U;
        Os_SchedTables[i].NextPending = FALSE;
        Os_SchedTables[i].NextTableID = (ScheduleTableType)0U;
        Os_SchedTables[i].Config = NULL;
        Os_SchedTableOwnerApp[i] = (ApplicationType)0U;
    }

    for (i = 0U; i < OS_MAX_TRUSTED_FUNCTIONS; i++)
    {
        Os_TrustedFunctions[i] = NULL;
    }

    for (i = 0U; (i < ConfigPtr->NumScheduleTables) && (i < OS_MAX_SCHEDULE_TABLES); i++)
    {
        Os_SchedTables[i].Config = &ConfigPtr->ScheduleTables[i];
    }

    Os_CurrentTask = INVALID_TASK;
    Os_CurrentIsr = INVALID_ISR;
    Os_CurrentApplication = (ApplicationType)0U;
    Os_ActiveAppMode = OSDEFAULTAPPMODE;
    Os_Initialized = TRUE;
    Os_Started = FALSE;
}

void StartOS(AppModeType Mode)
{
    uint8 i;
    (void)Mode;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        Os_ReportDetError(OS_SID_START_OS, OS_E_UNINIT);
        return;
    }
#endif

    if (Os_Started != FALSE)
    {
        return;
    }

    Os_LastTickMs = Os_GetSystemTimeMs();
    Os_ActiveAppMode = Mode;
    Os_Started = TRUE;

    /* Autostart tasks */
    for (i = 0U; i < Os_ConfigPtr->NumTasks; i++)
    {
        if (Os_ConfigPtr->Tasks[i].Autostart != FALSE)
        {
            Os_Tasks[i].State = OS_TASK_READY;
            Os_Tasks[i].ActivationCount = 1U;
        }
    }

    if (Os_StartupHook != NULL)
    {
        Os_StartupHook();
    }
}

void ShutdownOS(StatusType Error)
{
    uint8 i;
    (void)Error;

    Os_Started = FALSE;

    for (i = 0U; i < OS_MAX_TASKS; i++)
    {
        Os_Tasks[i].State = OS_TASK_SUSPENDED;
        Os_Tasks[i].ActivationCount = 0U;
        Os_Tasks[i].ResourceCount = 0U;
        Os_ForceReleaseTaskResources((TaskType)i);
    }

    for (i = 0U; i < OS_MAX_ALARMS; i++)
    {
        Os_Alarms[i].Active = FALSE;
    }

    for (i = 0U; i < OS_MAX_SCHEDULE_TABLES; i++)
    {
        Os_SchedTables[i].State = OS_SCHED_TABLE_STOPPED;
        Os_SchedTables[i].OffsetElapsed = FALSE;
        Os_SchedTables[i].NextExpiryIndex = 0U;
        Os_SchedTables[i].NextPending = FALSE;
        Os_SchedTables[i].NextTableID = (ScheduleTableType)0U;
    }

    Os_CurrentTask = INVALID_TASK;

    if (Os_ShutdownHook != NULL)
    {
        Os_ShutdownHook();
    }
}

StatusType ActivateTask(TaskType TaskID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        Os_ReportDetError(OS_SID_ACTIVATE_TASK, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (TaskID >= Os_ConfigPtr->NumTasks))
    {
        Os_ReportDetError(OS_SID_ACTIVATE_TASK, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        Os_ReportDetError(OS_SID_ACTIVATE_TASK, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Tasks[TaskID].ActivationCount >= OS_MAX_TASK_ACTIVATIONS)
    {
        return E_OS_LIMIT;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_TASK, (uint32)TaskID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    if (Os_Tasks[TaskID].State != OS_TASK_SUSPENDED)
    {
        Os_Tasks[TaskID].ActivationCount++;
        return E_OS_OK;
    }

    Os_Tasks[TaskID].State = OS_TASK_READY;
    Os_Tasks[TaskID].ActivationCount = 1U;

    return E_OS_OK;
}

StatusType TerminateTask(void)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_CurrentTask == INVALID_TASK)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_TERMINATE_TASK, OS_E_NOFUNC);
        return E_OS_NOFUNC;
    }
    if (Os_Tasks[Os_CurrentTask].ResourceCount > 0U)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_TERMINATE_TASK, OS_E_RESOURCE);
        return E_OS_RESOURCE;
    }
#endif

    Os_Tasks[Os_CurrentTask].State = OS_TASK_SUSPENDED;

    if (Os_Tasks[Os_CurrentTask].ActivationCount > 0U)
    {
        Os_Tasks[Os_CurrentTask].ActivationCount--;
        if (Os_Tasks[Os_CurrentTask].ActivationCount > 0U)
        {
            Os_Tasks[Os_CurrentTask].State = OS_TASK_READY;
        }
    }

    return E_OS_OK;
}

StatusType ChainTask(TaskType TaskID)
{
    TaskType currentTaskId;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_CurrentTask == INVALID_TASK)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CHAIN_TASK, OS_E_NOFUNC);
        return E_OS_NOFUNC;
    }
    if ((Os_ConfigPtr == NULL) || (TaskID >= Os_ConfigPtr->NumTasks))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CHAIN_TASK, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Tasks[Os_CurrentTask].ResourceCount > 0U)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CHAIN_TASK, OS_E_RESOURCE);
        return E_OS_RESOURCE;
    }
#endif

    currentTaskId = Os_CurrentTask;

    /* Terminate current */
    Os_Tasks[currentTaskId].State = OS_TASK_SUSPENDED;
    if (Os_Tasks[currentTaskId].ActivationCount > 0U)
    {
        Os_Tasks[currentTaskId].ActivationCount--;
    }
    if (Os_Tasks[currentTaskId].ActivationCount > 0U)
    {
        Os_Tasks[currentTaskId].State = OS_TASK_READY;
    }

    /* Activate target */
    if (Os_Tasks[TaskID].State == OS_TASK_SUSPENDED)
    {
        Os_Tasks[TaskID].State = OS_TASK_READY;
        Os_Tasks[TaskID].ActivationCount = 1U;
    }
    else
    {
        if (Os_Tasks[TaskID].ActivationCount >= OS_MAX_TASK_ACTIVATIONS)
        {
            return E_OS_LIMIT;
        }
        Os_Tasks[TaskID].ActivationCount++;
    }

    return E_OS_OK;
}

StatusType GetTaskID(TaskRefType TaskID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_TASK_ID, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
#endif

    *TaskID = Os_CurrentTask;
    return E_OS_OK;
}

StatusType GetTaskState(TaskType TaskID, TaskStateRefType State)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (State == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_TASK_STATE, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (TaskID >= Os_ConfigPtr->NumTasks))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_TASK_STATE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
#endif

    *State = Os_Tasks[TaskID].State;
    return E_OS_OK;
}

StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle)
{
    CounterType ctr;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_REL_ALARM, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (AlarmID >= Os_ConfigPtr->NumAlarms))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_REL_ALARM, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_REL_ALARM, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    ctr = Os_Alarms[AlarmID].CounterRef;
    if ((Os_ConfigPtr == NULL) || (ctr >= Os_ConfigPtr->NumCounters))
    {
        return E_OS_ID;
    }

    if ((increment == 0U) || (increment > Os_Counters[ctr].MaxAllowedValue))
    {
        return E_OS_VALUE;
    }
    if ((cycle != 0U) &&
        ((cycle < Os_Counters[ctr].MinCycle) || (cycle > Os_Counters[ctr].MaxAllowedValue)))
    {
        return E_OS_VALUE;
    }

    if (Os_Alarms[AlarmID].Active != FALSE)
    {
        return E_OS_STATE;
    }

    Os_Alarms[AlarmID].Expiry = Os_TickAdd(Os_Counters[ctr].Value,
                                           increment,
                                           Os_Counters[ctr].MaxAllowedValue);

    Os_Alarms[AlarmID].Cycle = cycle;
    Os_Alarms[AlarmID].Active = TRUE;

    return E_OS_OK;
}

StatusType SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle)
{
    CounterType ctr;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_ABS_ALARM, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (AlarmID >= Os_ConfigPtr->NumAlarms))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_ABS_ALARM, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_ABS_ALARM, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    ctr = Os_Alarms[AlarmID].CounterRef;
    if ((Os_ConfigPtr == NULL) || (ctr >= Os_ConfigPtr->NumCounters))
    {
        return E_OS_ID;
    }

    if (start > Os_Counters[ctr].MaxAllowedValue)
    {
        return E_OS_VALUE;
    }
    if ((cycle != 0U) &&
        ((cycle < Os_Counters[ctr].MinCycle) || (cycle > Os_Counters[ctr].MaxAllowedValue)))
    {
        return E_OS_VALUE;
    }

    if (Os_Alarms[AlarmID].Active != FALSE)
    {
        return E_OS_STATE;
    }

    Os_Alarms[AlarmID].Expiry = start;
    Os_Alarms[AlarmID].Cycle = cycle;
    Os_Alarms[AlarmID].Active = TRUE;

    return E_OS_OK;
}

StatusType CancelAlarm(AlarmType AlarmID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CANCEL_ALARM, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (AlarmID >= Os_ConfigPtr->NumAlarms))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CANCEL_ALARM, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CANCEL_ALARM, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Alarms[AlarmID].Active == FALSE)
    {
        return E_OS_NOFUNC;
    }

    Os_Alarms[AlarmID].Active = FALSE;
    return E_OS_OK;
}

StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Tick == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_ALARM, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (AlarmID >= Os_ConfigPtr->NumAlarms))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_ALARM, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_ALARM, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Alarms[AlarmID].Active == FALSE)
    {
        return E_OS_NOFUNC;
    }

    {
        CounterType ctr = Os_Alarms[AlarmID].CounterRef;
        if (Os_Alarms[AlarmID].Expiry >= Os_Counters[ctr].Value)
        {
            *Tick = Os_Alarms[AlarmID].Expiry - Os_Counters[ctr].Value;
        }
        else
        {
            *Tick = (Os_Counters[ctr].MaxAllowedValue - Os_Counters[ctr].Value) + Os_Alarms[AlarmID].Expiry + 1U;
        }
    }

    return E_OS_OK;
}

StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info)
{
    CounterType ctr;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Info == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_ALARM_BASE, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (AlarmID >= Os_ConfigPtr->NumAlarms))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_ALARM_BASE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
#endif

    ctr = Os_Alarms[AlarmID].CounterRef;
    if ((Os_ConfigPtr == NULL) || (ctr >= Os_ConfigPtr->NumCounters))
    {
        return E_OS_ID;
    }

    Info->maxallowedvalue = Os_Counters[ctr].MaxAllowedValue;
    Info->ticksperbase = Os_Counters[ctr].TicksPerBase;
    Info->mincycle = Os_Counters[ctr].MinCycle;
    return E_OS_OK;
}

StatusType IncrementCounter(CounterType CounterID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_Initialized == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_INCREMENT_COUNTER, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (CounterID >= Os_ConfigPtr->NumCounters))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_INCREMENT_COUNTER, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_INCREMENT_COUNTER, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    Os_Counters[CounterID].Value++;
    if (Os_Counters[CounterID].Value > Os_Counters[CounterID].MaxAllowedValue)
    {
        Os_Counters[CounterID].Value = 0U;
    }

    Os_ProcessAlarms(CounterID);
    Os_ProcessScheduleTables(CounterID);

    return E_OS_OK;
}

StatusType Os_GetCounterValue(CounterType CounterID, TickRefType Value)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Value == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_COUNTER_VALUE, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (CounterID >= Os_ConfigPtr->NumCounters))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_COUNTER_VALUE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_COUNTER_VALUE, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    *Value = Os_Counters[CounterID].Value;
    return E_OS_OK;
}

StatusType GetResource(ResourceType ResID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (ResID >= OS_MAX_RESOURCES)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_RESOURCE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_CurrentTask == INVALID_TASK)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_RESOURCE, OS_E_NOFUNC);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_RESOURCE, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Resources[ResID].Occupied != FALSE)
    {
        return E_OS_ACCESS;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_RESOURCE, (uint32)ResID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    Os_Resources[ResID].Occupied = TRUE;
    Os_Resources[ResID].OwnerTask = Os_CurrentTask;
    Os_Resources[ResID].SavedPriority = Os_Tasks[Os_CurrentTask].Priority;

    /* Priority ceiling: raise task priority */
    if (Os_Resources[ResID].CeilingPriority > Os_Tasks[Os_CurrentTask].Priority)
    {
        Os_Tasks[Os_CurrentTask].Priority = Os_Resources[ResID].CeilingPriority;
    }
    Os_Tasks[Os_CurrentTask].ResourceCount++;

    return E_OS_OK;
}

StatusType ReleaseResource(ResourceType ResID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (ResID >= OS_MAX_RESOURCES)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_RELEASE_RESOURCE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Resources[ResID].Occupied == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_RELEASE_RESOURCE, OS_E_NOFUNC);
        return E_OS_NOFUNC;
    }
    if (Os_Resources[ResID].OwnerTask != Os_CurrentTask)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_RELEASE_RESOURCE, OS_E_RESOURCE);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_RELEASE_RESOURCE, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_RESOURCE, (uint32)ResID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    /* Release resource and recompute effective task priority. */
    if (Os_Tasks[Os_CurrentTask].ResourceCount > 0U)
    {
        Os_Tasks[Os_CurrentTask].ResourceCount--;
    }
    Os_Resources[ResID].Occupied = FALSE;
    Os_Resources[ResID].OwnerTask = INVALID_TASK;
    Os_Resources[ResID].SavedPriority = 0U;
    if (Os_ConfigPtr != NULL)
    {
        Os_RecalculateTaskPriority(Os_CurrentTask);
    }

    return E_OS_OK;
}

StatusType Os_SetEvent(TaskType TaskID, EventMaskType Mask)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_ConfigPtr == NULL) || (TaskID >= Os_ConfigPtr->NumTasks))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_EVENT, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Tasks[TaskID].Kind != OS_TASK_EXTENDED)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_EVENT, OS_E_STATE);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_SET_EVENT, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Tasks[TaskID].State == OS_TASK_SUSPENDED)
    {
        return E_OS_STATE;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_TASK, (uint32)TaskID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    Os_Tasks[TaskID].EventsSet |= Mask;

    /* If task is waiting for this event, move to READY */
    if (Os_Tasks[TaskID].State == OS_TASK_WAITING)
    {
        if ((Os_Tasks[TaskID].EventsSet & Os_Tasks[TaskID].EventsWaiting) != 0U)
        {
            Os_Tasks[TaskID].State = OS_TASK_READY;
            Os_Tasks[TaskID].EventsWaiting = 0U;
        }
    }

    return E_OS_OK;
}

StatusType Os_ClearEvent(EventMaskType Mask)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_CurrentTask == INVALID_TASK)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CLEAR_EVENT, OS_E_NOFUNC);
        return E_OS_NOFUNC;
    }
    if (Os_Tasks[Os_CurrentTask].Kind != OS_TASK_EXTENDED)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CLEAR_EVENT, OS_E_STATE);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_CLEAR_EVENT, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    Os_Tasks[Os_CurrentTask].EventsSet &= ~Mask;
    return E_OS_OK;
}

StatusType Os_GetEvent(TaskType TaskID, EventMaskRefType Event)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Event == NULL)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_EVENT, OS_E_PARAM_POINTER);
        return E_OS_ACCESS;
    }
    if ((Os_ConfigPtr == NULL) || (TaskID >= Os_ConfigPtr->NumTasks))
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_EVENT, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Tasks[TaskID].Kind != OS_TASK_EXTENDED)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_EVENT, OS_E_STATE);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_GET_EVENT, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_Tasks[TaskID].State == OS_TASK_SUSPENDED)
    {
        return E_OS_STATE;
    }

    *Event = Os_Tasks[TaskID].EventsSet;
    return E_OS_OK;
}

StatusType Os_WaitEvent(EventMaskType Mask)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if (Os_CurrentTask == INVALID_TASK)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_WAIT_EVENT, OS_E_NOFUNC);
        return E_OS_NOFUNC;
    }
    if (Os_Tasks[Os_CurrentTask].Kind != OS_TASK_EXTENDED)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_WAIT_EVENT, OS_E_STATE);
        return E_OS_ACCESS;
    }
    if (Os_Started == FALSE)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_WAIT_EVENT, OS_E_STATE);
        return E_OS_STATE;
    }
    if (Os_Tasks[Os_CurrentTask].ResourceCount > 0U)
    {
        (void)Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, OS_SID_WAIT_EVENT, OS_E_RESOURCE);
        return E_OS_RESOURCE;
    }
#endif

    /* Cooperative: if event already set, return immediately */
    if ((Os_Tasks[Os_CurrentTask].EventsSet & Mask) != 0U)
    {
        return E_OS_OK;
    }

    /* Mark as waiting - dispatcher will skip this task */
    Os_Tasks[Os_CurrentTask].EventsWaiting = Mask;
    Os_Tasks[Os_CurrentTask].State = OS_TASK_WAITING;

    return E_OS_OK;
}

StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset)
{
    CounterType ctr;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_ConfigPtr == NULL) || (ScheduleTableID >= Os_ConfigPtr->NumScheduleTables))
    {
        Os_ReportDetError(OS_SID_START_SCHED_TABLE_REL, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        Os_ReportDetError(OS_SID_START_SCHED_TABLE_REL, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_SchedTables[ScheduleTableID].Config == NULL)
    {
        return E_OS_ID;
    }

    ctr = Os_SchedTables[ScheduleTableID].Config->CounterRef;
    if ((Os_ConfigPtr == NULL) || (ctr >= Os_ConfigPtr->NumCounters))
    {
        return E_OS_ID;
    }

    if (Offset > Os_Counters[ctr].MaxAllowedValue)
    {
        return E_OS_VALUE;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_SCHEDULETABLE, (uint32)ScheduleTableID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    if (Os_SchedTables[ScheduleTableID].State == OS_SCHED_TABLE_RUNNING)
    {
        return E_OS_STATE;
    }

    Os_SchedTables[ScheduleTableID].StartTick = Os_TickAdd(Os_Counters[ctr].Value,
                                                           Offset,
                                                           Os_Counters[ctr].MaxAllowedValue);
    Os_SchedTables[ScheduleTableID].OffsetElapsed = (Offset == 0U) ? TRUE : FALSE;
    Os_SchedTables[ScheduleTableID].NextExpiryIndex = 0U;
    Os_SchedTables[ScheduleTableID].State = OS_SCHED_TABLE_RUNNING;

    return E_OS_OK;
}

StatusType StopScheduleTable(ScheduleTableType ScheduleTableID)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_ConfigPtr == NULL) || (ScheduleTableID >= Os_ConfigPtr->NumScheduleTables))
    {
        Os_ReportDetError(OS_SID_STOP_SCHED_TABLE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        Os_ReportDetError(OS_SID_STOP_SCHED_TABLE, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_SchedTables[ScheduleTableID].State != OS_SCHED_TABLE_RUNNING)
    {
        return E_OS_NOFUNC;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_SCHEDULETABLE, (uint32)ScheduleTableID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    Os_SchedTables[ScheduleTableID].State = OS_SCHED_TABLE_STOPPED;
    Os_SchedTables[ScheduleTableID].OffsetElapsed = FALSE;
    Os_SchedTables[ScheduleTableID].NextExpiryIndex = 0U;
    Os_SchedTables[ScheduleTableID].NextPending = FALSE;
    Os_SchedTables[ScheduleTableID].NextTableID = (ScheduleTableType)0U;
    return E_OS_OK;
}

StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start)
{
    CounterType ctr;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_ConfigPtr == NULL) || (ScheduleTableID >= Os_ConfigPtr->NumScheduleTables))
    {
        Os_ReportDetError(OS_SID_START_SCHED_TABLE_ABS, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        Os_ReportDetError(OS_SID_START_SCHED_TABLE_ABS, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if (Os_SchedTables[ScheduleTableID].Config == NULL)
    {
        return E_OS_ID;
    }

    ctr = Os_SchedTables[ScheduleTableID].Config->CounterRef;
    if ((Os_ConfigPtr == NULL) || (ctr >= Os_ConfigPtr->NumCounters))
    {
        return E_OS_ID;
    }

    if (Start > Os_Counters[ctr].MaxAllowedValue)
    {
        return E_OS_VALUE;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_SCHEDULETABLE, (uint32)ScheduleTableID) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    if (Os_SchedTables[ScheduleTableID].State == OS_SCHED_TABLE_RUNNING)
    {
        return E_OS_STATE;
    }

    Os_SchedTables[ScheduleTableID].StartTick = Start;
    Os_SchedTables[ScheduleTableID].OffsetElapsed = FALSE;
    Os_SchedTables[ScheduleTableID].NextExpiryIndex = 0U;
    Os_SchedTables[ScheduleTableID].NextPending = FALSE;
    Os_SchedTables[ScheduleTableID].NextTableID = (ScheduleTableType)0U;
    Os_SchedTables[ScheduleTableID].State = OS_SCHED_TABLE_RUNNING;

    return E_OS_OK;
}

StatusType NextScheduleTable(ScheduleTableType ScheduleTableID_From, ScheduleTableType ScheduleTableID_To)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_ConfigPtr == NULL) ||
        (ScheduleTableID_From >= Os_ConfigPtr->NumScheduleTables) ||
        (ScheduleTableID_To >= Os_ConfigPtr->NumScheduleTables))
    {
        Os_ReportDetError(OS_SID_NEXT_SCHED_TABLE, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_Started == FALSE)
    {
        Os_ReportDetError(OS_SID_NEXT_SCHED_TABLE, OS_E_STATE);
        return E_OS_STATE;
    }
#endif

    if ((Os_SchedTables[ScheduleTableID_From].Config == NULL) ||
        (Os_SchedTables[ScheduleTableID_To].Config == NULL))
    {
        return E_OS_ID;
    }

    if (Os_SchedTables[ScheduleTableID_From].State != OS_SCHED_TABLE_RUNNING)
    {
        return E_OS_NOFUNC;
    }

    if (Os_SchedTables[ScheduleTableID_To].State == OS_SCHED_TABLE_RUNNING)
    {
        return E_OS_STATE;
    }

    if (Os_SchedTables[ScheduleTableID_From].Config->CounterRef !=
        Os_SchedTables[ScheduleTableID_To].Config->CounterRef)
    {
        return E_OS_ID;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_SCHEDULETABLE, (uint32)ScheduleTableID_From) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }
    if (Os_CheckCurrentApplicationObjectAccess(OS_OBJECT_SCHEDULETABLE, (uint32)ScheduleTableID_To) != E_OS_OK)
    {
        return E_OS_ACCESS;
    }

    Os_SchedTables[ScheduleTableID_From].NextPending = TRUE;
    Os_SchedTables[ScheduleTableID_From].NextTableID = ScheduleTableID_To;
    return E_OS_OK;
}

AppModeType GetActiveApplicationMode(void)
{
    return Os_ActiveAppMode;
}

StatusType Schedule(void)
{
#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_MAIN_FUNCTION, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
#endif

    if ((Os_CurrentTask != INVALID_TASK) && (Os_Tasks[Os_CurrentTask].ResourceCount > 0U))
    {
        return E_OS_RESOURCE;
    }

    Os_DispatchTasks();
    return E_OS_OK;
}

void Os_SetStartupHook(Os_HookFunctionType Hook)
{
    Os_StartupHook = Hook;
}

void Os_SetShutdownHook(Os_HookFunctionType Hook)
{
    Os_ShutdownHook = Hook;
}

void Os_SetErrorHook(Os_HookFunctionType Hook)
{
    Os_ErrorHook = Hook;
}

void Os_SetPreTaskHook(Os_HookFunctionType Hook)
{
    Os_PreTaskHook = Hook;
}

void Os_SetPostTaskHook(Os_HookFunctionType Hook)
{
    Os_PostTaskHook = Hook;
}

void Os_SetProtectionHook(Os_HookFunctionType Hook)
{
    Os_ProtectionHook = Hook;
}

void Os_MainFunction(void)
{
    uint32 nowMs;
    uint32 elapsed;

#if (OS_DEV_ERROR_DETECT == STD_ON)
    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_MAIN_FUNCTION, OS_E_UNINIT);
        return;
    }
#endif

    /* Compute elapsed ticks from platform timer */
    nowMs = Os_GetSystemTimeMs();
    if (nowMs >= Os_LastTickMs)
    {
        elapsed = nowMs - Os_LastTickMs;
    }
    else
    {
        /* Wrap-around */
        elapsed = (0xFFFFFFFFU - Os_LastTickMs) + nowMs + 1U;
    }

    /* Increment system counter for each elapsed ms */
    while (elapsed >= OS_TICK_DURATION_MS)
    {
        if ((Os_ConfigPtr != NULL) && (Os_ConfigPtr->NumCounters > OS_SYSTEM_COUNTER_ID))
        {
            (void)IncrementCounter(OS_SYSTEM_COUNTER_ID);
        }
        elapsed -= OS_TICK_DURATION_MS;
        Os_LastTickMs += OS_TICK_DURATION_MS;
    }

    /* Dispatch ready tasks by priority */
    Os_DispatchTasks();
}

void DisableAllInterrupts(void)
{
    Os_GlobalInterruptDisabled = TRUE;
}

void EnableAllInterrupts(void)
{
    Os_GlobalInterruptDisabled = FALSE;
}

void SuspendAllInterrupts(void)
{
    if (Os_AllInterruptSuspendNesting < 255U)
    {
        Os_AllInterruptSuspendNesting++;
    }
}

void ResumeAllInterrupts(void)
{
    if (Os_AllInterruptSuspendNesting > 0U)
    {
        Os_AllInterruptSuspendNesting--;
    }
}

void SuspendOSInterrupts(void)
{
    if (Os_OsInterruptSuspendNesting < 255U)
    {
        Os_OsInterruptSuspendNesting++;
    }
}

void ResumeOSInterrupts(void)
{
    if (Os_OsInterruptSuspendNesting > 0U)
    {
        Os_OsInterruptSuspendNesting--;
    }
}

ISRType GetISRID(void)
{
    return Os_CurrentIsr;
}

ApplicationType GetApplicationID(void)
{
    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        return INVALID_OSAPPLICATION;
    }

    return Os_CurrentApplication;
}

StatusType CheckObjectAccess(ApplicationType ApplID, Os_ObjectType_Type ObjectType, uint32 ObjectID)
{
    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_CHECK_OBJECT_ACCESS, OS_E_UNINIT);
        Os_ReportProtectionViolation();
        return E_OS_ACCESS;
    }

    if (ApplID >= OS_MAX_APPLICATIONS)
    {
        Os_ReportDetError(OS_SID_CHECK_OBJECT_ACCESS, OS_E_PARAM_ID);
        Os_ReportProtectionViolation();
        return E_OS_ID;
    }

    switch (ObjectType)
    {
        case OS_OBJECT_TASK:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumTasks))
            {
                return E_OS_ID;
            }
            if (Os_TaskOwnerApp[ObjectID] == ApplID)
            {
                return E_OS_OK;
            }
            Os_ReportProtectionViolation();
            return E_OS_ACCESS;

        case OS_OBJECT_ALARM:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumAlarms))
            {
                return E_OS_ID;
            }
            if (Os_AlarmOwnerApp[ObjectID] == ApplID)
            {
                return E_OS_OK;
            }
            Os_ReportProtectionViolation();
            return E_OS_ACCESS;

        case OS_OBJECT_COUNTER:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumCounters))
            {
                return E_OS_ID;
            }
            if (Os_CounterOwnerApp[ObjectID] == ApplID)
            {
                return E_OS_OK;
            }
            Os_ReportProtectionViolation();
            return E_OS_ACCESS;

        case OS_OBJECT_RESOURCE:
            if (ObjectID >= OS_MAX_RESOURCES)
            {
                return E_OS_ID;
            }
            if (Os_ResourceOwnerApp[ObjectID] == ApplID)
            {
                return E_OS_OK;
            }
            Os_ReportProtectionViolation();
            return E_OS_ACCESS;

        case OS_OBJECT_SCHEDULETABLE:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumScheduleTables))
            {
                return E_OS_ID;
            }
            if (Os_SchedTableOwnerApp[ObjectID] == ApplID)
            {
                return E_OS_OK;
            }
            Os_ReportProtectionViolation();
            return E_OS_ACCESS;

        default:
            return E_OS_ID;
    }
}

StatusType TerminateApplication(ApplicationType Application, RestartType RestartOption)
{
    AppModeType activeMode;

    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_TERMINATE_APPLICATION, OS_E_UNINIT);
        Os_ReportProtectionViolation();
        return E_OS_ACCESS;
    }

    if (Application >= OS_MAX_APPLICATIONS)
    {
        Os_ReportDetError(OS_SID_TERMINATE_APPLICATION, OS_E_PARAM_ID);
        Os_ReportProtectionViolation();
        return E_OS_ID;
    }

    if ((Application != Os_CurrentApplication) && (Os_CurrentTask != INVALID_TASK))
    {
        Os_ReportProtectionViolation();
        return E_OS_ACCESS;
    }

    activeMode = Os_ActiveAppMode;
    ShutdownOS(E_OS_OK);

    if (RestartOption == RESTART)
    {
        StartOS(activeMode);
    }

    return E_OS_OK;
}

StatusType CallTrustedFunction(TrustedFunctionIndexType FunctionIndex, TrustedFunctionParameterRefType FunctionParams)
{
    ApplicationType callerApplication;

    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_CALL_TRUSTED_FUNCTION, OS_E_UNINIT);
        Os_ReportProtectionViolation();
        return E_OS_ACCESS;
    }

    if (FunctionIndex >= OS_MAX_TRUSTED_FUNCTIONS)
    {
        Os_ReportDetError(OS_SID_CALL_TRUSTED_FUNCTION, OS_E_PARAM_ID);
        Os_ReportProtectionViolation();
        return E_OS_ID;
    }

    if (Os_TrustedFunctions[FunctionIndex] == NULL)
    {
        return E_OS_NOFUNC;
    }

    callerApplication = Os_CurrentApplication;
    Os_CurrentApplication = (ApplicationType)0U;
    if (Os_TrustedCallNesting < 255U)
    {
        Os_TrustedCallNesting++;
    }
    Os_TrustedFunctions[FunctionIndex](FunctionParams);
    if (Os_TrustedCallNesting > 0U)
    {
        Os_TrustedCallNesting--;
    }
    Os_CurrentApplication = callerApplication;
    return E_OS_OK;
}

StatusType Os_RegisterTrustedFunction(TrustedFunctionIndexType FunctionIndex, Os_TrustedFunctionType FunctionPtr)
{
    if (FunctionIndex >= OS_MAX_TRUSTED_FUNCTIONS)
    {
        return E_OS_ID;
    }

    Os_TrustedFunctions[FunctionIndex] = FunctionPtr;
    return E_OS_OK;
}

StatusType Os_EnterISR(ISRType IsrID, ApplicationType OwnerApplication)
{
    if ((Os_Initialized == FALSE) || (Os_Started == FALSE))
    {
        Os_ReportDetError(OS_SID_ENTER_ISR, OS_E_UNINIT);
        return E_OS_ACCESS;
    }
    if ((Os_GlobalInterruptDisabled != FALSE) ||
        (Os_AllInterruptSuspendNesting > 0U) ||
        (Os_OsInterruptSuspendNesting > 0U))
    {
        return E_OS_STATE;
    }
    if ((OwnerApplication >= OS_MAX_APPLICATIONS) || (IsrID == INVALID_ISR))
    {
        Os_ReportDetError(OS_SID_ENTER_ISR, OS_E_PARAM_ID);
        return E_OS_ID;
    }
    if (Os_CurrentIsr != INVALID_ISR)
    {
        return E_OS_STATE;
    }

    Os_CurrentIsr = IsrID;
    Os_CurrentApplication = OwnerApplication;
    return E_OS_OK;
}

void Os_ExitISR(void)
{
    if (Os_CurrentIsr != INVALID_ISR)
    {
        Os_CurrentIsr = INVALID_ISR;
        if (Os_CurrentTask != INVALID_TASK)
        {
            Os_CurrentApplication = Os_TaskOwnerApp[Os_CurrentTask];
        }
        else
        {
            Os_CurrentApplication = (ApplicationType)0U;
        }
    }
}

StatusType Os_AssignObjectToApplication(Os_ObjectType_Type ObjectType, uint32 ObjectID, ApplicationType ApplID)
{
    if (ApplID >= OS_MAX_APPLICATIONS)
    {
        Os_ReportDetError(OS_SID_ASSIGN_OBJECT_APP, OS_E_PARAM_ID);
        return E_OS_ID;
    }

    switch (ObjectType)
    {
        case OS_OBJECT_TASK:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumTasks))
            {
                return E_OS_ID;
            }
            Os_TaskOwnerApp[ObjectID] = ApplID;
            return E_OS_OK;

        case OS_OBJECT_ALARM:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumAlarms))
            {
                return E_OS_ID;
            }
            Os_AlarmOwnerApp[ObjectID] = ApplID;
            return E_OS_OK;

        case OS_OBJECT_COUNTER:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumCounters))
            {
                return E_OS_ID;
            }
            Os_CounterOwnerApp[ObjectID] = ApplID;
            return E_OS_OK;

        case OS_OBJECT_RESOURCE:
            if (ObjectID >= OS_MAX_RESOURCES)
            {
                return E_OS_ID;
            }
            Os_ResourceOwnerApp[ObjectID] = ApplID;
            return E_OS_OK;

        case OS_OBJECT_SCHEDULETABLE:
            if ((Os_ConfigPtr == NULL) || (ObjectID >= Os_ConfigPtr->NumScheduleTables))
            {
                return E_OS_ID;
            }
            Os_SchedTableOwnerApp[ObjectID] = ApplID;
            return E_OS_OK;

        default:
            return E_OS_ID;
    }
}
