/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "Scheduler/scheduler.h"

#include "Can/CanTP.h"
#include "Com/Com.h"
#include "Ethernet/ethernet.h"
#include "SecOC/SecOC.h"
#include "SecOC/SecOC_Debug.h"
#include "SoAd/SoAd.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

/* External API declarations (MISRA 8.4 visibility). */
void EthernetRecieveFn(void);
void RecieveMainFunctions(void);
void TxMainFunctions(void);
void start_task(int i);
void scheduler_handler(int signum);
void Scheduler_Start(void);


/********************************************************************************************************/
/********************************************GlobalVaribles**********************************************/
/********************************************************************************************************/

static char stacks[NUM_FUNCTIONS][STACK_SIZE];

static struct task_state {
    void (*function)(void);
    char *stack;
    int state;
    ucontext_t context; // Store the context for each task
}task_state;

static struct task_state tasks[NUM_FUNCTIONS];
static boolean once = FALSE;
static pthread_t t;


void EthernetRecieveFn() {
    while (1) {
        tasks[0].state++;

        if ( once == FALSE)
        {
            if(pthread_create(&t , NULL, (void *)&ethernet_RecieveMainFunction, NULL) != 0)
            {
                #ifdef SCHEDULER_DEBUG
                printf("error create thread");
                #endif
                return;
            }
        }
        swapcontext(&tasks[0].context, &tasks[1].context);
    }
}

void RecieveMainFunctions() {
    while (1) {
        tasks[1].state++;
        SoAd_MainFunctionRx();
#if (SCHEDULER_SECOC_HANDLED_BY_ECUM == 0)
        SecOC_MainFunctionRx();
#endif
        CanTp_MainFunctionRx();
        swapcontext(&tasks[1].context, &tasks[2].context);
    }
}

void TxMainFunctions() 
{
    while (1) 
    {
        tasks[2].state++;
        Com_MainTx();
#if (SCHEDULER_SECOC_HANDLED_BY_ECUM == 0)
        SecOC_MainFunctionTx();
#endif
        SoAd_MainFunctionTx();
        CanTp_MainFunctionTx();
        swapcontext(&tasks[2].context, &tasks[0].context);
    }
}

void start_task(int i) {
    tasks[i].function();
}

void scheduler_handler(int signum) {
    // Not used
}

void Scheduler_Start()
{
    int i;
    struct sigaction sa;
    struct itimerval timer;

    void (*functions[NUM_FUNCTIONS])(void) = {EthernetRecieveFn, RecieveMainFunctions, TxMainFunctions};

    // Initialize task states
    for (i = 0; i < NUM_FUNCTIONS; i++) {
        tasks[i].function = functions[i];
        /* cppcheck-suppress misra-c2012-18.4 ; pointer arithmetic required for stack top */
        tasks[i].stack = stacks[i] + STACK_SIZE;
        tasks[i].state = 0;
        getcontext(&tasks[i].context);
        tasks[i].context.uc_stack.ss_sp = tasks[i].stack;
        tasks[i].context.uc_stack.ss_size = STACK_SIZE;
        tasks[i].context.uc_link = NULL;
        makecontext(&tasks[i].context, (void (*)(void)) start_task, 1, i);
    }

    // Set up timer signal for scheduler
    sa.sa_handler = scheduler_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    // Configure and start timer for scheduler
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 900000; // 0.5 second
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 900000; //

    setitimer(ITIMER_REAL, &timer, NULL);
    tasks[0].state = 1; // Set initial state for Function 1 to 1
    int x = 0;
    swapcontext(&x, &tasks[0].context); // Start with Function

    // Wait for signals
    while (1) {
        pause();
    }

}