/**
 * @file EthIf.h
 * @brief Ethernet Interface (EthIf) module header
 * @details AUTOSAR CP R24-11 compliant Ethernet Interface abstraction layer.
 *          Sits between TcpIp (upper) and raw Eth driver (lower).
 */

#ifndef ETHIF_H
#define ETHIF_H

#include "Std_Types.h"
#include "Com/ComStack_Types.h"

/* --- Module Identification --- */
#define ETHIF_MODULE_ID                  (65U)
#define ETHIF_VENDOR_ID                  (0U)
#define ETHIF_SW_MAJOR_VERSION           (1U)
#define ETHIF_SW_MINOR_VERSION           (0U)
#define ETHIF_SW_PATCH_VERSION           (0U)

/* --- Configuration --- */
#define ETHIF_MAX_CTRL                   (1U)
#define ETHIF_TX_BUF_COUNT               (4U)
#define ETHIF_TX_BUF_SIZE                (4096U)
#define ETHIF_RX_BUF_SIZE                (4096U)
#define ETHIF_DEV_ERROR_DETECT           STD_ON

/* --- Service IDs --- */
#define ETHIF_SID_INIT                   (0x01U)
#define ETHIF_SID_SET_CONTROLLER_MODE    (0x03U)
#define ETHIF_SID_GET_CONTROLLER_MODE    (0x04U)
#define ETHIF_SID_GET_PHYS_ADDR          (0x08U)
#define ETHIF_SID_PROVIDE_TX_BUFFER      (0x09U)
#define ETHIF_SID_TRANSMIT               (0x0AU)
#define ETHIF_SID_RX_INDICATION          (0x10U)
#define ETHIF_SID_TX_CONFIRMATION        (0x11U)
#define ETHIF_SID_GET_VERSION_INFO       (0x0BU)
#define ETHIF_SID_MAIN_FUNCTION_RX       (0x20U)
#define ETHIF_SID_MAIN_FUNCTION_TX       (0x21U)

/* --- DET Error Codes --- */
#define ETHIF_E_INV_CTRL_IDX             (0x01U)
#define ETHIF_E_NOT_INITIALIZED          (0x02U)
#define ETHIF_E_PARAM_POINTER            (0x03U)
#define ETHIF_E_INV_PARAM                (0x04U)
#define ETHIF_E_NO_BUFFER                (0x05U)

/* --- Type Definitions --- */

/** @brief Ethernet controller mode */
typedef enum
{
    ETH_MODE_DOWN   = 0U,
    ETH_MODE_ACTIVE = 1U
} Eth_ModeType;

/** @brief Ethernet frame type (EtherType field) */
typedef uint16 Eth_FrameType;

/** @brief Tx buffer index */
typedef uint8 Eth_BufIdxType;

/** @brief EthIf configuration type */
typedef struct
{
    uint8 dummy;
} EthIf_ConfigType;

/** @brief Rx indication callback type for upper layers */
typedef void (*EthIf_RxIndicationCbkType)(uint8 CtrlIdx,
                                          Eth_FrameType FrameType,
                                          boolean IsBroadcast,
                                          const uint8* PhysAddrPtr,
                                          const uint8* DataPtr,
                                          uint16 LenByte);

/** @brief Tx confirmation callback type for upper layers */
typedef void (*EthIf_TxConfirmationCbkType)(uint8 CtrlIdx,
                                            Eth_BufIdxType BufIdx);

/* --- Common EtherType constants --- */
#define ETH_FRAME_TYPE_IPV4              (0x0800U)
#define ETH_FRAME_TYPE_ARP               (0x0806U)
#define ETH_FRAME_TYPE_IPV6              (0x86DDU)

/* --- Function Prototypes --- */

/**
 * @brief Initializes the Ethernet Interface module.
 * @param[in] CfgPtr Pointer to configuration. May be NULL.
 */
void EthIf_Init(const EthIf_ConfigType* CfgPtr);

/**
 * @brief Set the mode of an Ethernet controller.
 * @param[in] CtrlIdx  Controller index (0..ETHIF_MAX_CTRL-1).
 * @param[in] CtrlMode Requested mode (ETH_MODE_DOWN / ETH_MODE_ACTIVE).
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode);

/**
 * @brief Get the current mode of an Ethernet controller.
 * @param[in]  CtrlIdx     Controller index.
 * @param[out] CtrlModePtr Pointer to store the current mode.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthIf_GetControllerMode(uint8 CtrlIdx, Eth_ModeType* CtrlModePtr);

/**
 * @brief Get the physical (MAC) address of an Ethernet controller.
 * @param[in]  CtrlIdx    Controller index.
 * @param[out] PhysAddrPtr Pointer to 6-byte buffer to store MAC address.
 */
void EthIf_GetPhysAddr(uint8 CtrlIdx, uint8* PhysAddrPtr);

/**
 * @brief Request a transmit buffer from the pool.
 * @param[in]     CtrlIdx    Controller index.
 * @param[in]     FrameType  Ethernet frame type.
 * @param[in]     Priority   Transmit priority (unused, reserved).
 * @param[out]    BufIdxPtr  Pointer to store the allocated buffer index.
 * @param[out]    BufPtr     Pointer to store the buffer data pointer.
 * @param[in,out] LenBytePtr In: requested length, Out: granted length.
 * @return BUFREQ_OK on success, BUFREQ_E_BUSY if no buffer available.
 */
BufReq_ReturnType EthIf_ProvideTxBuffer(uint8 CtrlIdx,
                                        Eth_FrameType FrameType,
                                        uint8 Priority,
                                        Eth_BufIdxType* BufIdxPtr,
                                        uint8** BufPtr,
                                        uint16* LenBytePtr);

/**
 * @brief Transmit data using a previously allocated buffer.
 * @param[in] CtrlIdx        Controller index.
 * @param[in] BufIdx         Buffer index from EthIf_ProvideTxBuffer.
 * @param[in] FrameType      Ethernet frame type.
 * @param[in] TxConfirmation TRUE if Tx confirmation callback is requested.
 * @param[in] LenByte        Number of bytes to transmit.
 * @param[in] PhysAddrPtr    Destination MAC address (6 bytes). May be NULL.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType EthIf_Transmit(uint8 CtrlIdx,
                              Eth_BufIdxType BufIdx,
                              Eth_FrameType FrameType,
                              boolean TxConfirmation,
                              uint16 LenByte,
                              const uint8* PhysAddrPtr);

/**
 * @brief Rx indication callback from the Ethernet driver.
 * @param[in] CtrlIdx     Controller index.
 * @param[in] FrameType   Ethernet frame type of received frame.
 * @param[in] IsBroadcast TRUE if the frame was a broadcast.
 * @param[in] PhysAddrPtr Source MAC address (6 bytes).
 * @param[in] DataPtr     Pointer to received data payload.
 * @param[in] LenByte     Length of received data.
 */
void EthIf_RxIndication(uint8 CtrlIdx,
                        Eth_FrameType FrameType,
                        boolean IsBroadcast,
                        const uint8* PhysAddrPtr,
                        const uint8* DataPtr,
                        uint16 LenByte);

/**
 * @brief Tx confirmation callback from the Ethernet driver.
 * @param[in] CtrlIdx Controller index.
 * @param[in] BufIdx  Buffer index of the confirmed transmission.
 */
void EthIf_TxConfirmation(uint8 CtrlIdx, Eth_BufIdxType BufIdx);

/**
 * @brief Get module version information.
 * @param[out] VersionInfoPtr Pointer to version info structure.
 */
void EthIf_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/**
 * @brief Periodic Rx main function - polls the Ethernet driver for received frames.
 */
void EthIf_MainFunctionRx(void);

/**
 * @brief Periodic Tx main function - handles pending Tx confirmations.
 */
void EthIf_MainFunctionTx(void);

/**
 * @brief Register an upper-layer Rx indication callback.
 * @param[in] Callback Function pointer for Rx indication.
 */
void EthIf_SetRxIndicationCallback(EthIf_RxIndicationCbkType Callback);

/**
 * @brief Register an upper-layer Tx confirmation callback.
 * @param[in] Callback Function pointer for Tx confirmation.
 */
void EthIf_SetTxConfirmationCallback(EthIf_TxConfirmationCbkType Callback);

#endif /* ETHIF_H */
