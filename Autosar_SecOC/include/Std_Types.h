/******************************************************************************
 * File Name   : Std_Types                                                    *
 * Description : Header file of used Common Standard Types                    *
 * Created on  : April 6, 2020                                                *
 ******************************************************************************/
 
#ifndef INCLUDE_STD_TYPES_H_
#define INCLUDE_STD_TYPES_H_

/********************************************************************************************************/
/*******************************************DefinesAndTypedef********************************************/
/********************************************************************************************************/

/* Convert the bits to nearst byte */
#define BIT_TO_BYTES(NUMBITS) (((NUMBITS) % 8U == 0U) ? ((NUMBITS)/8U) : (((NUMBITS) / 8U) + 1U))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* Boolean Values */
#ifndef FALSE
#define FALSE       (0u)
#endif
#ifndef TRUE
#define TRUE        (1u)
#endif

#define HIGH        (1u)
#define LOW         (0u)


/* Boolean Data Type */
typedef unsigned char           boolean;
typedef unsigned char           uint8;          /*           0 .. 255             */
typedef unsigned short          uint16;         /*           0 .. 65535           */
typedef unsigned long           uint32;         /*           0 .. 4294967295      */
typedef signed char             sint8;          /*        -128 .. +127            */
typedef signed short            sint16;         /*      -32768 .. +32767          */
typedef signed long             sint32;         /* -2147483648 .. +2147483647     */
/* cppcheck-suppress misra-c2012-2.3 */
typedef signed long long        sint64;
/* cppcheck-suppress misra-c2012-2.3 */
typedef unsigned long long      uint64;         /*       0..18446744073709551615  */
/* cppcheck-suppress misra-c2012-2.3 */
typedef float                   float32;
typedef double                  float64;



#include <stddef.h>
#ifndef NULL
/* NULL is defined in stddef.h */
#endif

typedef uint8 Std_ReturnType;
#define E_OK            ((Std_ReturnType)0x00)
#define E_NOT_OK        ((Std_ReturnType)0x01)
#define E_BUSY          ((Std_ReturnType)0x02)
#define QUEUE_FULL      ((Std_ReturnType)0x03)

/* Standard Version Info Type */
typedef struct
{
	uint16 vendorID;
	uint16 moduleID;
	uint8 sw_major_version;
	uint8 sw_minor_version;
	uint8 sw_patch_version;
}Std_VersionInfoType;

#define STD_ON 0x01u
#define STD_OFF 0x00u

#endif /* INCLUDE_STD_TYPES_H_ */
