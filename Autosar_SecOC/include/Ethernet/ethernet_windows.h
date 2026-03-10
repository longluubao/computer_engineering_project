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

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define PORT_NUMBER 12345
#define BUS_LENGTH_RECEIVE 4096  // Increased for PQC signatures (3309 bytes)


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
void ethernet_init(void);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_send                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to send the data using   *
 * Winsock sockets                                      *
 *******************************************************/
Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, uint16 dataLen);


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
Std_ReturnType ethernet_receive(unsigned char* data, uint16 dataLen, unsigned short* id, uint16* actualSize);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_RecieveMainFunction  *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to route the data        *
 * Received to protocol                                 *
 *******************************************************/
void ethernet_RecieveMainFunction(void);

#endif
