/**
 * @file EthSM.h
 * @brief Ethernet State Manager (EthSM) module header
 * @details AUTOSAR CP R24-11 compliant Ethernet State Manager.
 *          Manages Ethernet communication states and integrates with ComM, EthIf, and TcpIp.
 */

#ifndef ETHSM_H
#define ETHSM_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComM/ComM.h"
#include "TcpIp/TcpIp.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Module identification */
#define ETHSM_MODULE_ID                  (143U)
#define ETHSM_VENDOR_ID                  (0U)
#define ETHSM_INSTANCE_ID                (0U)
#define ETHSM_SW_MAJOR_VERSION           (1U)
#define ETHSM_SW_MINOR_VERSION           (0U)
#define ETHSM_SW_PATCH_VERSION           (0U)

/* Configuration */
#define ETHSM_MAX_NETWORKS               (1U)

/* Development Error Detection */
#ifndef ETHSM_DEV_ERROR_DETECT
#define ETHSM_DEV_ERROR_DETECT           STD_ON
#endif

/* Service IDs for DET reporting */
#define ETHSM_SID_INIT                   (0x01U)
#define ETHSM_SID_REQUEST_COM_MODE       (0x02U)
#define ETHSM_SID_GET_CURRENT_COM_MODE   (0x03U)
#define ETHSM_SID_GET_CURRENT_INTERNAL_MODE (0x04U)
#define ETHSM_SID_TCPIP_MODE_INDICATION  (0x05U)
#define ETHSM_SID_MAIN_FUNCTION          (0x06U)
#define ETHSM_SID_GET_VERSION_INFO       (0x07U)
#define ETHSM_SID_DEINIT                 (0x08U)

/* DET error codes */
#define ETHSM_E_NOT_INITIALIZED          (0x01U)
#define ETHSM_E_PARAM_POINTER            (0x02U)
#define ETHSM_E_INV_NETWORK_HANDLE       (0x03U)
#define ETHSM_E_INV_NETWORK_MODE         (0x04U)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

/* cppcheck-suppress misra-c2012-5.6 */
/** @brief Network handle type */
typedef uint8 NetworkHandleType;

/** @brief EthSM internal network mode state */
typedef enum
{
    ETHSM_STATE_OFFLINE        = 0U,
    ETHSM_STATE_WAIT_TRCVLINK  = 1U,
    ETHSM_STATE_WAIT_ONLINE    = 2U,
    ETHSM_STATE_ONLINE         = 3U
} EthSM_NetworkModeStateType;

/** @brief EthSM configuration type */
typedef struct
{
    uint8 EthSM_CtrlIdx;                /**< Mapped EthIf controller index */
    TcpIp_LocalAddrIdType LocalAddrId;   /**< TcpIp local address for IP assignment */
} EthSM_NetworkConfigType;

/** @brief EthSM configuration type (top-level) */
typedef struct
{
    const EthSM_NetworkConfigType* Networks;
    uint8 NumNetworks;
} EthSM_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototypes*********************************************/
/********************************************************************************************************/

/**
 * @brief Initializes the EthSM module.
 * @param[in] ConfigPtr Pointer to configuration. May be NULL for default config.
 */
void EthSM_Init(const EthSM_ConfigType* ConfigPtr);
void EthSM_DeInit(void);

/**
 * @brief Request a communication mode for an Ethernet network.
 * @param[in] NetworkHandle  Network handle (0..ETHSM_MAX_NETWORKS-1).
 * @param[in] ComM_Mode      Requested communication mode.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthSM_RequestComMode(NetworkHandleType NetworkHandle, ComM_ModeType ComM_Mode);

/**
 * @brief Get the current communication mode of an Ethernet network.
 * @param[in]  NetworkHandle Network handle.
 * @param[out] ComM_ModePtr  Pointer to store the current mode.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthSM_GetCurrentComMode(NetworkHandleType NetworkHandle, ComM_ModeType* ComM_ModePtr);

/**
 * @brief Get the current internal mode state of an Ethernet network.
 * @param[in]  NetworkHandle   Network handle.
 * @param[out] EthSM_InternalMode Pointer to store the internal mode.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthSM_GetCurrentInternalMode(NetworkHandleType NetworkHandle,
                                             EthSM_NetworkModeStateType* EthSM_InternalMode);

/**
 * @brief Callback from TcpIp indicating a state change.
 * @param[in] CtrlIdx  Controller index.
 * @param[in] State    New TcpIp state.
 */
void EthSM_TcpIpModeIndication(uint8 CtrlIdx, TcpIp_StateType State);

/**
 * @brief Periodic main function for EthSM state machine processing.
 */
void EthSM_MainFunction(void);

/**
 * @brief Get module version information.
 * @param[out] VersionInfoPtr Pointer to version info structure.
 */
void EthSM_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

#endif /* ETHSM_H */
