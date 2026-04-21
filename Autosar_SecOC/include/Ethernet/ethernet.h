#ifndef SENDER_H__
#define SENDER_H__

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Std_Types.h"
#include <string.h>

/* MISRA C:2012 Rule 17.3 - Socket prototypes for static analysis when system headers are unavailable */
#if !defined(_SYS_SOCKET_H) && !defined(__SYS_SOCKET_H__) && !defined(_WINSOCK2API_)
struct sockaddr;
extern int setsockopt(int sockfd, int level, int optname, const void* optval, unsigned int optlen);
extern int bind(int sockfd, const struct sockaddr* addr, unsigned int addrlen);
extern int listen(int sockfd, int backlog);
extern int connect(int sockfd, const struct sockaddr* addr, unsigned int addrlen);
extern int getsockname(int sockfd, struct sockaddr* addr, unsigned int* addrlen);
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
 * send data to                                         *
 *******************************************************/
void EthDrv_Init(void);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_send                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to send the data using   *
 * Sockets                                              *
 *******************************************************/
Std_ReturnType EthDrv_Send(unsigned short id, unsigned char* data , uint16 dataLen);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_receive              *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to Receive the data using*
 * Sockets                                              *
 * @param actualSize [out] Actual number of bytes received*
 *******************************************************/
Std_ReturnType EthDrv_Receive(unsigned char* data , uint16 dataLen, unsigned short* id, uint16* actualSize);


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
