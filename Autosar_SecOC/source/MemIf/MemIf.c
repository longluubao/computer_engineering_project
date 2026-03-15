#include "MemIf.h"
#include "Fee.h"
#include "Ea.h"
#include "Det.h"

static uint8 MemIf_Initialized = FALSE;

static Std_ReturnType MemIf_CheckDeviceIndex(uint8 DeviceIndex, uint8 Sid)
{
    if (MemIf_Initialized == FALSE)
    {
        (void)Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, Sid, MEMIF_E_UNINIT);
        return E_NOT_OK;
    }

    if ((DeviceIndex != MEMIF_FEE_DEVICE_INDEX) && (DeviceIndex != MEMIF_EA_DEVICE_INDEX))
    {
        (void)Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, Sid, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }

    return E_OK;
}

void MemIf_Init(void)
{
    Fee_Init();
    Ea_Init();
    MemIf_Initialized = TRUE;
}

void MemIf_MainFunction(void)
{
    if (MemIf_Initialized == FALSE)
    {
        return;
    }

    Fee_MainFunction();
    Ea_MainFunction();
}

Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_READ) != E_OK)
    {
        return E_NOT_OK;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length);
    }
    return Ea_Read(BlockNumber, BlockOffset, DataBufferPtr, Length);
}

Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber, const uint8* DataBufferPtr, uint16 Length)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_WRITE) != E_OK)
    {
        return E_NOT_OK;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_Write(BlockNumber, DataBufferPtr, Length);
    }
    return Ea_Write(BlockNumber, DataBufferPtr, Length);
}

Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_INVALIDATE_BLOCK) != E_OK)
    {
        return E_NOT_OK;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_InvalidateBlock(BlockNumber);
    }
    return Ea_InvalidateBlock(BlockNumber);
}

Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_ERASE_IMMEDIATE_BLOCK) != E_OK)
    {
        return E_NOT_OK;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_EraseImmediateBlock(BlockNumber);
    }
    return Ea_EraseImmediateBlock(BlockNumber);
}

void MemIf_SetMode(MemIf_ModeType Mode)
{
    if (MemIf_Initialized == FALSE)
    {
        (void)Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_SET_MODE, MEMIF_E_UNINIT);
        return;
    }

    Fee_SetMode(Mode);
    Ea_SetMode(Mode);
}

MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_GET_STATUS) != E_OK)
    {
        return MEMIF_UNINIT;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_GetStatus();
    }
    return Ea_GetStatus();
}

MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_GET_JOB_RESULT) != E_OK)
    {
        return MEMIF_JOB_FAILED;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        return Fee_GetJobResult();
    }
    return Ea_GetJobResult();
}

Std_ReturnType MemIf_SetJobNotifications(
    uint8 DeviceIndex,
    MemIf_JobNotificationType JobEndNotificationPtr,
    MemIf_JobNotificationType JobErrorNotificationPtr
)
{
    if (MemIf_CheckDeviceIndex(DeviceIndex, MEMIF_SID_INIT) != E_OK)
    {
        return E_NOT_OK;
    }

    if (DeviceIndex == MEMIF_FEE_DEVICE_INDEX)
    {
        Fee_SetJobEndNotification((Fee_JobNotificationType)JobEndNotificationPtr);
        Fee_SetJobErrorNotification((Fee_JobNotificationType)JobErrorNotificationPtr);
    }
    else
    {
        Ea_SetJobEndNotification((Ea_JobNotificationType)JobEndNotificationPtr);
        Ea_SetJobErrorNotification((Ea_JobNotificationType)JobErrorNotificationPtr);
    }

    return E_OK;
}
