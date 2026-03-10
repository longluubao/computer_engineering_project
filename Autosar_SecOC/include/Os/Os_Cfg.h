/**
 * @file Os_Cfg.h
 * @brief AUTOSAR OS (OSEK) Configuration
 * @details Static configuration for the OS module.
 */

#ifndef OS_CFG_H
#define OS_CFG_H

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define OS_MAX_TASKS                ((uint8)8U)
#define OS_MAX_ALARMS               ((uint8)8U)
#define OS_MAX_COUNTERS             ((uint8)4U)
#define OS_MAX_RESOURCES            ((uint8)4U)
#define OS_MAX_SCHEDULE_TABLES      ((uint8)4U)
#define OS_MAX_EXPIRY_POINTS        ((uint8)8U)
#define OS_MAX_APPLICATIONS         ((uint8)2U)
#define OS_MAX_TRUSTED_FUNCTIONS    ((uint8)8U)

#define OS_SYSTEM_COUNTER_ID        ((uint8)0U)
#define OS_SYSTEM_COUNTER_MAX_VALUE ((uint32)0xFFFFFFFFUL)
#define OS_SYSTEM_COUNTER_TICKS_PER_BASE ((uint32)1U)
#define OS_SYSTEM_COUNTER_MIN_CYCLE ((uint32)1U)

#define OS_MAX_TASK_ACTIVATIONS     ((uint8)255U)

#define OS_TICK_DURATION_MS         (1U)

#define OS_DEV_ERROR_DETECT         (STD_ON)

#endif /* OS_CFG_H */
