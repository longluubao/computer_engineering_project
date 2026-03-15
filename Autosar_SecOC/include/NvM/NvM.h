#ifndef INCLUDE_NVM_H_
#define INCLUDE_NVM_H_

#include "Std_Types.h"

#define NVM_MODULE_ID                     ((uint16)20U)
#define NVM_INSTANCE_ID                   ((uint8)0U)

#define NVM_SID_INIT                      ((uint8)0x00U)
#define NVM_SID_READ_ALL                  ((uint8)0x01U)
#define NVM_SID_WRITE_ALL                 ((uint8)0x02U)
#define NVM_SID_MAIN_FUNCTION             ((uint8)0x03U)
#define NVM_SID_GET_ERROR_STATUS          ((uint8)0x04U)
#define NVM_SID_READ_BLOCK                ((uint8)0x05U)
#define NVM_SID_WRITE_BLOCK               ((uint8)0x06U)
#define NVM_SID_RESTORE_BLOCK_DEFAULTS    ((uint8)0x07U)
#define NVM_SID_INVALIDATE_NV_BLOCK       ((uint8)0x08U)
#define NVM_SID_ERASE_NV_BLOCK            ((uint8)0x09U)
#define NVM_SID_SET_RAM_BLOCK_STATUS      ((uint8)0x0AU)
#define NVM_SID_SET_DATA_INDEX            ((uint8)0x0BU)
#define NVM_SID_GET_DATA_INDEX            ((uint8)0x0CU)

#define NVM_E_UNINIT                      ((uint8)0x01U)
#define NVM_E_PARAM_BLOCK_ID              ((uint8)0x02U)
#define NVM_E_PARAM_POINTER               ((uint8)0x03U)
#define NVM_E_NOT_ALLOWED                 ((uint8)0x04U)
#define NVM_E_BUSY                        ((uint8)0x05U)

#define NVM_BLOCK_ID_DEM_DTC_STORAGE      ((uint16)1U)
#define NVM_BLOCK_ID_GATEWAY_HEALTH       ((uint16)2U)
#define NVM_BLOCK_ID_ECUM_DATASET         ((uint16)3U)
#define NVM_NUM_OF_NVRAM_BLOCKS           ((uint16)3U)

typedef uint16 NvM_BlockIdType;

typedef enum
{
    NVM_REQ_OK = 0,
    NVM_REQ_NOT_OK,
    NVM_REQ_PENDING,
    NVM_REQ_INTEGRITY_FAILED,
    NVM_REQ_BLOCK_SKIPPED,
    NVM_REQ_NV_INVALIDATED
} NvM_RequestResultType;

void NvM_Init(void);
void NvM_MainFunction(void);

Std_ReturnType NvM_ReadAll(void);
Std_ReturnType NvM_WriteAll(void);

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr);
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DstPtr);
Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId);
Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId);
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr);
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged);
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex);
Std_ReturnType NvM_GetDataIndex(NvM_BlockIdType BlockId, uint8* DataIndexPtr);

#endif /* INCLUDE_NVM_H_ */
