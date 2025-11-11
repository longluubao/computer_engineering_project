
#include "SecOC.h"
#include "SecOC_Debug.h"

#ifdef DEBUG_ALL
void SecOC_test(void);
#endif

int main(void)
{
    #ifdef DEBUG_ALL
        SecOC_test();
    #endif

    (void)printf("Program ran successfully\n");
    return 0;
}