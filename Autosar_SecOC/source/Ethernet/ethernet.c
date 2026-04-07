/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Can/CanIF.h"
#include "Can/CanTP.h"
#include "Ethernet/ethernet.h"
#include "PduR/PduR_CanIf.h"
#include "SecOC/SecOC_Cfg.h"
#include "SecOC/SecOC_Debug.h"
#include "SecOC/SecOC_Lcfg.h"
#include "SoAd/SoAd.h"
#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif 


/* MISRA C:2012 Rule 8.4 - Forward declarations for external linkage functions */
extern void EthDrv_Init(void);
extern Std_ReturnType EthDrv_Send(unsigned short id, unsigned char* data, uint16 dataLen);
extern Std_ReturnType EthDrv_Receive(unsigned char* data, uint16 dataLen, unsigned short* id, uint16* actualSize);
extern void EthDrv_ReceiveMainFunction(void);

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

/* cppcheck-suppress misra-c2012-7.4 */
static char ip_address_send[15] = "127.0.0.1";
/* PdusCollections declared in SecOC_Lcfg.h */
#ifdef SCHEDULER_ON
    pthread_mutex_t lock;
#endif 

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
void EthDrv_Init(void) 
{

    uint8 ip_address_read[16];
    /* Open the file containing the IP address */
    FILE* fp = fopen("./source/Ethernet/ip_address.txt", "r");
    if (fp == NULL) 
    {
        #ifdef ETHERNET_DEBUG
            printf("Error opening file\n");
        #endif
        return;
    }
    
    /* Read the IP address from the file */
    (void)fgets(ip_address_read, 16, fp);

    /* Close the file */
    (void)fclose(fp);

    #ifdef ETHERNET_DEBUG
        printf("IP is %s\n", ip_address_read);
    #endif

    /* Copy the IP address to the global variable */
    if (strlen(ip_address_read) > 0U) 
    {
        ip_address_read[strcspn(ip_address_read, "\n")] = 0;
        (void)strcpy(ip_address_send, ip_address_read);
    }
}

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
Std_ReturnType EthDrv_Send(unsigned short id, unsigned char* data , uint16 dataLen) {
    #ifdef ETHERNET_DEBUG
        printf("######## in Sent Ethernet\n");
    #endif
    /* create a socket*/
    int network_sockect;
    network_sockect = socket(AF_INET , SOCK_STREAM , 0);
    if (network_sockect < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Create Socket Error\n");
        #endif
        return E_NOT_OK;
    }

    /* specify an address for the socket*/
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = inet_addr(ip_address_send);

    int connection_status = connect(network_sockect , (struct sockaddr* ) &server_address , sizeof(server_address) );

    if (connection_status != 0) {
        #ifdef ETHERNET_DEBUG
            printf("Connection Error\n");
        #endif
        return E_NOT_OK;

    }

    /* Prepare For Send */
    uint8 sendData[BUS_LENGTH_RECEIVE + sizeof(id)];
    (void)memcpy(sendData, data, dataLen);
    for(unsigned char indx = 0; indx < sizeof(id); indx++)
    {
        sendData[dataLen+indx] = (id >> (8 * indx));
    }


    #ifdef ETHERNET_DEBUG
        printf("Sending %d bytes\n", dataLen + sizeof(id));
        for(int j = 0; j < dataLen + sizeof(id) && j < 20 ; j++)
        {
            printf("%d\t",sendData[j]);
        }
        printf("\n");
    #endif



    send(network_sockect , sendData , dataLen + sizeof(id) , 0);

    /* close the connection*/
    close(network_sockect);
    return E_OK;

}

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
Std_ReturnType EthDrv_Receive(unsigned char* data , uint16 dataLen, unsigned short* id, uint16* actualSize)
{

    #ifdef ETHERNET_DEBUG
        printf("######## in Recieve Ethernet\n");
    #endif
    /* create a socket*/
    int server_socket;
    int client_socket;
    server_socket = socket(AF_INET , SOCK_STREAM , 0);
    if (server_socket < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Create Socket Error\n");
        #endif
        return E_NOT_OK;

    }

    /* specify an address for the socket*/
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = INADDR_ANY; /* inet_addr("192.168.1.2");*/

    
    int opt = 1; 
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        #ifdef ETHERNET_DEBUG
            printf("connect setsockopt Error \n"); 
        #endif
        return E_NOT_OK;

    }
    
    /* bind the socket to our specified IP and Port*/
   
    if ( ( bind(server_socket , (struct sockaddr* ) &server_address , sizeof(server_address) )) < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Bind Error\n");
        #endif
        return E_NOT_OK;

    }

    

    if ( (listen(server_socket , 5)) < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Listen Error\n");
        #endif
        return E_NOT_OK;

    }
   
    client_socket = accept(server_socket , NULL , NULL);
    if (client_socket < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Accept Error\n");
        #endif
        return E_NOT_OK;
    }
    /* Receive data*/
    unsigned char recData [dataLen + sizeof(unsigned short)];
    int recv_result = recv( client_socket , recData , (dataLen + sizeof(unsigned short)) , 0);

    if (recv_result < 0)
    {
        #ifdef ETHERNET_DEBUG
            printf("Receive Error\n");
        #endif
        close(server_socket);
        return E_NOT_OK;
    }

    /* Calculate actual PDU size (received bytes - ID size) */
    int actualPduSize = recv_result - (int)sizeof(unsigned short);

    #ifdef ETHERNET_DEBUG
        printf("in Recieve Ethernet \t");
        printf("Received %d bytes total, PDU size = %d\n", recv_result, actualPduSize);
        printf("Info Received (first 20 bytes): \n");
        for(int j  = 0 ; j < recv_result && j < 20 ; j++)
        {
            printf("%d ",recData[j]);
        }
        printf("\n\n\n");
    #endif


    #ifdef SCHEDULER_ON
        pthread_mutex_lock(&lock);
    #endif

    /* Extract ID from END of received data (not from fixed buffer position!) */
    /* cppcheck-suppress misra-c2012-18.4 ; pointer arithmetic required for buffer offset */
    (void)memcpy(id, recData + actualPduSize, sizeof(unsigned short));
    (void)memcpy(data, recData, actualPduSize);

    /* Return actual PDU size to caller */
    if (actualSize != NULL)
    {
        *actualSize = (uint16)actualPduSize;
    }

    #ifdef ETHERNET_DEBUG
        printf("id = %d \n",*id);
    #endif
    /* close the socket*/
    close(server_socket);
    return E_OK;

}


/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
void EthDrv_ReceiveMainFunction(void)
{
    /* Legacy direct Ethernet->PduR path is permanently disabled. */
    (void)ETHERNET_LEGACY_DIRECT_ROUTING;
    return;
}