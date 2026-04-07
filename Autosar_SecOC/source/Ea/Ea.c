#include "Ea.h"
#include "Det.h"
#include <stdio.h>
#include <string.h>

/* External API declarations (MISRA 8.4 visibility). */
void Ea_Init(void);
void Ea_MainFunction(void);
Std_ReturnType Ea_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Ea_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length);
Std_ReturnType Ea_InvalidateBlock(uint16 BlockNumber);
Std_ReturnType Ea_EraseImmediateBlock(uint16 BlockNumber);
void Ea_SetMode(MemIf_ModeType Mode);
MemIf_StatusType Ea_GetStatus(void);
MemIf_JobResultType Ea_GetJobResult(void);
void Ea_SetJobEndNotification(Ea_JobNotificationType NotificationPtr);
void Ea_SetJobErrorNotification(Ea_JobNotificationType NotificationPtr);

#define EA_STORAGE_MAGIC             (0x32414555UL) /* "UEA2" */
#define EA_STORAGE_FILE_NAME         "ea_storage.bin"
#define EA_WL_SLOT_COUNT             ((uint8)3U)

typedef enum
{
    EA_JOB_NONE = 0,
    EA_JOB_READ,
    EA_JOB_WRITE,
    EA_JOB_INVALIDATE,
    EA_JOB_ERASE
} Ea_JobType_Type;

typedef struct
{
    Ea_JobType_Type JobType;
    uint16 BlockNumber;
    uint16 BlockOffset;
    uint16 Length;
    uint8* ReadPtr;
    const uint8* WritePtr;
} Ea_PendingJobType;

typedef struct
{
    uint32 Magic;
    uint32 WriteCount[EA_MAX_BLOCKS];
    uint16 BlockLength[EA_MAX_BLOCKS];
    uint8 BlockValid[EA_MAX_BLOCKS];
    uint8 ActiveSlot[EA_MAX_BLOCKS];
    uint8 Data[EA_MAX_BLOCKS][EA_WL_SLOT_COUNT][EA_MAX_BLOCK_LENGTH];
} Ea_StorageImageType;

static uint8 Ea_Initialized = FALSE;
static MemIf_ModeType Ea_Mode = MEMIF_MODE_SLOW;
static MemIf_StatusType Ea_Status = MEMIF_UNINIT;
static MemIf_JobResultType Ea_JobResult = MEMIF_JOB_OK;
static Ea_PendingJobType Ea_PendingJob = {EA_JOB_NONE, 0U, 0U, 0U, NULL, NULL};
static Ea_JobNotificationType Ea_JobEndNotification = (Ea_JobNotificationType)0;
static Ea_JobNotificationType Ea_JobErrorNotification = (Ea_JobNotificationType)0;

static uint8 Ea_EepromMemory[EA_MAX_BLOCKS][EA_WL_SLOT_COUNT][EA_MAX_BLOCK_LENGTH];
static uint16 Ea_BlockLength[EA_MAX_BLOCKS];
static uint8 Ea_BlockValid[EA_MAX_BLOCKS];
static uint8 Ea_ActiveSlot[EA_MAX_BLOCKS];
static uint32 Ea_WriteCount[EA_MAX_BLOCKS];

static void Ea_LoadStorage(void)
{
    Ea_StorageImageType Image;
    FILE* FilePtr = fopen(EA_STORAGE_FILE_NAME, "rb");

    if (FilePtr == NULL)
    {
        return;
    }

    if ((fread(&Image, sizeof(Ea_StorageImageType), 1U, FilePtr) == 1U) &&
        (Image.Magic == EA_STORAGE_MAGIC))
    {
        (void)memcpy(Ea_EepromMemory, Image.Data, sizeof(Ea_EepromMemory));
        (void)memcpy(Ea_BlockLength, Image.BlockLength, sizeof(Ea_BlockLength));
        (void)memcpy(Ea_BlockValid, Image.BlockValid, sizeof(Ea_BlockValid));
        (void)memcpy(Ea_ActiveSlot, Image.ActiveSlot, sizeof(Ea_ActiveSlot));
        (void)memcpy(Ea_WriteCount, Image.WriteCount, sizeof(Ea_WriteCount));
    }

    (void)fclose(FilePtr);
}

static void Ea_SaveStorage(void)
{
    Ea_StorageImageType Image;
    FILE* FilePtr;

    Image.Magic = EA_STORAGE_MAGIC;
    (void)memcpy(Image.Data, Ea_EepromMemory, sizeof(Ea_EepromMemory));
    (void)memcpy(Image.BlockLength, Ea_BlockLength, sizeof(Ea_BlockLength));
    (void)memcpy(Image.BlockValid, Ea_BlockValid, sizeof(Ea_BlockValid));
    (void)memcpy(Image.ActiveSlot, Ea_ActiveSlot, sizeof(Ea_ActiveSlot));
    (void)memcpy(Image.WriteCount, Ea_WriteCount, sizeof(Ea_WriteCount));

    FilePtr = fopen(EA_STORAGE_FILE_NAME, "wb");
    if (FilePtr == NULL)
    {
        return;
    }

    (void)fwrite(&Image, sizeof(Ea_StorageImageType), 1U, FilePtr);
    (void)fclose(FilePtr);
}

static void Ea_NotifyJobResult(void)
{
    if (Ea_JobResult == MEMIF_JOB_OK)
    {
        if (Ea_JobEndNotification != (Ea_JobNotificationType)0)
        {
            Ea_JobEndNotification();
        }
    }
    else
    {
        if (Ea_JobErrorNotification != (Ea_JobNotificationType)0)
        {
            Ea_JobErrorNotification();
        }
    }
}

static Std_ReturnType Ea_ValidateBlockAndState(uint16 BlockNumber, uint8 Sid)
{
    if (Ea_Initialized == FALSE)
    {
        (void)Det_ReportError(EA_MODULE_ID, EA_INSTANCE_ID, Sid, EA_E_UNINIT);
        return E_NOT_OK;
    }

    if (BlockNumber >= EA_MAX_BLOCKS)
    {
        (void)Det_ReportError(EA_MODULE_ID, EA_INSTANCE_ID, Sid, EA_E_PARAM_BLOCK_NUMBER);
        return E_NOT_OK;
    }

    if (Ea_Status != MEMIF_IDLE)
    {
        (void)Det_ReportError(EA_MODULE_ID, EA_INSTANCE_ID, Sid, EA_E_BUSY);
        return E_NOT_OK;
    }

    return E_OK;
}

void Ea_Init(void)
{
    (void)memset(Ea_EepromMemory, 0xFF, sizeof(Ea_EepromMemory));
    (void)memset(Ea_BlockLength, 0, sizeof(Ea_BlockLength));
    (void)memset(Ea_BlockValid, 0, sizeof(Ea_BlockValid));
    (void)memset(Ea_ActiveSlot, 0, sizeof(Ea_ActiveSlot));
    (void)memset(Ea_WriteCount, 0, sizeof(Ea_WriteCount));

    Ea_LoadStorage();

    Ea_PendingJob.JobType = EA_JOB_NONE;
    Ea_Initialized = TRUE;
    Ea_Mode = MEMIF_MODE_SLOW;
    Ea_Status = MEMIF_IDLE;
    Ea_JobResult = MEMIF_JOB_OK;
}

void Ea_MainFunction(void)
{
    uint8 Slot;

    if ((Ea_Initialized == FALSE) || (Ea_Status != MEMIF_BUSY))
    {
        return;
    }

    switch (Ea_PendingJob.JobType)
    {
        case EA_JOB_READ:
            Slot = Ea_ActiveSlot[Ea_PendingJob.BlockNumber];
            if ((Ea_BlockValid[Ea_PendingJob.BlockNumber] == FALSE) ||
                (Ea_BlockLength[Ea_PendingJob.BlockNumber] < (Ea_PendingJob.BlockOffset + Ea_PendingJob.Length)))
            {
                Ea_JobResult = MEMIF_BLOCK_INVALID;
            }
            else
            {
                (void)memcpy(
                    Ea_PendingJob.ReadPtr,
                    &Ea_EepromMemory[Ea_PendingJob.BlockNumber][Slot][Ea_PendingJob.BlockOffset],
                    Ea_PendingJob.Length
                );
                Ea_JobResult = MEMIF_JOB_OK;
            }
            break;

        case EA_JOB_WRITE:
            if (Ea_PendingJob.Length > EA_MAX_BLOCK_LENGTH)
            {
                Ea_JobResult = MEMIF_JOB_FAILED;
            }
            else
            {
                Slot = (uint8)((Ea_ActiveSlot[Ea_PendingJob.BlockNumber] + 1U) % EA_WL_SLOT_COUNT);
                (void)memset(Ea_EepromMemory[Ea_PendingJob.BlockNumber][Slot], 0xFF, EA_MAX_BLOCK_LENGTH);
                (void)memcpy(
                    Ea_EepromMemory[Ea_PendingJob.BlockNumber][Slot],
                    Ea_PendingJob.WritePtr,
                    Ea_PendingJob.Length
                );
                Ea_ActiveSlot[Ea_PendingJob.BlockNumber] = Slot;
                Ea_BlockLength[Ea_PendingJob.BlockNumber] = Ea_PendingJob.Length;
                Ea_BlockValid[Ea_PendingJob.BlockNumber] = TRUE;
                if (Ea_WriteCount[Ea_PendingJob.BlockNumber] < 0xFFFFFFFFUL)
                {
                    Ea_WriteCount[Ea_PendingJob.BlockNumber]++;
                }
                Ea_SaveStorage();
                Ea_JobResult = MEMIF_JOB_OK;
            }
            break;

        case EA_JOB_INVALIDATE:
            Ea_BlockValid[Ea_PendingJob.BlockNumber] = FALSE;
            Ea_SaveStorage();
            Ea_JobResult = MEMIF_BLOCK_INVALID;
            break;

        case EA_JOB_ERASE:
            for (Slot = 0U; Slot < EA_WL_SLOT_COUNT; Slot++)
            {
                (void)memset(Ea_EepromMemory[Ea_PendingJob.BlockNumber][Slot], 0xFF, EA_MAX_BLOCK_LENGTH);
            }
            Ea_BlockLength[Ea_PendingJob.BlockNumber] = 0U;
            Ea_BlockValid[Ea_PendingJob.BlockNumber] = FALSE;
            Ea_ActiveSlot[Ea_PendingJob.BlockNumber] = 0U;
            Ea_SaveStorage();
            Ea_JobResult = MEMIF_JOB_OK;
            break;

        case EA_JOB_NONE:
        default:
            Ea_JobResult = MEMIF_JOB_FAILED;
            break;
    }

    Ea_PendingJob.JobType = EA_JOB_NONE;
    Ea_Status = MEMIF_IDLE;
    Ea_NotifyJobResult();
}

Std_ReturnType Ea_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)
{
    if (DataBufferPtr == NULL)
    {
        (void)Det_ReportError(EA_MODULE_ID, EA_INSTANCE_ID, EA_SID_READ, EA_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((Length == 0U) || (Length > EA_MAX_BLOCK_LENGTH))
    {
        return E_NOT_OK;
    }

    if (Ea_ValidateBlockAndState(BlockNumber, EA_SID_READ) != E_OK)
    {
        return E_NOT_OK;
    }

    if ((uint32)BlockOffset + (uint32)Length > (uint32)EA_MAX_BLOCK_LENGTH)
    {
        return E_NOT_OK;
    }

    Ea_PendingJob.JobType = EA_JOB_READ;
    Ea_PendingJob.BlockNumber = BlockNumber;
    Ea_PendingJob.BlockOffset = BlockOffset;
    Ea_PendingJob.Length = Length;
    Ea_PendingJob.ReadPtr = DataBufferPtr;
    Ea_JobResult = MEMIF_JOB_PENDING;
    Ea_Status = MEMIF_BUSY;

    return E_OK;
}

Std_ReturnType Ea_Write(uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length)
{
    if (DataBufferPtr == NULL)
    {
        (void)Det_ReportError(EA_MODULE_ID, EA_INSTANCE_ID, EA_SID_WRITE, EA_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((Length == 0U) || (Length > EA_MAX_BLOCK_LENGTH))
    {
        return E_NOT_OK;
    }

    if (Ea_ValidateBlockAndState(BlockNumber, EA_SID_WRITE) != E_OK)
    {
        return E_NOT_OK;
    }

    Ea_PendingJob.JobType = EA_JOB_WRITE;
    Ea_PendingJob.BlockNumber = BlockNumber;
    Ea_PendingJob.BlockOffset = 0U;
    Ea_PendingJob.Length = Length;
    Ea_PendingJob.WritePtr = DataBufferPtr;
    Ea_JobResult = MEMIF_JOB_PENDING;
    Ea_Status = MEMIF_BUSY;

    return E_OK;
}

Std_ReturnType Ea_InvalidateBlock(uint16 BlockNumber)
{
    if (Ea_ValidateBlockAndState(BlockNumber, EA_SID_INVALIDATE_BLOCK) != E_OK)
    {
        return E_NOT_OK;
    }

    Ea_PendingJob.JobType = EA_JOB_INVALIDATE;
    Ea_PendingJob.BlockNumber = BlockNumber;
    Ea_PendingJob.BlockOffset = 0U;
    Ea_PendingJob.Length = 0U;
    Ea_JobResult = MEMIF_JOB_PENDING;
    Ea_Status = MEMIF_BUSY;

    return E_OK;
}

Std_ReturnType Ea_EraseImmediateBlock(uint16 BlockNumber)
{
    if (Ea_ValidateBlockAndState(BlockNumber, EA_SID_ERASE_BLOCK) != E_OK)
    {
        return E_NOT_OK;
    }

    Ea_PendingJob.JobType = EA_JOB_ERASE;
    Ea_PendingJob.BlockNumber = BlockNumber;
    Ea_PendingJob.BlockOffset = 0U;
    Ea_PendingJob.Length = 0U;
    Ea_JobResult = MEMIF_JOB_PENDING;
    Ea_Status = MEMIF_BUSY;

    return E_OK;
}

void Ea_SetMode(MemIf_ModeType Mode)
{
    Ea_Mode = Mode;
}

MemIf_StatusType Ea_GetStatus(void)
{
    return Ea_Status;
}

MemIf_JobResultType Ea_GetJobResult(void)
{
    return Ea_JobResult;
}

void Ea_SetJobEndNotification(Ea_JobNotificationType NotificationPtr)
{
    Ea_JobEndNotification = NotificationPtr;
}

void Ea_SetJobErrorNotification(Ea_JobNotificationType NotificationPtr)
{
    Ea_JobErrorNotification = NotificationPtr;
}
