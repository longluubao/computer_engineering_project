
#include "EcuM/EcuM.h"
#include "Os/Os.h"
#include "SecOC/SecOC.h"
#include "SecOC/SecOC_Debug.h"
#include <stdio.h>

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType EcuM_StartupTwo(void);

#ifdef DEBUG_ALL
void SecOC_test(void);
#endif

int main(void)
{
    EcuM_Init(NULL);
    if (EcuM_StartupTwo() != E_OK)
    {
        (void)printf("EcuM startup failed\n");
        return 1;
    }

    Os_Init(&Os_Config);
    Os_SetStartupHook(Os_GatewayStartupHook);
    StartOS(OSDEFAULTAPPMODE);

    #ifdef DEBUG_ALL
        SecOC_test();
    #endif

    (void)printf("Program entered RUN state (BSW/MCAL startup complete)\n");

    for (;;)
    {
        Os_MainFunction();
    }
}