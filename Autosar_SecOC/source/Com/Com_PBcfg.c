/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Com_PBcfg.h"
#include "SecOC_Lcfg.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define COM_PB_NUM_OF_TX_IPDU              ((PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_PB_NUM_OF_RX_IPDU              ((PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
#define COM_PB_NUM_OF_SIGNALS              ((uint16)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_PB_NUM_OF_SIGNAL_GROUPS        ((uint16)2U)
#define COM_PB_SIGNAL_MAX_LENGTH           ((uint16)SECOC_AUTHPDU_MAX_LENGTH)
#define COM_PB_DEFAULT_TX_TIMEOUT_TICKS    ((uint16)20U)
#define COM_PB_DEFAULT_RX_TIMEOUT_TICKS    ((uint16)20U)
#define COM_PB_SIGNAL_FILTER_MASK          ((uint8)0xFFU)
#define COM_PB_DEFAULT_INVALID_VALUE       ((uint8)0xFFU)

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

const Com_SignalConfigType Com_PbSignalConfig[COM_PB_NUM_OF_SIGNALS] =
{
    {0U, 0U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 0U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK},
    {1U, 1U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 1U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK},
    {2U, 2U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 0U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK},
    {3U, 3U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 1U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK},
    {4U, 4U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 0U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK},
    {5U, 5U, 0U, 1U, COM_PB_SIGNAL_MAX_LENGTH, 1U, COM_PB_DEFAULT_INVALID_VALUE, COM_PB_SIGNAL_FILTER_MASK}
};

const Com_SignalGroupConfigType Com_PbSignalGroupConfig[COM_PB_NUM_OF_SIGNAL_GROUPS] =
{
    {0U, (uint16)(COM_PB_NUM_OF_SIGNALS / 2U), 0U, COM_PB_SIGNAL_MAX_LENGTH},
    {(uint16)(COM_PB_NUM_OF_SIGNALS / 2U),
     (uint16)(COM_PB_NUM_OF_SIGNALS - (COM_PB_NUM_OF_SIGNALS / 2U)),
     1U,
     COM_PB_SIGNAL_MAX_LENGTH}
};

const Com_TxIpduConfigType Com_PbTxIpduConfig[COM_PB_NUM_OF_TX_IPDU] =
{
    {0U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 0U},
    {1U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 1U},
    {0U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 2U},
    {1U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 0U},
    {0U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 1U},
    {1U, COM_PB_DEFAULT_TX_TIMEOUT_TICKS + 2U}
};

const Com_RxIpduConfigType Com_PbRxIpduConfig[COM_PB_NUM_OF_RX_IPDU] =
{
    {0U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 0U},
    {1U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 1U},
    {0U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 2U},
    {1U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 0U},
    {0U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 1U},
    {1U, COM_PB_DEFAULT_RX_TIMEOUT_TICKS + 2U}
};
