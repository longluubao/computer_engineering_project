/**
 * @file Can_Pi4.c
 * @brief CAN Driver Implementation for Raspberry Pi 4 via SocketCAN
 * @details Replaces the simulation-based Can.c with a real hardware
 *          driver using Linux SocketCAN (AF_CAN / PF_CAN).
 *
 *          The Pi 4 does not have a built-in CAN controller; CAN is
 *          typically provided via:
 *          - MCP2515 SPI-to-CAN module (most common)
 *          - USB-CAN adapter (e.g. PEAK PCAN-USB)
 *          - Virtual CAN (vcan) for development/testing
 *
 *          Prerequisites on the Pi:
 *          - SocketCAN kernel modules loaded (can, can_raw, vcan, mcp251x)
 *          - Interface brought up:
 *              sudo ip link set can0 up type can bitrate 500000
 *            or for virtual CAN:
 *              sudo modprobe vcan
 *              sudo ip link add dev vcan0 type vcan
 *              sudo ip link set up vcan0
 *
 *          This file implements the AUTOSAR Can driver API (Can.h) using
 *          raw CAN sockets. It is selected instead of source/Can/Can.c
 *          at build time via CMake when MCAL_TARGET=PI4.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Can/Can.h"
#include "Det/Det.h"
#include "Mcal/Mcal_Cfg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/********************************************************************************************************/
/******************************************InternalTypes*************************************************/
/********************************************************************************************************/

/** @brief Per-controller runtime state for SocketCAN */
typedef struct
{
    Can_ControllerStateType State;
    int SocketFd;                       /**< Raw CAN socket fd */
    char IfName[16];                    /**< Interface name (e.g. "can0") */
    uint32 TxCount;
    uint32 RxCount;
    uint32 ErrorCount;
} Can_Pi4_ControllerType;

typedef struct
{
    PduIdType TxPduId;
    Can_PduType PduData;
    uint8 SduBuf[8];
} Can_TxQueueEntryType;

#define CAN_RX_QUEUE_LENGTH ((uint8)32)

typedef struct
{
    PduIdType  RxPduId;
    PduInfoType PduInfo;
    uint8 SduData[8];
} Can_RxQueueEntryType;

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Can_Initialized = FALSE;
static Can_Pi4_ControllerType Can_Controllers[CAN_MAX_CONTROLLERS];

/* Tx queue */
static Can_TxQueueEntryType Can_TxQueue[CAN_TX_QUEUE_LENGTH];
static uint8 Can_TxQueueHead = 0U;
static uint8 Can_TxQueueTail = 0U;
static uint8 Can_TxQueueCount = 0U;
static Can_TxConfirmationCallbackType Can_TxConfirmationCallback = NULL;

/* Rx queue */
static Can_RxQueueEntryType Can_RxQueue[CAN_RX_QUEUE_LENGTH];
static uint8 Can_RxQueueHead = 0U;
static uint8 Can_RxQueueTail = 0U;
static uint8 Can_RxQueueCount = 0U;
static Can_RxIndicationCallbackType Can_RxIndicationCallback = NULL;

/********************************************************************************************************/
/******************************************InternalFunctions*********************************************/
/********************************************************************************************************/

static void Can_ResetQueue(void)
{
    Can_TxQueueHead = 0U;
    Can_TxQueueTail = 0U;
    Can_TxQueueCount = 0U;
    Can_RxQueueHead = 0U;
    Can_RxQueueTail = 0U;
    Can_RxQueueCount = 0U;
}

static int Can_OpenSocket(const char* ifName)
{
    int sockFd;
    struct sockaddr_can addr;
    struct ifreq ifr;

    sockFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockFd < 0)
    {
        (void)printf("[CAN_Pi4] Failed to create CAN socket: %s\n", strerror(errno));
        return -1;
    }

    /* Set non-blocking mode for polling-based Rx */
    (void)fcntl(sockFd, F_SETFL, O_NONBLOCK);

    (void)memset(&ifr, 0, sizeof(ifr));
    (void)strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name) - 1U);

    if (ioctl(sockFd, SIOCGIFINDEX, &ifr) < 0)
    {
        (void)printf("[CAN_Pi4] Interface '%s' not found: %s\n", ifName, strerror(errno));
        (void)close(sockFd);
        return -1;
    }

    (void)memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        (void)printf("[CAN_Pi4] Failed to bind CAN socket: %s\n", strerror(errno));
        (void)close(sockFd);
        return -1;
    }

    return sockFd;
}

/********************************************************************************************************/
/***************************************ForwardDeclarations**********************************************/
/********************************************************************************************************/

void Can_Init(const Can_ConfigType* ConfigPtr);
void Can_DeInit(void);
Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType* ControllerModePtr);
Std_ReturnType Can_Write(PduIdType Hth, const Can_PduType* PduInfo);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);
void Can_RegisterTxConfirmation(Can_TxConfirmationCallbackType Callback);
void Can_RegisterRxIndication(Can_RxIndicationCallbackType Callback);
void Can_SimulateReception(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Can_Init(const Can_ConfigType* ConfigPtr)
{
    uint8 ctrl;
    (void)ConfigPtr;

    if (Can_Initialized != FALSE)
    {
        return;
    }

    for (ctrl = 0U; ctrl < CAN_MAX_CONTROLLERS; ctrl++)
    {
        Can_Controllers[ctrl].State = CAN_CS_STOPPED;
        Can_Controllers[ctrl].SocketFd = -1;
        Can_Controllers[ctrl].TxCount = 0U;
        Can_Controllers[ctrl].RxCount = 0U;
        Can_Controllers[ctrl].ErrorCount = 0U;

        /* Build interface name: can0, can1, ... */
        (void)snprintf(Can_Controllers[ctrl].IfName,
                       sizeof(Can_Controllers[ctrl].IfName),
                       "can%u", (unsigned int)ctrl);
    }

    Can_ResetQueue();
    Can_Initialized = TRUE;
    (void)printf("[CAN_Pi4] CAN driver initialized (%u controllers)\n",
                 (unsigned int)CAN_MAX_CONTROLLERS);
}

void Can_DeInit(void)
{
    uint8 ctrl;

    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_DEINIT, CAN_E_UNINIT);
        return;
    }

    for (ctrl = 0U; ctrl < CAN_MAX_CONTROLLERS; ctrl++)
    {
        if (Can_Controllers[ctrl].SocketFd >= 0)
        {
            (void)close(Can_Controllers[ctrl].SocketFd);
            Can_Controllers[ctrl].SocketFd = -1;
        }
        Can_Controllers[ctrl].State = CAN_CS_UNINIT;
    }

    Can_ResetQueue();
    Can_TxConfirmationCallback = NULL;
    Can_RxIndicationCallback = NULL;
    Can_Initialized = FALSE;
}

Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition)
{
    Std_ReturnType RetVal = E_OK;
    Can_ControllerStateType CurrentState;

    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_UNINIT);
        return E_NOT_OK;
    }

    if (Controller >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    CurrentState = Can_Controllers[Controller].State;

    switch (Transition)
    {
        case CAN_CS_STARTED:
            if ((CurrentState != CAN_CS_STOPPED) && (CurrentState != CAN_CS_STARTED))
            {
                (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
                RetVal = E_NOT_OK;
            }
            else if (CurrentState != CAN_CS_STARTED)
            {
                /* Open the SocketCAN socket on transition to STARTED */
                Can_Controllers[Controller].SocketFd = Can_OpenSocket(Can_Controllers[Controller].IfName);
                if (Can_Controllers[Controller].SocketFd < 0)
                {
                    (void)printf("[CAN_Pi4] WARNING: Could not open %s, trying vcan0\n",
                                 Can_Controllers[Controller].IfName);
                    /* Fallback to virtual CAN for development */
                    Can_Controllers[Controller].SocketFd = Can_OpenSocket("vcan0");
                }

                if (Can_Controllers[Controller].SocketFd < 0)
                {
                    (void)printf("[CAN_Pi4] ERROR: No CAN interface available for controller %u\n",
                                 (unsigned int)Controller);
                    RetVal = E_NOT_OK;
                }
                else
                {
                    Can_Controllers[Controller].State = CAN_CS_STARTED;
                    (void)printf("[CAN_Pi4] Controller %u STARTED on %s (fd=%d)\n",
                                 (unsigned int)Controller,
                                 Can_Controllers[Controller].IfName,
                                 Can_Controllers[Controller].SocketFd);
                }
            }
            else
            {
                /* Already started, no action */
            }
            break;

        case CAN_CS_STOPPED:
            if (Can_Controllers[Controller].SocketFd >= 0)
            {
                (void)close(Can_Controllers[Controller].SocketFd);
                Can_Controllers[Controller].SocketFd = -1;
            }
            Can_Controllers[Controller].State = CAN_CS_STOPPED;
            break;

        case CAN_CS_SLEEP:
            if ((CurrentState != CAN_CS_STOPPED) && (CurrentState != CAN_CS_SLEEP))
            {
                (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
                RetVal = E_NOT_OK;
            }
            else
            {
                if (Can_Controllers[Controller].SocketFd >= 0)
                {
                    (void)close(Can_Controllers[Controller].SocketFd);
                    Can_Controllers[Controller].SocketFd = -1;
                }
                Can_Controllers[Controller].State = CAN_CS_SLEEP;
            }
            break;

        default:
            (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
            RetVal = E_NOT_OK;
            break;
    }

    return RetVal;
}

Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType* ControllerModePtr)
{
    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_UNINIT);
        return E_NOT_OK;
    }

    if (Controller >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (ControllerModePtr == NULL)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *ControllerModePtr = Can_Controllers[Controller].State;
    return E_OK;
}

Std_ReturnType Can_Write(PduIdType Hth, const Can_PduType* PduInfo)
{
    Std_ReturnType RetVal = E_OK;

    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfo == NULL)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Hth >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_HANDLE);
        return E_NOT_OK;
    }

    if ((PduInfo->sdu == NULL) && (PduInfo->length > 0U))
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Can_Controllers[Hth].State != CAN_CS_STARTED)
    {
        return E_NOT_OK;
    }

    if (Can_TxQueueCount >= CAN_TX_QUEUE_LENGTH)
    {
        return E_BUSY;
    }

    /* Enqueue the PDU with its data for deferred transmission */
    {
        Can_TxQueueEntryType* entry = &Can_TxQueue[Can_TxQueueTail];
        uint8 copyLen = (PduInfo->length <= 8U) ? PduInfo->length : 8U;

        entry->TxPduId = PduInfo->swPduHandle;
        entry->PduData.id = PduInfo->id;
        entry->PduData.length = copyLen;
        entry->PduData.swPduHandle = PduInfo->swPduHandle;

        if ((PduInfo->sdu != NULL) && (copyLen > 0U))
        {
            (void)memcpy(entry->SduBuf, PduInfo->sdu, copyLen);
        }
        entry->PduData.sdu = entry->SduBuf;

        Can_TxQueueTail = (uint8)((Can_TxQueueTail + 1U) % CAN_TX_QUEUE_LENGTH);
        Can_TxQueueCount++;
    }

    return RetVal;
}

void Can_MainFunction_Write(void)
{
    if (Can_Initialized == FALSE)
    {
        return;
    }

    while (Can_TxQueueCount > 0U)
    {
        Can_TxQueueEntryType* entry = &Can_TxQueue[Can_TxQueueHead];
        struct can_frame frame;
        ssize_t nbytes;
        Std_ReturnType txResult = E_OK;
        uint8 ctrl = 0U; /* Default to controller 0 */

        /* Build the SocketCAN frame */
        (void)memset(&frame, 0, sizeof(frame));
        frame.can_id = entry->PduData.id;
        frame.can_dlc = entry->PduData.length;
        if ((entry->PduData.sdu != NULL) && (entry->PduData.length > 0U))
        {
            (void)memcpy(frame.data, entry->PduData.sdu, entry->PduData.length);
        }

        /* Send the frame on the SocketCAN interface */
        if (Can_Controllers[ctrl].SocketFd >= 0)
        {
            nbytes = write(Can_Controllers[ctrl].SocketFd, &frame, sizeof(frame));
            if (nbytes != (ssize_t)sizeof(frame))
            {
                txResult = E_NOT_OK;
                Can_Controllers[ctrl].ErrorCount++;
            }
            else
            {
                Can_Controllers[ctrl].TxCount++;
            }
        }
        else
        {
            /* No socket: behave like simulation (immediate confirm) */
            txResult = E_OK;
        }

        Can_TxQueueHead = (uint8)((Can_TxQueueHead + 1U) % CAN_TX_QUEUE_LENGTH);
        Can_TxQueueCount--;

        if (Can_TxConfirmationCallback != NULL)
        {
            Can_TxConfirmationCallback(entry->TxPduId, txResult);
        }
    }
}

void Can_MainFunction_Read(void)
{
    uint8 ctrl;
    struct can_frame frame;
    ssize_t nbytes;

    if (Can_Initialized == FALSE)
    {
        return;
    }

    /* Poll each controller for incoming frames */
    for (ctrl = 0U; ctrl < CAN_MAX_CONTROLLERS; ctrl++)
    {
        if ((Can_Controllers[ctrl].State != CAN_CS_STARTED) ||
            (Can_Controllers[ctrl].SocketFd < 0))
        {
            continue;
        }

        /* Read all available frames (non-blocking) */
        while (TRUE)
        {
            nbytes = read(Can_Controllers[ctrl].SocketFd, &frame, sizeof(frame));
            if (nbytes != (ssize_t)sizeof(frame))
            {
                break; /* No more frames or error */
            }

            Can_Controllers[ctrl].RxCount++;

            /* Map CAN ID to AUTOSAR PduId.
             * The PDU ID mapping depends on configuration; for the gateway
             * we use the CAN ID directly as a PduId lookup key.
             * CanIf is responsible for the final mapping. */
            if (Can_RxQueueCount < CAN_RX_QUEUE_LENGTH)
            {
                Can_RxQueueEntryType* rxEntry = &Can_RxQueue[Can_RxQueueTail];

                /* Use CAN ID as RxPduId (CanIf will do the proper mapping) */
                rxEntry->RxPduId = (PduIdType)(frame.can_id & CAN_SFF_MASK);
                rxEntry->PduInfo.SduLength = (PduLengthType)frame.can_dlc;
                (void)memcpy(rxEntry->SduData, frame.data, frame.can_dlc);
                rxEntry->PduInfo.SduDataPtr = rxEntry->SduData;
                rxEntry->PduInfo.MetaDataPtr = NULL;

                Can_RxQueueTail = (uint8)((Can_RxQueueTail + 1U) % CAN_RX_QUEUE_LENGTH);
                Can_RxQueueCount++;
            }
            else
            {
                /* Rx queue overflow */
                Can_Controllers[ctrl].ErrorCount++;
            }
        }
    }

    /* Deliver queued Rx indications to upper layer */
    while (Can_RxQueueCount > 0U)
    {
        Can_RxQueueEntryType* entry = &Can_RxQueue[Can_RxQueueHead];
        Can_RxQueueHead = (uint8)((Can_RxQueueHead + 1U) % CAN_RX_QUEUE_LENGTH);
        Can_RxQueueCount--;

        if (Can_RxIndicationCallback != NULL)
        {
            Can_RxIndicationCallback(entry->RxPduId, &entry->PduInfo);
        }
    }
}

void Can_RegisterTxConfirmation(Can_TxConfirmationCallbackType Callback)
{
    Can_TxConfirmationCallback = Callback;
}

void Can_RegisterRxIndication(Can_RxIndicationCallbackType Callback)
{
    Can_RxIndicationCallback = Callback;
}

void Can_SimulateReception(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* Simulation injection is still supported for testing even on Pi 4 */
    if ((Can_Initialized == FALSE) || (PduInfoPtr == NULL))
    {
        return;
    }

    if (Can_RxQueueCount >= CAN_RX_QUEUE_LENGTH)
    {
        return;
    }

    {
        Can_RxQueueEntryType* entry = &Can_RxQueue[Can_RxQueueTail];
        PduLengthType copyLen = (PduInfoPtr->SduLength <= 8U) ? PduInfoPtr->SduLength : 8U;

        entry->RxPduId = RxPduId;
        if ((PduInfoPtr->SduDataPtr != NULL) && (copyLen > 0U))
        {
            (void)memcpy(entry->SduData, PduInfoPtr->SduDataPtr, copyLen);
        }
        entry->PduInfo.SduDataPtr = entry->SduData;
        entry->PduInfo.SduLength = copyLen;
        entry->PduInfo.MetaDataPtr = NULL;

        Can_RxQueueTail = (uint8)((Can_RxQueueTail + 1U) % CAN_RX_QUEUE_LENGTH);
        Can_RxQueueCount++;
    }
}
