#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include <pthread.h>
#include <ucontext.h>

/* MISRA C:2012 Rule 17.3 - pthread prototype for static analysis when system headers are unavailable */
#ifndef _PTHREAD_H
typedef unsigned long int pthread_t;
extern int pthread_create(pthread_t* thread, const void* attr, void*(*start_routine)(void*), void* arg);
#endif

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define NUM_FUNCTIONS 3
#define STACK_SIZE 1152
#define SCHEDULER_SECOC_HANDLED_BY_ECUM 1


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void EthernetRecieveFn(void);
void RecieveMainFunctions(void);
void TxMainFunctions(void);
void start_task(int i);
void scheduler_handler(int signum);
void Scheduler_Start(void);

#endif