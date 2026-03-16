#include "Os.h"
#include "EcuM.h"

static void Os_TaskBswCycle(void)
{
    EcuM_MainFunctionState();
    EcuM_MainFunctionComStack();
    EcuM_MainFunctionSecurityStack();
    EcuM_MainFunctionNetworkStack();
    EcuM_MainFunctionDiagnosticsStack();
    EcuM_MainFunctionNvStack();
    (void)TerminateTask();
}

static const Os_TaskConfigType Os_TaskConfigList[] =
{
    { Os_TaskBswCycle, 1U, OS_TASK_BASIC, FALSE }
};

static const Os_AlarmConfigType Os_AlarmConfigList[] =
{
    { OS_SYSTEM_COUNTER_ID, { OS_ALARM_ACTION_ACTIVATE_TASK, OS_TASK_ID_BSW_CYCLE, NULL } }
};

static const Os_CounterConfigType Os_CounterConfigList[] =
{
    { OS_SYSTEM_COUNTER_MAX_VALUE, OS_SYSTEM_COUNTER_TICKS_PER_BASE, OS_SYSTEM_COUNTER_MIN_CYCLE }
};

const Os_ConfigType Os_Config =
{
    Os_TaskConfigList,
    (uint8)(sizeof(Os_TaskConfigList) / sizeof(Os_TaskConfigList[0])),
    Os_AlarmConfigList,
    (uint8)(sizeof(Os_AlarmConfigList) / sizeof(Os_AlarmConfigList[0])),
    Os_CounterConfigList,
    (uint8)(sizeof(Os_CounterConfigList) / sizeof(Os_CounterConfigList[0])),
    NULL,
    0U
};

void Os_GatewayStartupHook(void)
{
    (void)SetRelAlarm((AlarmType)OS_ALARM_ID_BSW_CYCLE,
                      (TickType)1U,
                      (TickType)OS_BSW_CYCLE_PERIOD_TICKS);
}
