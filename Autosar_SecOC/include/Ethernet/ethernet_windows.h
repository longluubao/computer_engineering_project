#ifndef ETHERNET_H__
#define ETHERNET_H__

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include <stdlib.h>
#include "Std_Types.h"
#include <string.h>

#ifdef WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

/* MISRA C:2012 Rule 17.3 - Socket prototypes for static analysis when system headers are unavailable */
#if !defined(_SYS_SOCKET_H) && !defined(__SYS_SOCKET_H__) && !defined(_WINSOCK2API_)
struct sockaddr;
extern int setsockopt(int, int, int, const void*, unsigned int);
extern int bind(int, const struct sockaddr*, unsigned int);
extern int listen(int, int);
extern int connect(int, const struct sockaddr*, unsigned int);
extern int getsockname(int, struct sockaddr*, unsigned int*);
#endif

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define PORT_NUMBER 12345
#define BUS_LENGTH_RECEIVE 4096  // Increased for PQC signatures (3309 bytes)
#define ETHERNET_LEGACY_DIRECT_ROUTING STD_OFF


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_init                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to get the ip that       *
 * send data to and initialize Winsock                  *
 *******************************************************/
void EthDrv_Init(void);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_send                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to send the data using   *
 * Winsock sockets                                      *
 *******************************************************/
Std_ReturnType EthDrv_Send(unsigned short id, unsigned char* data, uint16 dataLen);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_receive              *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to Receive the data using*
 * Winsock sockets                                      *
 * @param actualSize [out] Actual number of bytes received*
 *******************************************************/
Std_ReturnType EthDrv_Receive(unsigned char* data, uint16 dataLen, unsigned short* id, uint16* actualSize);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_RecieveMainFunction  *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to route the data        *
 * Received to protocol                                 *
 *******************************************************/
void EthDrv_ReceiveMainFunction(void);

#endif
