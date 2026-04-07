/**
 * @file EthSM.c
 * @brief Ethernet State Manager (EthSM) module implementation
 * @details AUTOSAR CP R24-11 compliant Ethernet State Manager.
 *          Manages Ethernet communication state machine and integrates with
 *          ComM (upper), EthIf (lower controller), and TcpIp (IP stack).
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "BswM/BswM.h"
#include "Det/Det.h"
#include "EthIf/EthIf.h"
#include "EthSM/EthSM.h"
#include "TcpIp/TcpIp.h"

/* MISRA C:2012 Rule 8.4 - Forward declarations for external linkage functions */
extern void EthSM_Init(const EthSM_ConfigType* ConfigPtr);
extern void EthSM_DeInit(void);
extern Std_ReturnType EthSM_RequestComMode(NetworkHandleType NetworkHandle, ComM_ModeType ComM_Mode);
extern Std_ReturnType EthSM_GetCurrentComMode(NetworkHandleType NetworkHandle, ComM_ModeType* ComM_ModePtr);
extern Std_ReturnType EthSM_GetCurrentInternalMode(NetworkHandleType NetworkHandle, EthSM_NetworkModeStateType* EthSM_InternalMode);
extern void EthSM_TcpIpModeIndication(uint8 CtrlIdx, TcpIp_StateType State);
extern void EthSM_MainFunction(void);
extern void EthSM_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode);
extern Std_ReturnType EthIf_GetControllerMode(uint8 CtrlIdx, Eth_ModeType* CtrlModePtr);
extern Std_ReturnType TcpIp_RequestIpAddrAssignment(TcpIp_LocalAddrIdType LocalAddrId,
                                                    TcpIp_IpAddrAssignmentType Type,
                                                    const TcpIp_SockAddrType* LocalIpAddrPtr);

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

/** @brief Default network configuration (single network, controller 0) */
static const EthSM_NetworkConfigType EthSM_DefaultNetworkCfg[ETHSM_MAX_NETWORKS] =
{
    {
        0U,     /* EthSM_CtrlIdx */
        0U      /* LocalAddrId */
    }
};

static const EthSM_ConfigType EthSM_DefaultConfig =
{
    EthSM_DefaultNetworkCfg,
    ETHSM_MAX_NETWORKS
};

/** @brief Module initialization state */
static boolean EthSM_Initialized = FALSE;

/** @brief Active configuration pointer */
static const EthSM_ConfigType* EthSM_ConfigPtr = NULL;

/** @brief Per-network runtime state */
typedef struct
{
    EthSM_NetworkModeStateType InternalState;
    ComM_ModeType              RequestedMode;
    ComM_ModeType              CurrentComMode;
    TcpIp_StateType            TcpIpState;
} EthSM_NetworkStateType;

static EthSM_NetworkStateType EthSM_NetworkState[ETHSM_MAX_NETWORKS];

/********************************************************************************************************/
/*****************************************StaticFunctions************************************************/
/********************************************************************************************************/

/**
 * @brief Find the network index that owns the given EthIf controller.
 * @param[in]  CtrlIdx     EthIf controller index.
 * @param[out] NetworkIdx  Pointer to store the network index.
 * @return TRUE if found, FALSE otherwise.
 */
static boolean EthSM_FindNetworkByCtrl(uint8 CtrlIdx, uint8* NetworkIdx)
{
    uint8 idx;

    for (idx = 0U; idx < EthSM_ConfigPtr->NumNetworks; idx++)
    {
        if (EthSM_ConfigPtr->Networks[idx].EthSM_CtrlIdx == CtrlIdx)
        {
            *NetworkIdx = idx;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief Notify ComM of a communication mode change on a network.
 * @param[in] NetworkHandle Network handle / channel.
 * @param[in] ComMode       New communication mode.
 */
static void EthSM_NotifyComM(NetworkHandleType NetworkHandle, ComM_ModeType ComMode)
{
    (void)ComM_BusSM_ModeIndication(NetworkHandle, ComMode);
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void EthSM_Init(const EthSM_ConfigType* ConfigPtr)
{
    uint8 idx;

    if (ConfigPtr != NULL)
    {
        EthSM_ConfigPtr = ConfigPtr;
    }
    else
    {
        EthSM_ConfigPtr = &EthSM_DefaultConfig;
    }

    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        EthSM_NetworkState[idx].InternalState = ETHSM_STATE_OFFLINE;
        EthSM_NetworkState[idx].RequestedMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].CurrentComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].TcpIpState    = TCPIP_STATE_OFFLINE;
    }

    EthSM_Initialized = TRUE;
    (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_OFFLINE);
}

void EthSM_DeInit(void)
{
    uint8 idx;

    if (EthSM_Initialized == FALSE)
    {
        return;
    }

    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        EthSM_NetworkState[idx].InternalState = ETHSM_STATE_OFFLINE;
        EthSM_NetworkState[idx].RequestedMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].CurrentComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].TcpIpState = TCPIP_STATE_OFFLINE;
    }

    EthSM_Initialized = FALSE;
}

Std_ReturnType EthSM_RequestComMode(NetworkHandleType NetworkHandle, ComM_ModeType ComM_Mode)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_REQUEST_COM_MODE, ETHSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NetworkHandle >= ETHSM_MAX_NETWORKS)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_REQUEST_COM_MODE, ETHSM_E_INV_NETWORK_HANDLE);
        return E_NOT_OK;
    }

    if ((ComM_Mode != COMM_NO_COMMUNICATION) && (ComM_Mode != COMM_FULL_COMMUNICATION))
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_REQUEST_COM_MODE, ETHSM_E_INV_NETWORK_MODE);
        return E_NOT_OK;
    }
#endif

    EthSM_NetworkState[NetworkHandle].RequestedMode = ComM_Mode;

    return E_OK;
}

Std_ReturnType EthSM_GetCurrentComMode(NetworkHandleType NetworkHandle, ComM_ModeType* ComM_ModePtr)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_COM_MODE, ETHSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NetworkHandle >= ETHSM_MAX_NETWORKS)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_COM_MODE, ETHSM_E_INV_NETWORK_HANDLE);
        return E_NOT_OK;
    }

    if (ComM_ModePtr == NULL)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_COM_MODE, ETHSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *ComM_ModePtr = EthSM_NetworkState[NetworkHandle].CurrentComMode;
    return E_OK;
}

Std_ReturnType EthSM_GetCurrentInternalMode(NetworkHandleType NetworkHandle,
                                             EthSM_NetworkModeStateType* EthSM_InternalMode)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_INTERNAL_MODE, ETHSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NetworkHandle >= ETHSM_MAX_NETWORKS)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_INTERNAL_MODE, ETHSM_E_INV_NETWORK_HANDLE);
        return E_NOT_OK;
    }

    if (EthSM_InternalMode == NULL)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_CURRENT_INTERNAL_MODE, ETHSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *EthSM_InternalMode = EthSM_NetworkState[NetworkHandle].InternalState;
    return E_OK;
}

void EthSM_TcpIpModeIndication(uint8 CtrlIdx, TcpIp_StateType State)
{
    uint8 networkIdx = 0U;

#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_TCPIP_MODE_INDICATION, ETHSM_E_NOT_INITIALIZED);
        return;
    }
#endif

    if (EthSM_FindNetworkByCtrl(CtrlIdx, &networkIdx) == FALSE)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_TCPIP_MODE_INDICATION, ETHSM_E_INV_NETWORK_HANDLE);
#endif
        return;
    }

    EthSM_NetworkState[networkIdx].TcpIpState = State;

    /* Handle immediate transitions based on TcpIp state callback */
    if (State == TCPIP_STATE_ONLINE)
    {
        if (EthSM_NetworkState[networkIdx].InternalState == ETHSM_STATE_WAIT_ONLINE)
        {
            EthSM_NetworkState[networkIdx].InternalState = ETHSM_STATE_ONLINE;
            EthSM_NetworkState[networkIdx].CurrentComMode = COMM_FULL_COMMUNICATION;
            (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_ONLINE);
            EthSM_NotifyComM(networkIdx, COMM_FULL_COMMUNICATION);
        }
    }
    else if (State == TCPIP_STATE_OFFLINE)
    {
        if (EthSM_NetworkState[networkIdx].InternalState == ETHSM_STATE_ONLINE)
        {
            EthSM_NetworkState[networkIdx].InternalState = ETHSM_STATE_OFFLINE;
            EthSM_NetworkState[networkIdx].CurrentComMode = COMM_NO_COMMUNICATION;
            (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_OFFLINE);
            EthSM_NotifyComM(networkIdx, COMM_NO_COMMUNICATION);
        }
    }
    else
    {
        /* TCPIP_STATE_UNINIT or other - no action */
    }
}

void EthSM_MainFunction(void)
{
    uint8 idx;

    if (EthSM_Initialized == FALSE)
    {
        return;
    }

    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        EthSM_NetworkModeStateType currentState = EthSM_NetworkState[idx].InternalState;
        ComM_ModeType requestedMode = EthSM_NetworkState[idx].RequestedMode;

        if (requestedMode == COMM_FULL_COMMUNICATION)
        {
            /* Transition towards ONLINE */
            switch (currentState)
            {
                case ETHSM_STATE_OFFLINE:
                {
                    /* Set EthIf controller to ACTIVE */
                    uint8 ctrlIdx = EthSM_ConfigPtr->Networks[idx].EthSM_CtrlIdx;

                    if (EthIf_SetControllerMode(ctrlIdx, ETH_MODE_ACTIVE) == E_OK)
                    {
                        EthSM_NetworkState[idx].InternalState = ETHSM_STATE_WAIT_TRCVLINK;
                        (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_WAIT_TRCVLINK);
                    }
                    break;
                }

                case ETHSM_STATE_WAIT_TRCVLINK:
                {
                    /* Check if controller is active (link up) */
                    Eth_ModeType ethMode = ETH_MODE_DOWN;
                    uint8 ctrlIdx = EthSM_ConfigPtr->Networks[idx].EthSM_CtrlIdx;

                    if (EthIf_GetControllerMode(ctrlIdx, &ethMode) == E_OK)
                    {
                        if (ethMode == ETH_MODE_ACTIVE)
                        {
                            /* Request IP address assignment from TcpIp */
                            TcpIp_LocalAddrIdType localAddr = EthSM_ConfigPtr->Networks[idx].LocalAddrId;

                            if (TcpIp_RequestIpAddrAssignment(localAddr,
                                    TCPIP_IPADDR_ASSIGNMENT_STATIC, NULL) == E_OK)
                            {
                                EthSM_NetworkState[idx].InternalState = ETHSM_STATE_WAIT_ONLINE;
                                (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_WAIT_ONLINE);
                            }
                        }
                    }
                    break;
                }

                case ETHSM_STATE_WAIT_ONLINE:
                {
                    /* Wait for TcpIpModeIndication callback to transition to ONLINE */
                    if (EthSM_NetworkState[idx].TcpIpState == TCPIP_STATE_ONLINE)
                    {
                        EthSM_NetworkState[idx].InternalState = ETHSM_STATE_ONLINE;
                        EthSM_NetworkState[idx].CurrentComMode = COMM_FULL_COMMUNICATION;
                        (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_ONLINE);
                        EthSM_NotifyComM(idx, COMM_FULL_COMMUNICATION);
                    }
                    break;
                }

                case ETHSM_STATE_ONLINE:
                {
                    /* Already online, nothing to do */
                    break;
                }

                default:
                {
                    /* Should not reach here */
                    break;
                }
            }
        }
        else if (requestedMode == COMM_NO_COMMUNICATION)
        {
            /* Transition towards OFFLINE */
            if (currentState != ETHSM_STATE_OFFLINE)
            {
                uint8 ctrlIdx = EthSM_ConfigPtr->Networks[idx].EthSM_CtrlIdx;

                /* Set EthIf controller to DOWN */
                (void)EthIf_SetControllerMode(ctrlIdx, ETH_MODE_DOWN);

                EthSM_NetworkState[idx].InternalState = ETHSM_STATE_OFFLINE;
                EthSM_NetworkState[idx].TcpIpState    = TCPIP_STATE_OFFLINE;
                (void)BswM_RequestMode((uint16)ETHSM_MODULE_ID, (BswM_ModeType)ETHSM_STATE_OFFLINE);

                if (EthSM_NetworkState[idx].CurrentComMode != COMM_NO_COMMUNICATION)
                {
                    EthSM_NetworkState[idx].CurrentComMode = COMM_NO_COMMUNICATION;
                    EthSM_NotifyComM(idx, COMM_NO_COMMUNICATION);
                }
            }
        }
        else
        {
            /* No action for other modes */
        }
    }
}

void EthSM_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL)
    {
        (void)Det_ReportError(ETHSM_MODULE_ID, ETHSM_INSTANCE_ID,
                              ETHSM_SID_GET_VERSION_INFO, ETHSM_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID         = ETHSM_VENDOR_ID;
    VersionInfoPtr->moduleID         = ETHSM_MODULE_ID;
    VersionInfoPtr->sw_major_version = ETHSM_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = ETHSM_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = ETHSM_SW_PATCH_VERSION;
}
