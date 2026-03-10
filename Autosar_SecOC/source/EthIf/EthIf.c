/**
 * @file EthIf.c
 * @brief Ethernet Interface (EthIf) module implementation
 * @details AUTOSAR CP R24-11 compliant Ethernet Interface abstraction layer.
 *          Wraps the existing ethernet_init/ethernet_send/ethernet_receive functions.
 */

#include "EthIf.h"
#include "Det.h"
#include <string.h>
#include <stdio.h>

#ifdef WINDOWS
    #include "ethernet_windows.h"
#else
    #include "ethernet.h"
#endif

/* --- Internal Types --- */

/** @brief State of a single Tx buffer slot */
typedef enum
{
    ETHIF_BUF_FREE = 0U,
    ETHIF_BUF_ALLOCATED,
    ETHIF_BUF_TX_PENDING
} EthIf_BufStateType;

/** @brief Tx buffer descriptor */
typedef struct
{
    uint8               Data[ETHIF_TX_BUF_SIZE];
    uint16              Length;
    Eth_FrameType       FrameType;
    boolean             ConfirmRequested;
    EthIf_BufStateType  State;
} EthIf_TxBufType;

/** @brief Per-controller runtime state */
typedef struct
{
    Eth_ModeType    Mode;
    uint8           PhysAddr[6];
    uint32          TxCount;
    uint32          RxCount;
    uint32          TxErrCount;
    uint32          RxErrCount;
} EthIf_CtrlStateType;

/* --- Static Data --- */

static boolean              EthIf_Initialized = FALSE;
static EthIf_CtrlStateType  EthIf_CtrlState[ETHIF_MAX_CTRL];
static EthIf_TxBufType      EthIf_TxBufPool[ETHIF_TX_BUF_COUNT];

/* Upper-layer callbacks */
static EthIf_RxIndicationCbkType    EthIf_RxCbk = NULL;
static EthIf_TxConfirmationCbkType  EthIf_TxCbk = NULL;

/* --- Function Definitions --- */

void EthIf_Init(const EthIf_ConfigType* CfgPtr)
{
    uint8 idx;

    (void)CfgPtr;

    /* Initialize controller state */
    for (idx = 0U; idx < ETHIF_MAX_CTRL; idx++)
    {
        EthIf_CtrlState[idx].Mode       = ETH_MODE_DOWN;
        (void)memset(EthIf_CtrlState[idx].PhysAddr, 0, 6U);
        EthIf_CtrlState[idx].TxCount    = 0U;
        EthIf_CtrlState[idx].RxCount    = 0U;
        EthIf_CtrlState[idx].TxErrCount = 0U;
        EthIf_CtrlState[idx].RxErrCount = 0U;
    }

    /* Initialize Tx buffer pool */
    for (idx = 0U; idx < ETHIF_TX_BUF_COUNT; idx++)
    {
        EthIf_TxBufPool[idx].State            = ETHIF_BUF_FREE;
        EthIf_TxBufPool[idx].Length            = 0U;
        EthIf_TxBufPool[idx].FrameType         = 0U;
        EthIf_TxBufPool[idx].ConfirmRequested  = FALSE;
    }

    EthIf_RxCbk = NULL;
    EthIf_TxCbk = NULL;

    /* Initialize the underlying Ethernet driver */
    ethernet_init();

    EthIf_Initialized = TRUE;

    printf("[EthIf] Initialized\n");
}

Std_ReturnType EthIf_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SET_CONTROLLER_MODE, ETHIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SET_CONTROLLER_MODE, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if ((CtrlMode != ETH_MODE_DOWN) && (CtrlMode != ETH_MODE_ACTIVE))
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_SET_CONTROLLER_MODE, ETHIF_E_INV_PARAM);
        return E_NOT_OK;
    }
#endif

    EthIf_CtrlState[CtrlIdx].Mode = CtrlMode;

    printf("[EthIf] Controller %u mode set to %s\n",
           CtrlIdx,
           (CtrlMode == ETH_MODE_ACTIVE) ? "ACTIVE" : "DOWN");

    return E_OK;
}

Std_ReturnType EthIf_GetControllerMode(uint8 CtrlIdx, Eth_ModeType* CtrlModePtr)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_CONTROLLER_MODE, ETHIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_CONTROLLER_MODE, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (CtrlModePtr == NULL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_CONTROLLER_MODE, ETHIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *CtrlModePtr = EthIf_CtrlState[CtrlIdx].Mode;
    return E_OK;
}

void EthIf_GetPhysAddr(uint8 CtrlIdx, uint8* PhysAddrPtr)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_PHYS_ADDR, ETHIF_E_NOT_INITIALIZED);
        return;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_PHYS_ADDR, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if (PhysAddrPtr == NULL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_PHYS_ADDR, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif

    (void)memcpy(PhysAddrPtr, EthIf_CtrlState[CtrlIdx].PhysAddr, 6U);
}

BufReq_ReturnType EthIf_ProvideTxBuffer(uint8 CtrlIdx,
                                        Eth_FrameType FrameType,
                                        uint8 Priority,
                                        Eth_BufIdxType* BufIdxPtr,
                                        uint8** BufPtr,
                                        uint16* LenBytePtr)
{
    uint8 idx;

    (void)Priority;

#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_PROVIDE_TX_BUFFER, ETHIF_E_NOT_INITIALIZED);
        return BUFREQ_E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_PROVIDE_TX_BUFFER, ETHIF_E_INV_CTRL_IDX);
        return BUFREQ_E_NOT_OK;
    }
    if ((BufIdxPtr == NULL) || (BufPtr == NULL) || (LenBytePtr == NULL))
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_PROVIDE_TX_BUFFER, ETHIF_E_PARAM_POINTER);
        return BUFREQ_E_NOT_OK;
    }
    if (EthIf_CtrlState[CtrlIdx].Mode != ETH_MODE_ACTIVE)
    {
        return BUFREQ_E_NOT_OK;
    }
#endif

    /* Check requested length fits in a buffer */
    if (*LenBytePtr > ETHIF_TX_BUF_SIZE)
    {
        return BUFREQ_E_OVFL;
    }

    /* Find a free buffer */
    for (idx = 0U; idx < ETHIF_TX_BUF_COUNT; idx++)
    {
        if (EthIf_TxBufPool[idx].State == ETHIF_BUF_FREE)
        {
            EthIf_TxBufPool[idx].State     = ETHIF_BUF_ALLOCATED;
            EthIf_TxBufPool[idx].FrameType = FrameType;
            EthIf_TxBufPool[idx].Length     = *LenBytePtr;

            *BufIdxPtr  = idx;
            *BufPtr     = EthIf_TxBufPool[idx].Data;
            /* Grant the requested length (capped at buffer size) */
            if (*LenBytePtr > ETHIF_TX_BUF_SIZE)
            {
                *LenBytePtr = ETHIF_TX_BUF_SIZE;
            }
            return BUFREQ_OK;
        }
    }

    return BUFREQ_E_BUSY;
}

Std_ReturnType EthIf_Transmit(uint8 CtrlIdx,
                              Eth_BufIdxType BufIdx,
                              Eth_FrameType FrameType,
                              boolean TxConfirmation,
                              uint16 LenByte,
                              const uint8* PhysAddrPtr)
{
    Std_ReturnType result;

    (void)PhysAddrPtr;

#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_CTRL_IDX);
        return E_NOT_OK;
    }
    if (BufIdx >= ETHIF_TX_BUF_COUNT)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_PARAM);
        return E_NOT_OK;
    }
    if (EthIf_TxBufPool[BufIdx].State != ETHIF_BUF_ALLOCATED)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_INV_PARAM);
        return E_NOT_OK;
    }
    if (EthIf_CtrlState[CtrlIdx].Mode != ETH_MODE_ACTIVE)
    {
        EthIf_TxBufPool[BufIdx].State = ETHIF_BUF_FREE;
        return E_NOT_OK;
    }
#endif

    /* Use FrameType as the PDU ID for the underlying ethernet_send */
    result = ethernet_send((unsigned short)FrameType,
                           EthIf_TxBufPool[BufIdx].Data,
                           LenByte);

    if (result == E_OK)
    {
        EthIf_CtrlState[CtrlIdx].TxCount++;

        if (TxConfirmation == TRUE)
        {
            EthIf_TxBufPool[BufIdx].State            = ETHIF_BUF_TX_PENDING;
            EthIf_TxBufPool[BufIdx].ConfirmRequested  = TRUE;
        }
        else
        {
            EthIf_TxBufPool[BufIdx].State = ETHIF_BUF_FREE;
        }
    }
    else
    {
        EthIf_CtrlState[CtrlIdx].TxErrCount++;
        EthIf_TxBufPool[BufIdx].State = ETHIF_BUF_FREE;
    }

    return result;
}

void EthIf_RxIndication(uint8 CtrlIdx,
                        Eth_FrameType FrameType,
                        boolean IsBroadcast,
                        const uint8* PhysAddrPtr,
                        const uint8* DataPtr,
                        uint16 LenByte)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RX_INDICATION, ETHIF_E_NOT_INITIALIZED);
        return;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RX_INDICATION, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if (DataPtr == NULL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RX_INDICATION, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif

    EthIf_CtrlState[CtrlIdx].RxCount++;

    /* Forward to registered upper-layer callback */
    if (EthIf_RxCbk != NULL)
    {
        EthIf_RxCbk(CtrlIdx, FrameType, IsBroadcast, PhysAddrPtr, DataPtr, LenByte);
    }
}

void EthIf_TxConfirmation(uint8 CtrlIdx, Eth_BufIdxType BufIdx)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (EthIf_Initialized == FALSE)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TX_CONFIRMATION, ETHIF_E_NOT_INITIALIZED);
        return;
    }
    if (CtrlIdx >= ETHIF_MAX_CTRL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TX_CONFIRMATION, ETHIF_E_INV_CTRL_IDX);
        return;
    }
    if (BufIdx >= ETHIF_TX_BUF_COUNT)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TX_CONFIRMATION, ETHIF_E_INV_PARAM);
        return;
    }
#endif

    /* Release the buffer */
    EthIf_TxBufPool[BufIdx].State           = ETHIF_BUF_FREE;
    EthIf_TxBufPool[BufIdx].ConfirmRequested = FALSE;

    /* Notify upper layer */
    if (EthIf_TxCbk != NULL)
    {
        EthIf_TxCbk(CtrlIdx, BufIdx);
    }
}

void EthIf_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL)
    {
        (void)Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_VERSION_INFO, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID         = ETHIF_VENDOR_ID;
    VersionInfoPtr->moduleID         = ETHIF_MODULE_ID;
    VersionInfoPtr->sw_major_version = ETHIF_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = ETHIF_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = ETHIF_SW_PATCH_VERSION;
}

void EthIf_MainFunctionRx(void)
{
    static uint8 rxBuffer[ETHIF_RX_BUF_SIZE];
    uint16 rxId;
    uint16 actualSize;
    Std_ReturnType result;

    if (EthIf_Initialized == FALSE)
    {
        return;
    }

    if (EthIf_CtrlState[0].Mode != ETH_MODE_ACTIVE)
    {
        return;
    }

    /* Poll the underlying Ethernet driver for received data */
    result = ethernet_receive(rxBuffer, ETHIF_RX_BUF_SIZE, &rxId, &actualSize);

    if (result == E_OK)
    {
        /* Forward via RxIndication with FrameType = rxId */
        EthIf_RxIndication(0U,
                           (Eth_FrameType)rxId,
                           FALSE,
                           NULL,
                           rxBuffer,
                           actualSize);
    }
}

void EthIf_MainFunctionTx(void)
{
    uint8 idx;

    if (EthIf_Initialized == FALSE)
    {
        return;
    }

    /* Process pending Tx confirmations */
    for (idx = 0U; idx < ETHIF_TX_BUF_COUNT; idx++)
    {
        if (EthIf_TxBufPool[idx].State == ETHIF_BUF_TX_PENDING)
        {
            /* Confirm and release */
            EthIf_TxConfirmation(0U, idx);
        }
    }
}

void EthIf_SetRxIndicationCallback(EthIf_RxIndicationCbkType Callback)
{
    EthIf_RxCbk = Callback;
}

void EthIf_SetTxConfirmationCallback(EthIf_TxConfirmationCbkType Callback)
{
    EthIf_TxCbk = Callback;
}
