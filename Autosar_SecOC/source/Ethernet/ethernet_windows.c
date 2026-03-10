/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#ifdef WINDOWS
    #include "ethernet_windows.h"
#else
    #include "ethernet.h"
#endif
#include "SecOC_Debug.h"
#include "SecOC_Lcfg.h"
#include "CanTP.h"
#include "CanIF.h"
#include "PduR_CanIf.h"
#include "SoAd.h"

#ifdef WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static uint8 ip_address_send[15] = "127.0.0.1";
extern SecOC_PduCollection PdusCollections[];

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
                printf("WSAStartup failed\n");
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

void ethernet_init(void)
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
            printf("Error opening file, using default IP: %s\n", ip_address_send);
        #endif
        return;
    }

    /* Read the IP address from the file */
    fgets(ip_address_read, 16, fp);

    /* Close the file */
    fclose(fp);

    #ifdef ETHERNET_DEBUG
        printf("IP is %s\n", ip_address_read);
    #endif

    /* Copy the IP address to the global variable */
    if (strlen(ip_address_read) > 0)
    {
        ip_address_read[strcspn(ip_address_read, "\n")] = 0;
        strcpy(ip_address_send, ip_address_read);
    }
}

Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, uint16 dataLen)
{
    #ifdef ETHERNET_DEBUG
        printf("######## in Sent Ethernet\n");
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
            printf("Create Socket Error: %d\n", WSAGetLastError());
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
            printf("Connection Error: %d\n", WSAGetLastError());
        #endif
        closesocket(network_socket);
        return E_NOT_OK;
    }

    /* Prepare For Send */
    uint8 sendData[BUS_LENGTH_RECEIVE + sizeof(id)];
    (void)memcpy(sendData, data, dataLen);
    for (unsigned char indx = 0; indx < sizeof(id); indx++)
    {
        sendData[dataLen + indx] = (id >> (8 * indx));
    }

    #ifdef ETHERNET_DEBUG
        printf("Sending %d bytes\n", dataLen + sizeof(id));
        for (int j = 0; j < dataLen + sizeof(id) && j < 20; j++)
            printf("%d\t", sendData[j]);
        printf("\n");
    #endif

    send(network_socket, (const char*)sendData, dataLen + sizeof(id), 0);

    /* Close the connection */
    closesocket(network_socket);
    return E_OK;
}

Std_ReturnType ethernet_receive(unsigned char* data, uint16 dataLen, unsigned short* id, uint16* actualSize)
{
    #ifdef ETHERNET_DEBUG
        printf("######## in Receive Ethernet\n");
    #endif

    #ifdef WINDOWS
    /* Ensure Winsock is initialized */
    if (winsock_init() != E_OK)
    {
        return E_NOT_OK;
    }
    #endif

    /* Create a socket */
    SOCKET server_socket, client_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET)
    {
        #ifdef ETHERNET_DEBUG
            printf("Create Socket Error: %d\n", WSAGetLastError());
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
            printf("setsockopt Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    /* Bind the socket to our specified IP and Port */
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            printf("Bind Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR)
    {
        #ifdef ETHERNET_DEBUG
            printf("Listen Error: %d\n", WSAGetLastError());
        #endif
        closesocket(server_socket);
        return E_NOT_OK;
    }

    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET)
    {
        #ifdef ETHERNET_DEBUG
            printf("Accept Error: %d\n", WSAGetLastError());
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
            printf("Receive Error: %d\n", WSAGetLastError());
        #endif
        closesocket(client_socket);
        closesocket(server_socket);
        return E_NOT_OK;
    }

    /* Calculate actual PDU size (received bytes - ID size) */
    int actualPduSize = recv_result - sizeof(unsigned short);

    #ifdef ETHERNET_DEBUG
        printf("in Receive Ethernet \t");
        printf("Received %d bytes total, PDU size = %d\n", recv_result, actualPduSize);
        printf("Info Received (first 20 bytes): \n");
        for (int j = 0; j < recv_result && j < 20; j++)
        {
            printf("%d ", recData[j]);
        }
        printf("\n\n\n");
    #endif

    /* Extract ID from END of received data (not from fixed buffer position!) */
    (void)memcpy(id, recData + actualPduSize, sizeof(unsigned short));
    (void)memcpy(data, recData, actualPduSize);

    /* Return actual PDU size to caller */
    if (actualSize != NULL)
    {
        *actualSize = (uint16)actualPduSize;
    }

    #ifdef ETHERNET_DEBUG
        printf("id = %d \n", *id);
    #endif

    /* Close the socket */
    closesocket(client_socket);
    closesocket(server_socket);
    return E_OK;
}

void ethernet_RecieveMainFunction(void)
{
    static uint8 dataRecieve[BUS_LENGTH_RECEIVE];
    uint16 id;
    uint16 actualSize;
    if (ethernet_receive(dataRecieve, BUS_LENGTH_RECEIVE, &id, &actualSize) != E_OK)
    {
        return;
    }
    PduInfoType PduInfoPtr = {
        .SduDataPtr = dataRecieve,
        .MetaDataPtr = (uint8*)&PdusCollections[id],
        .SduLength = actualSize,  /* Use actual received size, not buffer size */
    };
    switch (PdusCollections[id].Type)
    {
    case SECOC_SECURED_PDU_CANIF:
        #ifdef ETHERNET_DEBUG
            printf("here in Direct \n");
        #endif
        CanIf_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_CANTP:
        #ifdef ETHERNET_DEBUG
            printf("here in CANTP \n");
        #endif
        CanTp_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_SOADTP:
        #ifdef ETHERNET_DEBUG
            printf("here in Ethernet SOADTP \n");
        #endif
        SoAdTp_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_SOADIF:
        #ifdef ETHERNET_DEBUG
            printf("here in Ethernet SOADIF \n");
        #endif
        PduR_SoAdIfRxIndication(id, &PduInfoPtr);
        break;

    case SECOC_AUTH_COLLECTON_PDU:
        #ifdef ETHERNET_DEBUG
            printf("here in Direct - pdu collection - auth\n");
        #endif
        CanIf_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_CRYPTO_COLLECTON_PDU:
        #ifdef ETHERNET_DEBUG
            printf("here in Direct- pdu collection - crypto \n");
        #endif
        CanIf_RxIndication(id, &PduInfoPtr);
        break;
    default:
        #ifdef ETHERNET_DEBUG
            printf("This is no type like it for ID : %d  type : %d \n", id, PdusCollections[id].Type);
        #endif
        break;
    }
}
