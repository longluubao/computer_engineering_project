
#include "SecOC.h"
#include "SecOC_Debug.h"
#include "EcuM.h"

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

    #ifdef DEBUG_ALL
        SecOC_test();
    #endif

    (void)printf("Program ran successfully (BSW/MCAL startup complete)\n");
    return 0;
}