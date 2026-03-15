#include "NvM.h"
#include "Dem.h"
#include "Det.h"
#include "MemIf.h"
#include <string.h>

#define NVM_CRC_LENGTH                       ((uint16)4U)
#define NVM_MAX_PAYLOAD_LENGTH               ((uint16)500U)
#define NVM_MAX_NV_BLOCK_LENGTH              (NVM_MAX_PAYLOAD_LENGTH + NVM_CRC_LENGTH)
#define NVM_HIGH_PRIO_QUEUE_SIZE             ((uint8)4U)
#define NVM_NORMAL_PRIO_QUEUE_SIZE           ((uint8)8U)

#define NVM_PHY_BLOCK_DEM_PRIMARY            ((uint16)1U)
#define NVM_PHY_BLOCK_GATEWAY_HEALTH         ((uint16)16U)
#define NVM_PHY_BLOCK_ECUM_DATASET_BASE      ((uint16)24U)

typedef enum
{
    NVM_BLOCK_NATIVE = 0,
    NVM_BLOCK_REDUNDANT,
    NVM_BLOCK_DATASET
} NvM_BlockManagementType_Type;

typedef enum
{
    NVM_INT_OP_NONE = 0,
    NVM_INT_OP_READ_BLOCK,
    NVM_INT_OP_WRITE_BLOCK,
    NVM_INT_OP_INVALIDATE_BLOCK,
    NVM_INT_OP_ERASE_BLOCK,
    NVM_INT_OP_READ_ALL,
    NVM_INT_OP_WRITE_ALL
} NvM_InternalOperation_Type;

typedef enum
{
    NVM_INT_STATE_IDLE = 0,
    NVM_INT_STATE_ISSUE,
    NVM_INT_STATE_WAIT_MEMIF
} NvM_InternalState_Type;

typedef struct
{
    NvM_BlockIdType BlockId;
    uint16 PayloadLength;
    uint16 PhysicalBlockBaseId;
    uint8 DeviceIndex;
    NvM_BlockManagementType_Type BlockManagementType;
    uint8 DatasetSize;
    boolean SelectForReadAll;
    boolean SelectForWriteAll;
    boolean WriteVerification;
    uint8* RamBlockDataAddress;
    const uint8* RomBlockDataAddress;
} NvM_BlockDescriptorType;

typedef struct
{
    NvM_RequestResultType RequestResult;
    boolean RamBlockValid;
    boolean NvBlockValid;
    boolean RamBlockChanged;
    uint8 DataIndex;
} NvM_BlockAdminType;

typedef struct
{
    uint16 BlockIndex;
    NvM_InternalOperation_Type Operation;
    uint8* DstPtr;
    const uint8* SrcPtr;
} NvM_QueuedRequestType;

static uint8 NvM_Initialized = FALSE;
static NvM_InternalOperation_Type NvM_InternalOperation = NVM_INT_OP_NONE;
static NvM_InternalState_Type NvM_InternalState = NVM_INT_STATE_IDLE;
static uint16 NvM_ScanIndex = 0U;
static uint16 NvM_CurrentBlockIndex = 0U;
static uint8 NvM_CurrentCopyIndex = 0U;
static uint8 NvM_VerifyReadPending = FALSE;
static uint8 NvM_TargetIsMultiBlock = FALSE;
static uint8 NvM_AnyCopySucceeded = FALSE;
static uint8* NvM_CurrentDstPtr = NULL;
static const uint8* NvM_CurrentSrcPtr = NULL;
static uint8 NvM_HighPrioQueueHead = 0U;
static uint8 NvM_HighPrioQueueTail = 0U;
static uint8 NvM_HighPrioQueueCount = 0U;
static uint8 NvM_NormalPrioQueueHead = 0U;
static uint8 NvM_NormalPrioQueueTail = 0U;
static uint8 NvM_NormalPrioQueueCount = 0U;

static uint8 NvM_GatewayHealthRamBlock[16] = {0};
static const uint8 NvM_GatewayHealthDefaults[16] = {0};
static uint32 NvM_EcuMDataDatasetRam = 0UL;
static const uint8 NvM_EcuMDataDatasetDefaults[sizeof(uint32)] = {0U, 0U, 0U, 0U};

static uint8 NvM_WriteBuffer[NVM_MAX_NV_BLOCK_LENGTH];
static uint8 NvM_ReadBuffer[NVM_MAX_NV_BLOCK_LENGTH];
static uint8 NvM_VerifyBuffer[NVM_MAX_NV_BLOCK_LENGTH];
static NvM_QueuedRequestType NvM_HighPrioQueue[NVM_HIGH_PRIO_QUEUE_SIZE];
static NvM_QueuedRequestType NvM_NormalPrioQueue[NVM_NORMAL_PRIO_QUEUE_SIZE];

extern Dem_DtcRecordType Dem_DtcStorage[DEM_MAX_NUMBER_OF_DTCS];

static const uint8 NvM_DemDtcStorageDefaults[sizeof(Dem_DtcRecordType) * DEM_MAX_NUMBER_OF_DTCS] = {0};

static const NvM_BlockDescriptorType NvM_BlockDescriptors[NVM_NUM_OF_NVRAM_BLOCKS] =
{
    {
        NVM_BLOCK_ID_DEM_DTC_STORAGE,
        (uint16)(sizeof(Dem_DtcRecordType) * DEM_MAX_NUMBER_OF_DTCS),
        NVM_PHY_BLOCK_DEM_PRIMARY,
        MEMIF_FEE_DEVICE_INDEX,
        NVM_BLOCK_REDUNDANT,
        (uint8)1U,
        TRUE,
        TRUE,
        TRUE,
        (uint8*)Dem_DtcStorage,
        NvM_DemDtcStorageDefaults
    },
    {
        NVM_BLOCK_ID_GATEWAY_HEALTH,
        (uint16)sizeof(NvM_GatewayHealthRamBlock),
        NVM_PHY_BLOCK_GATEWAY_HEALTH,
        MEMIF_EA_DEVICE_INDEX,
        NVM_BLOCK_NATIVE,
        (uint8)1U,
        TRUE,
        TRUE,
        TRUE,
        NvM_GatewayHealthRamBlock,
        NvM_GatewayHealthDefaults
    },
    {
        NVM_BLOCK_ID_ECUM_DATASET,
        (uint16)sizeof(uint32),
        NVM_PHY_BLOCK_ECUM_DATASET_BASE,
        MEMIF_EA_DEVICE_INDEX,
        NVM_BLOCK_DATASET,
        (uint8)2U,
        TRUE,
        TRUE,
        TRUE,
        (uint8*)&NvM_EcuMDataDatasetRam,
        NvM_EcuMDataDatasetDefaults
    }
};

static NvM_BlockAdminType NvM_BlockAdmin[NVM_NUM_OF_NVRAM_BLOCKS];

static void NvM_MemIfJobEndNotification(void)
{
    /* Kept for vendor-style hook compatibility. */
}

static void NvM_MemIfJobErrorNotification(void)
{
    /* Kept for vendor-style hook compatibility. */
}

static boolean NvM_HasQueuedRequests(void)
{
    return ((NvM_HighPrioQueueCount != 0U) || (NvM_NormalPrioQueueCount != 0U)) ? TRUE : FALSE;
}

static Std_ReturnType NvM_EnqueueRequest(
    uint16 BlockIndex,
    NvM_InternalOperation_Type Operation,
    uint8* DstPtr,
    const uint8* SrcPtr
)
{
    NvM_QueuedRequestType Request;

    Request.BlockIndex = BlockIndex;
    Request.Operation = Operation;
    Request.DstPtr = DstPtr;
    Request.SrcPtr = SrcPtr;

    if ((Operation == NVM_INT_OP_WRITE_BLOCK) ||
        (Operation == NVM_INT_OP_INVALIDATE_BLOCK) ||
        (Operation == NVM_INT_OP_ERASE_BLOCK))
    {
        if (NvM_HighPrioQueueCount >= NVM_HIGH_PRIO_QUEUE_SIZE)
        {
            return E_NOT_OK;
        }
        NvM_HighPrioQueue[NvM_HighPrioQueueTail] = Request;
        NvM_HighPrioQueueTail = (uint8)((NvM_HighPrioQueueTail + 1U) % NVM_HIGH_PRIO_QUEUE_SIZE);
        NvM_HighPrioQueueCount++;
        return E_OK;
    }

    if (NvM_NormalPrioQueueCount >= NVM_NORMAL_PRIO_QUEUE_SIZE)
    {
        return E_NOT_OK;
    }
    NvM_NormalPrioQueue[NvM_NormalPrioQueueTail] = Request;
    NvM_NormalPrioQueueTail = (uint8)((NvM_NormalPrioQueueTail + 1U) % NVM_NORMAL_PRIO_QUEUE_SIZE);
    NvM_NormalPrioQueueCount++;
    return E_OK;
}

static boolean NvM_DequeueRequest(NvM_QueuedRequestType* RequestPtr)
{
    if (RequestPtr == NULL)
    {
        return FALSE;
    }

    if (NvM_HighPrioQueueCount != 0U)
    {
        *RequestPtr = NvM_HighPrioQueue[NvM_HighPrioQueueHead];
        NvM_HighPrioQueueHead = (uint8)((NvM_HighPrioQueueHead + 1U) % NVM_HIGH_PRIO_QUEUE_SIZE);
        NvM_HighPrioQueueCount--;
        return TRUE;
    }

    if (NvM_NormalPrioQueueCount != 0U)
    {
        *RequestPtr = NvM_NormalPrioQueue[NvM_NormalPrioQueueHead];
        NvM_NormalPrioQueueHead = (uint8)((NvM_NormalPrioQueueHead + 1U) % NVM_NORMAL_PRIO_QUEUE_SIZE);
        NvM_NormalPrioQueueCount--;
        return TRUE;
    }

    return FALSE;
}

static uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length)
{
    uint32 Crc = 0xFFFFFFFFUL;
    uint16 Index;
    uint8 Bit;

    for (Index = 0U; Index < Length; Index++)
    {
        Crc ^= (uint32)DataPtr[Index];
        for (Bit = 0U; Bit < 8U; Bit++)
        {
            if ((Crc & 1UL) != 0UL)
            {
                Crc = (Crc >> 1U) ^ 0xEDB88320UL;
            }
            else
            {
                Crc >>= 1U;
            }
        }
    }

    return (Crc ^ 0xFFFFFFFFUL);
}

static void NvM_WriteUint32Le(uint8* BufferPtr, uint32 Value)
{
    BufferPtr[0] = (uint8)(Value & 0xFFUL);
    BufferPtr[1] = (uint8)((Value >> 8U) & 0xFFUL);
    BufferPtr[2] = (uint8)((Value >> 16U) & 0xFFUL);
    BufferPtr[3] = (uint8)((Value >> 24U) & 0xFFUL);
}

static uint32 NvM_ReadUint32Le(const uint8* BufferPtr)
{
    return ((uint32)BufferPtr[0]) |
           (((uint32)BufferPtr[1]) << 8U) |
           (((uint32)BufferPtr[2]) << 16U) |
           (((uint32)BufferPtr[3]) << 24U);
}

static uint16 NvM_GetNvLength(const NvM_BlockDescriptorType* DescriptorPtr)
{
    return (uint16)(DescriptorPtr->PayloadLength + NVM_CRC_LENGTH);
}

static sint16 NvM_GetBlockIndexById(NvM_BlockIdType BlockId)
{
    uint16 Index;

    for (Index = 0U; Index < NVM_NUM_OF_NVRAM_BLOCKS; Index++)
    {
        if (NvM_BlockDescriptors[Index].BlockId == BlockId)
        {
            return (sint16)Index;
        }
    }

    return (sint16)-1;
}

static Std_ReturnType NvM_CheckInitializedAndBlock(NvM_BlockIdType BlockId, uint8 Sid, sint16* BlockIndexPtr)
{
    sint16 LocalIndex;

    if (NvM_Initialized == FALSE)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, Sid, NVM_E_UNINIT);
        return E_NOT_OK;
    }

    LocalIndex = NvM_GetBlockIndexById(BlockId);
    if (LocalIndex < 0)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, Sid, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }

    *BlockIndexPtr = LocalIndex;
    return E_OK;
}

static uint16 NvM_GetPhysicalBlockId(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = DescriptorPtr->PhysicalBlockBaseId;

    if (DescriptorPtr->BlockManagementType == NVM_BLOCK_REDUNDANT)
    {
        PhysicalId = (uint16)(DescriptorPtr->PhysicalBlockBaseId + CopyIndex);
    }
    else if (DescriptorPtr->BlockManagementType == NVM_BLOCK_DATASET)
    {
        PhysicalId = (uint16)(DescriptorPtr->PhysicalBlockBaseId + NvM_BlockAdmin[BlockIndex].DataIndex);
    }

    return PhysicalId;
}

static uint8 NvM_GetRequiredCopies(uint16 BlockIndex)
{
    if (NvM_BlockDescriptors[BlockIndex].BlockManagementType == NVM_BLOCK_REDUNDANT)
    {
        return (uint8)2U;
    }
    return (uint8)1U;
}

static void NvM_CopyDefaultsToRam(uint16 BlockIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];

    if (DescriptorPtr->RamBlockDataAddress == NULL)
    {
        return;
    }

    if (DescriptorPtr->RomBlockDataAddress != NULL)
    {
        (void)memcpy(DescriptorPtr->RamBlockDataAddress, DescriptorPtr->RomBlockDataAddress, DescriptorPtr->PayloadLength);
    }
    else
    {
        (void)memset(DescriptorPtr->RamBlockDataAddress, 0, DescriptorPtr->PayloadLength);
    }
}

static void NvM_PrepareWriteBuffer(uint16 BlockIndex, const uint8* SourcePtr)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint32 Crc;

    (void)memcpy(NvM_WriteBuffer, SourcePtr, DescriptorPtr->PayloadLength);
    Crc = NvM_CalculateCrc32(SourcePtr, DescriptorPtr->PayloadLength);
    NvM_WriteUint32Le(&NvM_WriteBuffer[DescriptorPtr->PayloadLength], Crc);
}

static Std_ReturnType NvM_ValidateReadBuffer(uint16 BlockIndex, uint8* DestPtr)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint32 ExpectedCrc;
    uint32 CalculatedCrc;

    ExpectedCrc = NvM_ReadUint32Le(&NvM_ReadBuffer[DescriptorPtr->PayloadLength]);
    CalculatedCrc = NvM_CalculateCrc32(NvM_ReadBuffer, DescriptorPtr->PayloadLength);

    if (ExpectedCrc != CalculatedCrc)
    {
        return E_NOT_OK;
    }

    (void)memcpy(DestPtr, NvM_ReadBuffer, DescriptorPtr->PayloadLength);
    return E_OK;
}

static void NvM_FinishOperation(void)
{
    NvM_InternalOperation = NVM_INT_OP_NONE;
    NvM_InternalState = NVM_INT_STATE_IDLE;
    NvM_ScanIndex = 0U;
    NvM_CurrentBlockIndex = 0U;
    NvM_CurrentCopyIndex = 0U;
    NvM_VerifyReadPending = FALSE;
    NvM_TargetIsMultiBlock = FALSE;
    NvM_AnyCopySucceeded = FALSE;
    NvM_CurrentDstPtr = NULL;
    NvM_CurrentSrcPtr = NULL;
}

static Std_ReturnType NvM_IssueCurrentRead(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = NvM_GetPhysicalBlockId(BlockIndex, CopyIndex);

    return MemIf_Read(
        DescriptorPtr->DeviceIndex,
        PhysicalId,
        0U,
        NvM_ReadBuffer,
        NvM_GetNvLength(DescriptorPtr)
    );
}

static Std_ReturnType NvM_IssueCurrentWrite(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = NvM_GetPhysicalBlockId(BlockIndex, CopyIndex);

    return MemIf_Write(
        DescriptorPtr->DeviceIndex,
        PhysicalId,
        NvM_WriteBuffer,
        NvM_GetNvLength(DescriptorPtr)
    );
}

static Std_ReturnType NvM_IssueCurrentVerifyRead(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = NvM_GetPhysicalBlockId(BlockIndex, CopyIndex);

    return MemIf_Read(
        DescriptorPtr->DeviceIndex,
        PhysicalId,
        0U,
        NvM_VerifyBuffer,
        NvM_GetNvLength(DescriptorPtr)
    );
}

static Std_ReturnType NvM_IssueCurrentInvalidate(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = NvM_GetPhysicalBlockId(BlockIndex, CopyIndex);

    return MemIf_InvalidateBlock(DescriptorPtr->DeviceIndex, PhysicalId);
}

static Std_ReturnType NvM_IssueCurrentErase(uint16 BlockIndex, uint8 CopyIndex)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    uint16 PhysicalId = NvM_GetPhysicalBlockId(BlockIndex, CopyIndex);

    return MemIf_EraseImmediateBlock(DescriptorPtr->DeviceIndex, PhysicalId);
}

static Std_ReturnType NvM_StartBlockOperation(uint16 BlockIndex, NvM_InternalOperation_Type Operation, boolean IsMultiBlock)
{
    const NvM_BlockDescriptorType* DescriptorPtr = &NvM_BlockDescriptors[BlockIndex];
    const uint8* SourcePtr;

    NvM_CurrentBlockIndex = BlockIndex;
    NvM_CurrentCopyIndex = 0U;
    NvM_VerifyReadPending = FALSE;
    NvM_TargetIsMultiBlock = IsMultiBlock;
    NvM_AnyCopySucceeded = FALSE;

    if ((Operation == NVM_INT_OP_READ_BLOCK) || (Operation == NVM_INT_OP_READ_ALL))
    {
        return NvM_IssueCurrentRead(BlockIndex, 0U);
    }

    if ((Operation == NVM_INT_OP_WRITE_BLOCK) || (Operation == NVM_INT_OP_WRITE_ALL))
    {
        SourcePtr = (NvM_CurrentSrcPtr != NULL) ? NvM_CurrentSrcPtr : DescriptorPtr->RamBlockDataAddress;
        if (SourcePtr == NULL)
        {
            return E_NOT_OK;
        }
        NvM_PrepareWriteBuffer(BlockIndex, SourcePtr);
        return NvM_IssueCurrentWrite(BlockIndex, 0U);
    }

    if (Operation == NVM_INT_OP_INVALIDATE_BLOCK)
    {
        return NvM_IssueCurrentInvalidate(BlockIndex, 0U);
    }

    if (Operation == NVM_INT_OP_ERASE_BLOCK)
    {
        return NvM_IssueCurrentErase(BlockIndex, 0U);
    }

    return E_NOT_OK;
}

void NvM_Init(void)
{
    uint16 Index;

    for (Index = 0U; Index < NVM_NUM_OF_NVRAM_BLOCKS; Index++)
    {
        NvM_BlockAdmin[Index].RequestResult = NVM_REQ_NOT_OK;
        NvM_BlockAdmin[Index].RamBlockValid = FALSE;
        NvM_BlockAdmin[Index].NvBlockValid = FALSE;
        NvM_BlockAdmin[Index].RamBlockChanged = FALSE;
        NvM_BlockAdmin[Index].DataIndex = 0U;

        if ((NvM_BlockDescriptors[Index].PayloadLength == 0U) ||
            (NvM_GetNvLength(&NvM_BlockDescriptors[Index]) > NVM_MAX_NV_BLOCK_LENGTH))
        {
            NvM_BlockAdmin[Index].RequestResult = NVM_REQ_NOT_OK;
            continue;
        }

        NvM_CopyDefaultsToRam(Index);
        NvM_BlockAdmin[Index].RamBlockValid = TRUE;
    }

    NvM_Initialized = TRUE;
    NvM_HighPrioQueueHead = 0U;
    NvM_HighPrioQueueTail = 0U;
    NvM_HighPrioQueueCount = 0U;
    NvM_NormalPrioQueueHead = 0U;
    NvM_NormalPrioQueueTail = 0U;
    NvM_NormalPrioQueueCount = 0U;
    (void)MemIf_SetJobNotifications(
        MEMIF_FEE_DEVICE_INDEX,
        NvM_MemIfJobEndNotification,
        NvM_MemIfJobErrorNotification
    );
    (void)MemIf_SetJobNotifications(
        MEMIF_EA_DEVICE_INDEX,
        NvM_MemIfJobEndNotification,
        NvM_MemIfJobErrorNotification
    );
    NvM_FinishOperation();
}

Std_ReturnType NvM_ReadAll(void)
{
    uint16 Index;

    if (NvM_Initialized == FALSE)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_READ_ALL, NVM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((NvM_InternalState != NVM_INT_STATE_IDLE) || (NvM_HasQueuedRequests() == TRUE))
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_READ_ALL, NVM_E_BUSY);
        return E_NOT_OK;
    }

    for (Index = 0U; Index < NVM_NUM_OF_NVRAM_BLOCKS; Index++)
    {
        NvM_BlockAdmin[Index].RequestResult =
            (NvM_BlockDescriptors[Index].SelectForReadAll == TRUE) ? NVM_REQ_PENDING : NVM_REQ_BLOCK_SKIPPED;
    }

    NvM_InternalOperation = NVM_INT_OP_READ_ALL;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_ScanIndex = 0U;
    NvM_TargetIsMultiBlock = TRUE;

    return E_OK;
}

Std_ReturnType NvM_WriteAll(void)
{
    uint16 Index;

    if (NvM_Initialized == FALSE)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_WRITE_ALL, NVM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((NvM_InternalState != NVM_INT_STATE_IDLE) || (NvM_HasQueuedRequests() == TRUE))
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_WRITE_ALL, NVM_E_BUSY);
        return E_NOT_OK;
    }

    for (Index = 0U; Index < NVM_NUM_OF_NVRAM_BLOCKS; Index++)
    {
        NvM_BlockAdmin[Index].RequestResult =
            (NvM_BlockDescriptors[Index].SelectForWriteAll == TRUE) ? NVM_REQ_PENDING : NVM_REQ_BLOCK_SKIPPED;
    }

    NvM_InternalOperation = NVM_INT_OP_WRITE_ALL;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_ScanIndex = 0U;
    NvM_TargetIsMultiBlock = TRUE;

    return E_OK;
}

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    sint16 BlockIndex;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_READ_BLOCK, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_InternalState != NVM_INT_STATE_IDLE)
    {
        if (NvM_EnqueueRequest((uint16)BlockIndex, NVM_INT_OP_READ_BLOCK, (uint8*)NvM_DstPtr, NULL) != E_OK)
        {
            (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_READ_BLOCK, NVM_E_BUSY);
            return E_NOT_OK;
        }
        return E_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RequestResult = NVM_REQ_PENDING;
    NvM_InternalOperation = NVM_INT_OP_READ_BLOCK;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_CurrentDstPtr = (uint8*)NvM_DstPtr;
    NvM_CurrentSrcPtr = NULL;
    NvM_CurrentBlockIndex = (uint16)BlockIndex;
    NvM_TargetIsMultiBlock = FALSE;

    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    sint16 BlockIndex;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_WRITE_BLOCK, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_InternalState != NVM_INT_STATE_IDLE)
    {
        if (NvM_EnqueueRequest((uint16)BlockIndex, NVM_INT_OP_WRITE_BLOCK, NULL, (const uint8*)NvM_SrcPtr) != E_OK)
        {
            (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_WRITE_BLOCK, NVM_E_BUSY);
            return E_NOT_OK;
        }
        return E_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RequestResult = NVM_REQ_PENDING;
    NvM_BlockAdmin[(uint16)BlockIndex].RamBlockChanged = TRUE;
    NvM_InternalOperation = NVM_INT_OP_WRITE_BLOCK;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_CurrentDstPtr = NULL;
    NvM_CurrentSrcPtr = (const uint8*)NvM_SrcPtr;
    NvM_CurrentBlockIndex = (uint16)BlockIndex;
    NvM_TargetIsMultiBlock = FALSE;

    return E_OK;
}

Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    sint16 BlockIndex;
    uint8* TargetPtr;
    const NvM_BlockDescriptorType* DescriptorPtr;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_RESTORE_BLOCK_DEFAULTS, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_InternalState != NVM_INT_STATE_IDLE)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_RESTORE_BLOCK_DEFAULTS, NVM_E_BUSY);
        return E_NOT_OK;
    }

    DescriptorPtr = &NvM_BlockDescriptors[(uint16)BlockIndex];
    TargetPtr = (NvM_DstPtr != NULL) ? (uint8*)NvM_DstPtr : DescriptorPtr->RamBlockDataAddress;
    if (TargetPtr == NULL)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_RESTORE_BLOCK_DEFAULTS, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (DescriptorPtr->RomBlockDataAddress != NULL)
    {
        (void)memcpy(TargetPtr, DescriptorPtr->RomBlockDataAddress, DescriptorPtr->PayloadLength);
    }
    else
    {
        (void)memset(TargetPtr, 0, DescriptorPtr->PayloadLength);
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RamBlockValid = TRUE;
    NvM_BlockAdmin[(uint16)BlockIndex].RamBlockChanged = TRUE;
    NvM_BlockAdmin[(uint16)BlockIndex].RequestResult = NVM_REQ_OK;

    return E_OK;
}

Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)
{
    sint16 BlockIndex;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_INVALIDATE_NV_BLOCK, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_InternalState != NVM_INT_STATE_IDLE)
    {
        if (NvM_EnqueueRequest((uint16)BlockIndex, NVM_INT_OP_INVALIDATE_BLOCK, NULL, NULL) != E_OK)
        {
            (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_INVALIDATE_NV_BLOCK, NVM_E_BUSY);
            return E_NOT_OK;
        }
        return E_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RequestResult = NVM_REQ_PENDING;
    NvM_InternalOperation = NVM_INT_OP_INVALIDATE_BLOCK;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_CurrentBlockIndex = (uint16)BlockIndex;
    NvM_TargetIsMultiBlock = FALSE;

    return E_OK;
}

Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId)
{
    sint16 BlockIndex;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_ERASE_NV_BLOCK, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_InternalState != NVM_INT_STATE_IDLE)
    {
        if (NvM_EnqueueRequest((uint16)BlockIndex, NVM_INT_OP_ERASE_BLOCK, NULL, NULL) != E_OK)
        {
            (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_ERASE_NV_BLOCK, NVM_E_BUSY);
            return E_NOT_OK;
        }
        return E_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RequestResult = NVM_REQ_PENDING;
    NvM_InternalOperation = NVM_INT_OP_ERASE_BLOCK;
    NvM_InternalState = NVM_INT_STATE_ISSUE;
    NvM_CurrentBlockIndex = (uint16)BlockIndex;
    NvM_TargetIsMultiBlock = FALSE;

    return E_OK;
}

Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)
{
    sint16 BlockIndex;

    if (RequestResultPtr == NULL)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_GET_ERROR_STATUS, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_GET_ERROR_STATUS, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    *RequestResultPtr = NvM_BlockAdmin[(uint16)BlockIndex].RequestResult;
    return E_OK;
}

Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)
{
    sint16 BlockIndex;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_SET_RAM_BLOCK_STATUS, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].RamBlockChanged = BlockChanged;
    NvM_BlockAdmin[(uint16)BlockIndex].RamBlockValid = TRUE;

    return E_OK;
}

Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)
{
    sint16 BlockIndex;
    uint8 DatasetSize;

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_SET_DATA_INDEX, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_BlockDescriptors[(uint16)BlockIndex].BlockManagementType != NVM_BLOCK_DATASET)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_SET_DATA_INDEX, NVM_E_NOT_ALLOWED);
        return E_NOT_OK;
    }

    DatasetSize = NvM_BlockDescriptors[(uint16)BlockIndex].DatasetSize;
    if (DataIndex >= DatasetSize)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_SET_DATA_INDEX, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }

    NvM_BlockAdmin[(uint16)BlockIndex].DataIndex = DataIndex;
    return E_OK;
}

Std_ReturnType NvM_GetDataIndex(NvM_BlockIdType BlockId, uint8* DataIndexPtr)
{
    sint16 BlockIndex;

    if (DataIndexPtr == NULL)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_GET_DATA_INDEX, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_CheckInitializedAndBlock(BlockId, NVM_SID_GET_DATA_INDEX, &BlockIndex) != E_OK)
    {
        return E_NOT_OK;
    }

    if (NvM_BlockDescriptors[(uint16)BlockIndex].BlockManagementType != NVM_BLOCK_DATASET)
    {
        (void)Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, NVM_SID_GET_DATA_INDEX, NVM_E_NOT_ALLOWED);
        return E_NOT_OK;
    }

    *DataIndexPtr = NvM_BlockAdmin[(uint16)BlockIndex].DataIndex;
    return E_OK;
}

void NvM_MainFunction(void)
{
    const NvM_BlockDescriptorType* DescriptorPtr;
    NvM_BlockAdminType* AdminPtr;
    MemIf_StatusType MemStatus;
    MemIf_JobResultType JobResult;
    uint8 RequiredCopies;
    uint8* ReadTargetPtr;
    boolean BlockSucceeded = FALSE;

    if (NvM_Initialized == FALSE)
    {
        return;
    }

    MemIf_MainFunction();

    if (NvM_InternalState == NVM_INT_STATE_IDLE)
    {
        NvM_QueuedRequestType QueuedRequest;
        if (NvM_DequeueRequest(&QueuedRequest) == TRUE)
        {
            NvM_CurrentBlockIndex = QueuedRequest.BlockIndex;
            NvM_InternalOperation = QueuedRequest.Operation;
            NvM_CurrentDstPtr = QueuedRequest.DstPtr;
            NvM_CurrentSrcPtr = QueuedRequest.SrcPtr;
            NvM_CurrentCopyIndex = 0U;
            NvM_VerifyReadPending = FALSE;
            NvM_TargetIsMultiBlock = FALSE;
            NvM_AnyCopySucceeded = FALSE;
            NvM_InternalState = NVM_INT_STATE_ISSUE;
        }
        else
        {
            return;
        }
    }

    if (NvM_InternalState == NVM_INT_STATE_ISSUE)
    {
        if ((NvM_InternalOperation == NVM_INT_OP_READ_ALL) || (NvM_InternalOperation == NVM_INT_OP_WRITE_ALL))
        {
            while (NvM_ScanIndex < NVM_NUM_OF_NVRAM_BLOCKS)
            {
                boolean ShouldProcess = FALSE;
                if ((NvM_InternalOperation == NVM_INT_OP_READ_ALL) &&
                    (NvM_BlockDescriptors[NvM_ScanIndex].SelectForReadAll == TRUE))
                {
                    ShouldProcess = TRUE;
                }
                if ((NvM_InternalOperation == NVM_INT_OP_WRITE_ALL) &&
                    (NvM_BlockDescriptors[NvM_ScanIndex].SelectForWriteAll == TRUE))
                {
                    ShouldProcess = TRUE;
                }

                if (ShouldProcess == FALSE)
                {
                    NvM_BlockAdmin[NvM_ScanIndex].RequestResult = NVM_REQ_BLOCK_SKIPPED;
                    NvM_ScanIndex++;
                    continue;
                }

                if ((NvM_InternalOperation == NVM_INT_OP_WRITE_ALL) &&
                    (NvM_BlockAdmin[NvM_ScanIndex].RamBlockChanged == FALSE))
                {
                    NvM_BlockAdmin[NvM_ScanIndex].RequestResult = NVM_REQ_BLOCK_SKIPPED;
                    NvM_ScanIndex++;
                    continue;
                }

                if (NvM_StartBlockOperation(
                        NvM_ScanIndex,
                        (NvM_InternalOperation == NVM_INT_OP_READ_ALL) ? NVM_INT_OP_READ_BLOCK : NVM_INT_OP_WRITE_BLOCK,
                        TRUE
                    ) != E_OK)
                {
                    NvM_BlockAdmin[NvM_ScanIndex].RequestResult = NVM_REQ_NOT_OK;
                    NvM_ScanIndex++;
                    continue;
                }

                NvM_InternalState = NVM_INT_STATE_WAIT_MEMIF;
                return;
            }

            NvM_FinishOperation();
            return;
        }

        if (NvM_StartBlockOperation(NvM_CurrentBlockIndex, NvM_InternalOperation, FALSE) != E_OK)
        {
            NvM_BlockAdmin[NvM_CurrentBlockIndex].RequestResult = NVM_REQ_NOT_OK;
            NvM_FinishOperation();
            return;
        }

        NvM_InternalState = NVM_INT_STATE_WAIT_MEMIF;
        return;
    }

    DescriptorPtr = &NvM_BlockDescriptors[NvM_CurrentBlockIndex];
    AdminPtr = &NvM_BlockAdmin[NvM_CurrentBlockIndex];
    RequiredCopies = NvM_GetRequiredCopies(NvM_CurrentBlockIndex);

    MemStatus = MemIf_GetStatus(DescriptorPtr->DeviceIndex);
    if (MemStatus == MEMIF_BUSY)
    {
        return;
    }

    JobResult = MemIf_GetJobResult(DescriptorPtr->DeviceIndex);

    if ((NvM_InternalOperation == NVM_INT_OP_READ_BLOCK) || (NvM_InternalOperation == NVM_INT_OP_READ_ALL))
    {
        if (JobResult == MEMIF_JOB_OK)
        {
            ReadTargetPtr = (NvM_CurrentDstPtr != NULL) ? NvM_CurrentDstPtr : DescriptorPtr->RamBlockDataAddress;
            if ((ReadTargetPtr != NULL) && (NvM_ValidateReadBuffer(NvM_CurrentBlockIndex, ReadTargetPtr) == E_OK))
            {
                AdminPtr->RequestResult = NVM_REQ_OK;
                AdminPtr->RamBlockValid = TRUE;
                AdminPtr->NvBlockValid = TRUE;
                AdminPtr->RamBlockChanged = FALSE;
                BlockSucceeded = TRUE;
            }
            else
            {
                JobResult = MEMIF_BLOCK_INCONSISTENT;
            }
        }

        if (BlockSucceeded == FALSE)
        {
            NvM_CurrentCopyIndex++;
            if (NvM_CurrentCopyIndex < RequiredCopies)
            {
                NvM_InternalState = NVM_INT_STATE_ISSUE;
                return;
            }

            NvM_CopyDefaultsToRam(NvM_CurrentBlockIndex);
            AdminPtr->RamBlockValid = TRUE;
            AdminPtr->NvBlockValid = FALSE;
            AdminPtr->RamBlockChanged = TRUE;
            AdminPtr->RequestResult =
                (JobResult == MEMIF_BLOCK_INVALID) ? NVM_REQ_NV_INVALIDATED : NVM_REQ_INTEGRITY_FAILED;
        }
    }
    else if ((NvM_InternalOperation == NVM_INT_OP_WRITE_BLOCK) || (NvM_InternalOperation == NVM_INT_OP_WRITE_ALL))
    {
        if ((JobResult == MEMIF_JOB_OK) && (NvM_VerifyReadPending == FALSE) && (DescriptorPtr->WriteVerification == TRUE))
        {
            if (NvM_IssueCurrentVerifyRead(NvM_CurrentBlockIndex, NvM_CurrentCopyIndex) == E_OK)
            {
                NvM_VerifyReadPending = TRUE;
                return;
            }
            JobResult = MEMIF_JOB_FAILED;
        }

        if ((JobResult == MEMIF_JOB_OK) && (NvM_VerifyReadPending == TRUE))
        {
            if (memcmp(NvM_VerifyBuffer, NvM_WriteBuffer, NvM_GetNvLength(DescriptorPtr)) != 0)
            {
                JobResult = MEMIF_BLOCK_INCONSISTENT;
            }
            NvM_VerifyReadPending = FALSE;
        }

        if (JobResult == MEMIF_JOB_OK)
        {
            NvM_AnyCopySucceeded = TRUE;
            NvM_CurrentCopyIndex++;
            if (NvM_CurrentCopyIndex < RequiredCopies)
            {
                NvM_InternalState = NVM_INT_STATE_ISSUE;
                return;
            }

            AdminPtr->RequestResult = NVM_REQ_OK;
            AdminPtr->RamBlockValid = TRUE;
            AdminPtr->NvBlockValid = TRUE;
            AdminPtr->RamBlockChanged = FALSE;
        }
        else
        {
            NvM_CurrentCopyIndex++;
            NvM_VerifyReadPending = FALSE;
            if ((NvM_AnyCopySucceeded == FALSE) && (NvM_CurrentCopyIndex < RequiredCopies))
            {
                NvM_InternalState = NVM_INT_STATE_ISSUE;
                return;
            }

            AdminPtr->RequestResult = NVM_REQ_NOT_OK;
        }
    }
    else if (NvM_InternalOperation == NVM_INT_OP_INVALIDATE_BLOCK)
    {
        if ((JobResult == MEMIF_JOB_OK) || (JobResult == MEMIF_BLOCK_INVALID))
        {
            NvM_CurrentCopyIndex++;
            if (NvM_CurrentCopyIndex < RequiredCopies)
            {
                if (NvM_IssueCurrentInvalidate(NvM_CurrentBlockIndex, NvM_CurrentCopyIndex) == E_OK)
                {
                    return;
                }
                AdminPtr->RequestResult = NVM_REQ_NOT_OK;
            }
            else
            {
                AdminPtr->NvBlockValid = FALSE;
                AdminPtr->RequestResult = NVM_REQ_NV_INVALIDATED;
            }
        }
        else
        {
            AdminPtr->RequestResult = NVM_REQ_NOT_OK;
        }
    }
    else if (NvM_InternalOperation == NVM_INT_OP_ERASE_BLOCK)
    {
        if (JobResult == MEMIF_JOB_OK)
        {
            NvM_CurrentCopyIndex++;
            if (NvM_CurrentCopyIndex < RequiredCopies)
            {
                if (NvM_IssueCurrentErase(NvM_CurrentBlockIndex, NvM_CurrentCopyIndex) == E_OK)
                {
                    return;
                }
                AdminPtr->RequestResult = NVM_REQ_NOT_OK;
            }
            else
            {
                AdminPtr->NvBlockValid = FALSE;
                AdminPtr->RequestResult = NVM_REQ_OK;
            }
        }
        else
        {
            AdminPtr->RequestResult = NVM_REQ_NOT_OK;
        }
    }

    if (NvM_TargetIsMultiBlock == TRUE)
    {
        NvM_ScanIndex++;
        NvM_InternalState = NVM_INT_STATE_ISSUE;
        return;
    }

    NvM_FinishOperation();
}
