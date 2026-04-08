/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#ifdef WINDOWS
    #include "Ethernet/ethernet_windows.h"
#else
    #include "Ethernet/ethernet.h"
#endif
#include "Can/CanIF.h"
#include "Can/CanTP.h"
#include "PduR/PduR_CanIf.h"
#include "SecOC/SecOC_Cfg.h"
#include "SecOC/SecOC_Debug.h"
#include "SecOC/SecOC_Lcfg.h"
#include "SoAd/SoAd.h"

#ifdef WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
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
/* cppcheck-suppress misra-c2012-5.9 */
static char ip_address_send[15] = "127.0.0.1";
/* PdusCollections declared in SecOC_Lcfg.h */

#ifdef WINDOWS
static WSADATA wsa_data;
static boolean winsock_initialized = FALSE;
#endif

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

#ifdef WINDOWS
static Std_ReturnType winsock_init(void)
{
    if (!winsock_initialized)
    {
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            #ifdef ETHERNET_DEBUG
                (void)printf("WSAStartup failed\n");
            #endif
            return E_NOT_OK;
        }
        winsock_initialized = TRUE;
    }
    return E_OK;
}

static void winsock_cleanup(void)
{
    if (winsock_initialized)
    {
        WSACleanup();
        winsock_initialized = FALSE;
    }
}
#endif

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
void EthDrv_Init(void)
{
    #ifdef WINDOWS
    /* Initialize Winsock */
    if (winsock_init() != E_OK)
    {
        return;
    }
    #endif

    uint8 ip_address_read[16];
    /* Open the file containing the IP address */
    FILE* fp = fopen("./source/Ethernet/ip_address.txt", "r");
    if (fp == NULL)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Error opening file, using default IP: %s\n", ip_address_send);
        #endif
        return;
    }

    /* Read the IP address from the file */
    (void)fgets(ip_address_read, 16, fp);

    /* Close the file */
    (void)fclose(fp);

    #ifdef ETHERNET_DEBUG
        (void)printf("IP is %s\n", ip_address_read);
    #endif

    /* Copy the IP address to the global variable */
    if (strlen(ip_address_read) > 0U)
    {
        ip_address_read[strcspn(ip_address_read, "\n")] = 0;
        (void)strcpy(ip_address_send, ip_address_read);
    }
}

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
Std_ReturnType EthDrv_Send(unsigned short id, unsigned char* data, uint16 dataLen)
{
    #ifdef ETHERNET_DEBUG
        (void)printf("######## in Sent Ethernet\n");
    #endif

    #ifdef WINDOWS
    /* Ensure Winsock is initialized */
    if (winsock_init() != E_OK)
    {
        return E_NOT_OK;
    }
    #endif

    /* Create a socket */
    SOCKET network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (network_socket == INVALID_SOCKET)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Create Socket Error: %d\n", WSAGetLastError());
        #endif
        return E_NOT_OK;
    }

    /* Specify an address for the socket */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    inet_pton(AF_INET, ip_address_send, &server_address.sin_addr);

    int connection_status = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    if (connection_status == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Connection Error: %d\n", WSAGetLastError());
        #endif
        closesocket(network_socket);
        return E_NOT_OK;
    }

    /* Prepare For Send */
    uint8 sendData[(uint16)BUS_LENGTH_RECEIVE + (uint16)sizeof(id)];
    (void)memcpy(sendData, data, dataLen);
    for (unsigned char indx = 0; indx < sizeof(id); indx++)
    {
        sendData[dataLen + indx] = (uint8)(id >> (8U * (unsigned int)indx));
    }

    #ifdef ETHERNET_DEBUG
        (void)printf("Sending %d bytes\n", dataLen + sizeof(id));
        for (uint16 j = 0U; j < (uint16)dataLen + (uint16)sizeof(id) && j < 20U; j++)
        {
            (void)printf("%d\t", sendData[j]);
        }
        (void)printf("\n");
    #endif

    send(network_socket, (const char*)sendData, dataLen + sizeof(id), 0);

    /* Close the connection */
    closesocket(network_socket);
    return E_OK;
}

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
Std_ReturnType EthDrv_Receive(unsigned char* data, uint16 dataLen, unsigned short* id, uint16* actualSize)
{
    #ifdef ETHERNET_DEBUG
        (void)printf("######## in Receive Ethernet\n");
    #endif

    #ifdef WINDOWS
    /* Ensure Winsock is initialized */
    if (winsock_init() != E_OK)
    {
        return E_NOT_OK;
    }
    #endif

    /* Create a socket */
    SOCKET server_socket;
    SOCKET client_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Create Socket Error: %d\n", WSAGetLastError());
        #endif
        return E_NOT_OK;
    }

    /* Specify an address for the socket */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /* Set socket options for reuse */
    BOOL opt = TRUE;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("setsockopt Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    /* Bind the socket to our specified IP and Port */
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Bind Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Listen Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Accept Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    /* Receive data */
    unsigned char recData[dataLen + sizeof(unsigned short)];
    int recv_result = recv(client_socket, (char*)recData, (dataLen + sizeof(unsigned short)), 0);

    if (recv_result == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            (void)printf("Receive Error: %d\n", WSAGetLastError());
        #endif
        closesocket(client_socket);
        closesocket(server_socket);
        return E_NOT_OK;
    }

    /* Calculate actual PDU size (received bytes - ID size) */
    int actualPduSize = recv_result - (int)sizeof(unsigned short);

    #ifdef ETHERNET_DEBUG
        (void)printf("in Receive Ethernet \t");
        (void)printf("Received %d bytes total, PDU size = %d\n", recv_result, actualPduSize);
        (void)printf("Info Received (first 20 bytes): \n");
        for (int j = 0; (j < recv_result) && (j < 20); j++)
        {
            (void)printf("%d ", recData[j]);
        }
        (void)printf("\n\n\n");
    #endif

    /* Extract ID from END of received data (not from fixed buffer position!) */
    /* cppcheck-suppress misra-c2012-18.4 */
    /* cppcheck-suppress misra-c2012-21.15 */
    (void)memcpy(id, &recData[actualPduSize], sizeof(unsigned short));
    (void)memcpy(data, recData, actualPduSize);

    /* Return actual PDU size to caller */
    if (actualSize != NULL)
    {
        *actualSize = (uint16)actualPduSize;
    }

    #ifdef ETHERNET_DEBUG
        (void)printf("id = %d \n", *id);
    #endif

    /* Close the socket */
    closesocket(client_socket);
    closesocket(server_socket);
    return E_OK;
}

/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */
void EthDrv_ReceiveMainFunction(void)
{
    /* Legacy direct Ethernet->PduR path is permanently disabled. */
    (void)ETHERNET_LEGACY_DIRECT_ROUTING;
    return;
}
