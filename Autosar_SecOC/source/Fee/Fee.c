#include "Fee.h"
#include "Det.h"
#include <stdio.h>
#include <string.h>

/* External API declarations (MISRA 8.4 visibility). */
void Fee_Init(void);
void Fee_MainFunction(void);
Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber);
Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber);
void Fee_SetMode(MemIf_ModeType Mode);
MemIf_StatusType Fee_GetStatus(void);
MemIf_JobResultType Fee_GetJobResult(void);
void Fee_SetJobEndNotification(Fee_JobNotificationType NotificationPtr);
void Fee_SetJobErrorNotification(Fee_JobNotificationType NotificationPtr);

#define FEE_STORAGE_MAGIC (0x33454546UL)
#define FEE_STORAGE_FILE_NAME "fee_storage.bin"
#define FEE_WL_SLOT_COUNT ((uint8)4U)

typedef enum
{
    FEE_JOB_NONE = 0,
    FEE_JOB_READ,
    FEE_JOB_WRITE,
    FEE_JOB_INVALIDATE,
    FEE_JOB_ERASE
} Fee_JobType_Type;

typedef struct
{
    Fee_JobType_Type JobType;
    uint16 BlockNumber;
    uint16 BlockOffset;
    uint16 Length;
    uint8* ReadPtr;
    const uint8* WritePtr;
} Fee_PendingJobType;

typedef struct
{
    uint32 Magic;
    uint32 WriteCount[FEE_MAX_BLOCKS];
    uint16 BlockLength[FEE_MAX_BLOCKS];
    uint8 BlockValid[FEE_MAX_BLOCKS];
    uint8 ActiveSlot[FEE_MAX_BLOCKS];
    uint8 Data[FEE_MAX_BLOCKS][FEE_WL_SLOT_COUNT][FEE_MAX_BLOCK_LENGTH];
} Fee_StorageImageType;

static uint8 Fee_Initialized = FALSE;
static MemIf_ModeType Fee_Mode = MEMIF_MODE_SLOW;
static MemIf_StatusType Fee_Status = MEMIF_UNINIT;
static MemIf_JobResultType Fee_JobResult = MEMIF_JOB_OK;
static Fee_PendingJobType Fee_PendingJob = {FEE_JOB_NONE, 0U, 0U, 0U, NULL, NULL};
static Fee_JobNotificationType Fee_JobEndNotification = (Fee_JobNotificationType)0;
static Fee_JobNotificationType Fee_JobErrorNotification = (Fee_JobNotificationType)0;
static uint8 Fee_Data[FEE_MAX_BLOCKS][FEE_WL_SLOT_COUNT][FEE_MAX_BLOCK_LENGTH];
static uint16 Fee_BlockLength[FEE_MAX_BLOCKS];
static uint8 Fee_BlockValid[FEE_MAX_BLOCKS];
static uint8 Fee_ActiveSlot[FEE_MAX_BLOCKS];
static uint32 Fee_WriteCount[FEE_MAX_BLOCKS];

static void Fee_LoadStorage(void)
{
    Fee_StorageImageType Image;
    FILE* FilePtr = fopen(FEE_STORAGE_FILE_NAME, "rb");
    if (FilePtr == NULL)
    {
        return;
    }
    if ((fread(&Image, sizeof(Image), 1U, FilePtr) == 1U) && (Image.Magic == FEE_STORAGE_MAGIC))
    {
        (void)memcpy(Fee_Data, Image.Data, sizeof(Fee_Data));
        (void)memcpy(Fee_BlockLength, Image.BlockLength, sizeof(Fee_BlockLength));
        (void)memcpy(Fee_BlockValid, Image.BlockValid, sizeof(Fee_BlockValid));
        (void)memcpy(Fee_ActiveSlot, Image.ActiveSlot, sizeof(Fee_ActiveSlot));
        (void)memcpy(Fee_WriteCount, Image.WriteCount, sizeof(Fee_WriteCount));
    }
    (void)fclose(FilePtr);
}

static void Fee_SaveStorage(void)
{
    Fee_StorageImageType Image;
    FILE* FilePtr;
    Image.Magic = FEE_STORAGE_MAGIC;
    (void)memcpy(Image.Data, Fee_Data, sizeof(Fee_Data));
    (void)memcpy(Image.BlockLength, Fee_BlockLength, sizeof(Fee_BlockLength));
    (void)memcpy(Image.BlockValid, Fee_BlockValid, sizeof(Fee_BlockValid));
    (void)memcpy(Image.ActiveSlot, Fee_ActiveSlot, sizeof(Fee_ActiveSlot));
    (void)memcpy(Image.WriteCount, Fee_WriteCount, sizeof(Fee_WriteCount));
    FilePtr = fopen(FEE_STORAGE_FILE_NAME, "wb");
    if (FilePtr == NULL)
    {
        return;
    }
    (void)fwrite(&Image, sizeof(Image), 1U, FilePtr);
    (void)fclose(FilePtr);
}

static void Fee_Notify(void)
{
    if (Fee_JobResult == MEMIF_JOB_OK)
    {
        if (Fee_JobEndNotification != (Fee_JobNotificationType)0) { Fee_JobEndNotification(); }
    }
    else
    {
        if (Fee_JobErrorNotification != (Fee_JobNotificationType)0) { Fee_JobErrorNotification(); }
    }
}

void Fee_Init(void)
{
    (void)memset(Fee_Data, 0xFF, sizeof(Fee_Data));
    (void)memset(Fee_BlockLength, 0, sizeof(Fee_BlockLength));
    (void)memset(Fee_BlockValid, 0, sizeof(Fee_BlockValid));
    (void)memset(Fee_ActiveSlot, 0, sizeof(Fee_ActiveSlot));
    (void)memset(Fee_WriteCount, 0, sizeof(Fee_WriteCount));
    Fee_LoadStorage();
    Fee_PendingJob.JobType = FEE_JOB_NONE;
    Fee_Initialized = TRUE;
    Fee_Mode = MEMIF_MODE_SLOW;
    Fee_Status = MEMIF_IDLE;
    Fee_JobResult = MEMIF_JOB_OK;
}

void Fee_MainFunction(void)
{
    uint8 Slot;
    if ((Fee_Initialized == FALSE) || (Fee_Status != MEMIF_BUSY)) { return; }
    switch (Fee_PendingJob.JobType)
    {
        case FEE_JOB_READ:
            Slot = Fee_ActiveSlot[Fee_PendingJob.BlockNumber];
            if ((Fee_BlockValid[Fee_PendingJob.BlockNumber] == FALSE) ||
                (Fee_BlockLength[Fee_PendingJob.BlockNumber] < (Fee_PendingJob.BlockOffset + Fee_PendingJob.Length)))
            { Fee_JobResult = MEMIF_BLOCK_INVALID; }
            else
            { (void)memcpy(Fee_PendingJob.ReadPtr, &Fee_Data[Fee_PendingJob.BlockNumber][Slot][Fee_PendingJob.BlockOffset], Fee_PendingJob.Length); Fee_JobResult = MEMIF_JOB_OK; }
            break;
        case FEE_JOB_WRITE:
            Slot = (uint8)((Fee_ActiveSlot[Fee_PendingJob.BlockNumber] + 1U) % FEE_WL_SLOT_COUNT);
            (void)memset(Fee_Data[Fee_PendingJob.BlockNumber][Slot], 0xFF, FEE_MAX_BLOCK_LENGTH);
            (void)memcpy(Fee_Data[Fee_PendingJob.BlockNumber][Slot], Fee_PendingJob.WritePtr, Fee_PendingJob.Length);
            Fee_ActiveSlot[Fee_PendingJob.BlockNumber] = Slot;
            Fee_BlockLength[Fee_PendingJob.BlockNumber] = Fee_PendingJob.Length;
            Fee_BlockValid[Fee_PendingJob.BlockNumber] = TRUE;
            if (Fee_WriteCount[Fee_PendingJob.BlockNumber] < 0xFFFFFFFFUL) { Fee_WriteCount[Fee_PendingJob.BlockNumber]++; }
            Fee_SaveStorage();
            Fee_JobResult = MEMIF_JOB_OK;
            break;
        case FEE_JOB_INVALIDATE:
            Fee_BlockValid[Fee_PendingJob.BlockNumber] = FALSE;
            Fee_SaveStorage();
            Fee_JobResult = MEMIF_BLOCK_INVALID;
            break;
        case FEE_JOB_ERASE:
            for (Slot = 0U; Slot < FEE_WL_SLOT_COUNT; Slot++) { (void)memset(Fee_Data[Fee_PendingJob.BlockNumber][Slot], 0xFF, FEE_MAX_BLOCK_LENGTH); }
            Fee_BlockLength[Fee_PendingJob.BlockNumber] = 0U;
            Fee_BlockValid[Fee_PendingJob.BlockNumber] = FALSE;
            Fee_ActiveSlot[Fee_PendingJob.BlockNumber] = 0U;
            Fee_SaveStorage();
            Fee_JobResult = MEMIF_JOB_OK;
            break;
        default:
            Fee_JobResult = MEMIF_JOB_FAILED;
            break;
    }
    Fee_PendingJob.JobType = FEE_JOB_NONE;
    Fee_Status = MEMIF_IDLE;
    Fee_Notify();
}

Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)
{
    if ((Fee_Initialized == FALSE) || (DataBufferPtr == NULL) || (BlockNumber >= FEE_MAX_BLOCKS) || (Length == 0U) || (Length > FEE_MAX_BLOCK_LENGTH) || ((uint32)BlockOffset + (uint32)Length > (uint32)FEE_MAX_BLOCK_LENGTH) || (Fee_Status != MEMIF_IDLE)) { return E_NOT_OK; }
    Fee_PendingJob = (Fee_PendingJobType){FEE_JOB_READ, BlockNumber, BlockOffset, Length, DataBufferPtr, NULL};
    Fee_JobResult = MEMIF_JOB_PENDING;
    Fee_Status = MEMIF_BUSY;
    return E_OK;
}

Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length)
{
    if ((Fee_Initialized == FALSE) || (DataBufferPtr == NULL) || (BlockNumber >= FEE_MAX_BLOCKS) || (Length == 0U) || (Length > FEE_MAX_BLOCK_LENGTH) || (Fee_Status != MEMIF_IDLE)) { return E_NOT_OK; }
    Fee_PendingJob = (Fee_PendingJobType){FEE_JOB_WRITE, BlockNumber, 0U, Length, NULL, DataBufferPtr};
    Fee_JobResult = MEMIF_JOB_PENDING;
    Fee_Status = MEMIF_BUSY;
    return E_OK;
}

Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber)
{
    if ((Fee_Initialized == FALSE) || (BlockNumber >= FEE_MAX_BLOCKS) || (Fee_Status != MEMIF_IDLE)) { return E_NOT_OK; }
    Fee_PendingJob = (Fee_PendingJobType){FEE_JOB_INVALIDATE, BlockNumber, 0U, 0U, NULL, NULL};
    Fee_JobResult = MEMIF_JOB_PENDING;
    Fee_Status = MEMIF_BUSY;
    return E_OK;
}

Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber)
{
    if ((Fee_Initialized == FALSE) || (BlockNumber >= FEE_MAX_BLOCKS) || (Fee_Status != MEMIF_IDLE)) { return E_NOT_OK; }
    Fee_PendingJob = (Fee_PendingJobType){FEE_JOB_ERASE, BlockNumber, 0U, 0U, NULL, NULL};
    Fee_JobResult = MEMIF_JOB_PENDING;
    Fee_Status = MEMIF_BUSY;
    return E_OK;
}

void Fee_SetMode(MemIf_ModeType Mode) { Fee_Mode = Mode; }
MemIf_StatusType Fee_GetStatus(void) { return Fee_Status; }
MemIf_JobResultType Fee_GetJobResult(void) { return Fee_JobResult; }
void Fee_SetJobEndNotification(Fee_JobNotificationType NotificationPtr) { Fee_JobEndNotification = NotificationPtr; }
void Fee_SetJobErrorNotification(Fee_JobNotificationType NotificationPtr) { Fee_JobErrorNotification = NotificationPtr; }
