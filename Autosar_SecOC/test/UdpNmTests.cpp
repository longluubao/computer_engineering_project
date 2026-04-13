#include <gtest/gtest.h>
#include <string.h>

extern "C" {
#include "UdpNm.h"
#include "Std_Types.h"
#include "ComStack_Types.h"
}

/* ============================================================
 * Helper: build a minimal received NM PDU
 * ============================================================ */
static void BuildRxPdu(uint8 *buf, uint8 nodeId, uint8 cbv)
{
    memset(buf, 0, UDPNM_PDU_LENGTH);
    buf[UDPNM_PDU_NID_POSITION] = nodeId;
    buf[UDPNM_PDU_CBV_POSITION] = cbv;
}

/* Convenience wrapper: query state only */
static UdpNm_NmStateType GetState(void)
{
    UdpNm_NmStateType state;
    UdpNm_ModeType    mode;
    UdpNm_GetState(0U, &state, &mode);
    return state;
}

/* Convenience wrapper: query mode only */
static UdpNm_ModeType GetMode(void)
{
    UdpNm_NmStateType state;
    UdpNm_ModeType    mode;
    UdpNm_GetState(0U, &state, &mode);
    return mode;
}

/* ============================================================
 * Test fixture: re-initialise UdpNm before every test
 * ============================================================ */
class UdpNmTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        UdpNm_Init();
    }
};

/* ============================================================
 * Init / DeInit
 * ============================================================ */

TEST_F(UdpNmTests, InitSetsStateToBusSleepMode)
{
    EXPECT_EQ(GetState(), UDPNM_STATE_BUS_SLEEP);
    EXPECT_EQ(GetMode(),  UDPNM_MODE_BUS_SLEEP);
}

TEST_F(UdpNmTests, DeInitThenApiCallsReturnError)
{
    UdpNm_DeInit();

    UdpNm_NmStateType state;
    UdpNm_ModeType    mode;
    EXPECT_EQ(UdpNm_GetState(0U, &state, &mode), E_NOT_OK);
    EXPECT_EQ(UdpNm_NetworkRequest(0U),           E_NOT_OK);
}

/* ============================================================
 * GetState: parameter validation
 * ============================================================ */

TEST_F(UdpNmTests, GetStateNullStatePtrReturnsError)
{
    UdpNm_ModeType mode;
    EXPECT_EQ(UdpNm_GetState(0U, NULL, &mode), E_NOT_OK);
}

TEST_F(UdpNmTests, GetStateNullModePtrReturnsError)
{
    UdpNm_NmStateType state;
    EXPECT_EQ(UdpNm_GetState(0U, &state, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, GetStateInvalidNetworkReturnsError)
{
    UdpNm_NmStateType state;
    UdpNm_ModeType    mode;
    EXPECT_EQ(UdpNm_GetState(1U, &state, &mode), E_NOT_OK);
}

/* ============================================================
 * Parameter validation – other APIs
 * ============================================================ */

TEST_F(UdpNmTests, NetworkRequestInvalidNetworkReturnsError)
{
    EXPECT_EQ(UdpNm_NetworkRequest(1U), E_NOT_OK);
}

TEST_F(UdpNmTests, NetworkReleaseInvalidNetworkReturnsError)
{
    EXPECT_EQ(UdpNm_NetworkRelease(1U), E_NOT_OK);
}

TEST_F(UdpNmTests, SetUserDataNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_SetUserData(0U, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, GetUserDataNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_GetUserData(0U, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, GetPduDataNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_GetPduData(0U, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, GetNodeIdentifierNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_GetNodeIdentifier(0U, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, GetLocalNodeIdentifierNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_GetLocalNodeIdentifier(0U, NULL), E_NOT_OK);
}

TEST_F(UdpNmTests, TransmitNullPointerReturnsError)
{
    EXPECT_EQ(UdpNm_Transmit(UDPNM_TX_PDU_ID, NULL), E_NOT_OK);
}

/* ============================================================
 * NetworkRequest: BusSleep -> RepeatMessage
 * ============================================================ */

TEST_F(UdpNmTests, NetworkRequestFromBusSleepEntersRepeatMessage)
{
    EXPECT_EQ(UdpNm_NetworkRequest(0U), E_OK);
    EXPECT_EQ(GetState(), UDPNM_STATE_REPEAT_MESSAGE);
    EXPECT_EQ(GetMode(),  UDPNM_MODE_NETWORK);
}

/* ============================================================
 * NetworkRelease: NormalOperation -> ReadySleep
 * ============================================================ */

TEST_F(UdpNmTests, NetworkReleaseFromNormalOperationEntersReadySleep)
{
    /* Get to Normal Operation */
    UdpNm_NetworkRequest(0U);
    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }
    EXPECT_EQ(GetState(), UDPNM_STATE_NORMAL_OPERATION);

    EXPECT_EQ(UdpNm_NetworkRelease(0U), E_OK);
    EXPECT_EQ(GetState(), UDPNM_STATE_READY_SLEEP);
    EXPECT_EQ(GetMode(),  UDPNM_MODE_NETWORK);
}

/* ============================================================
 * ReadySleep: continues TX and eventually goes to PrepareBusSleep
 * ============================================================ */

TEST_F(UdpNmTests, ReadySleepTimeoutEntersPrepareBusSleep)
{
    UdpNm_NetworkRequest(0U);

    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }
    UdpNm_NetworkRelease(0U);   /* -> ReadySleep */

    for (uint16 i = 0U; i < UDPNM_TIMEOUT_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    EXPECT_EQ(GetState(), UDPNM_STATE_PREPARE_BUS_SLEEP);
    EXPECT_EQ(GetMode(),  UDPNM_MODE_PREPARE_BUS_SLEEP);
}

/* ============================================================
 * PrepareBusSleep -> BusSleep
 * ============================================================ */

TEST_F(UdpNmTests, PrepareBusSleepTimeoutEntersBusSleep)
{
    UdpNm_NetworkRequest(0U);
    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }
    UdpNm_NetworkRelease(0U);
    for (uint16 i = 0U; i < UDPNM_TIMEOUT_TIME; i++)
    {
        UdpNm_MainFunction();
    }
    /* Now in PrepareBusSleep */
    for (uint16 i = 0U; i < UDPNM_WAIT_BUS_SLEEP_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    EXPECT_EQ(GetState(), UDPNM_STATE_BUS_SLEEP);
    EXPECT_EQ(GetMode(),  UDPNM_MODE_BUS_SLEEP);
}

/* ============================================================
 * PassiveStartUp
 * ============================================================ */

TEST_F(UdpNmTests, PassiveStartUpFromBusSleepEntersRepeatMessage)
{
    EXPECT_EQ(UdpNm_PassiveStartUp(0U), E_OK);
    EXPECT_EQ(GetState(), UDPNM_STATE_REPEAT_MESSAGE);
}

TEST_F(UdpNmTests, PassiveStartUpInvalidNetworkReturnsError)
{
    EXPECT_EQ(UdpNm_PassiveStartUp(1U), E_NOT_OK);
}

/*
 * SWS: PassiveStartUp does NOT set NetworkRequested.
 * After RepeatMessageTime, without a NetworkRequest, node enters ReadySleep.
 */
TEST_F(UdpNmTests, PassiveStartUpWithoutNetworkRequestGoesToReadySleep)
{
    UdpNm_PassiveStartUp(0U);

    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    EXPECT_EQ(GetState(), UDPNM_STATE_READY_SLEEP);
}

/* ============================================================
 * RxIndication wakes up from BusSleep
 * ============================================================ */

TEST_F(UdpNmTests, RxIndicationFromBusSleepEntersRepeatMessage)
{
    uint8 rxBuf[UDPNM_PDU_LENGTH];
    PduInfoType pduInfo;

    BuildRxPdu(rxBuf, 0x02U, 0x00U);
    pduInfo.SduDataPtr  = rxBuf;
    pduInfo.SduLength   = UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;

    UdpNm_RxIndication(UDPNM_RX_PDU_ID, &pduInfo);

    EXPECT_EQ(GetState(), UDPNM_STATE_REPEAT_MESSAGE);
}

TEST_F(UdpNmTests, RxIndicationNullPointerIsIgnored)
{
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, NULL);
    EXPECT_EQ(GetState(), UDPNM_STATE_BUS_SLEEP);
}

/*
 * SWS: NmTimeoutTimer is reset on reception of any NM PDU.
 * Verify: node in NormalOperation does not timeout if RX arrives before deadline.
 */
TEST_F(UdpNmTests, RxIndicationResetsTimeoutInNormalOperation)
{
    uint8 rxBuf[UDPNM_PDU_LENGTH];
    PduInfoType pduInfo;
    BuildRxPdu(rxBuf, 0x02U, 0x00U);
    pduInfo.SduDataPtr  = rxBuf;
    pduInfo.SduLength   = UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;

    UdpNm_NetworkRequest(0U);
    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }
    EXPECT_EQ(GetState(), UDPNM_STATE_NORMAL_OPERATION);

    /* Run almost to timeout, then reset via RX */
    for (uint16 i = 0U; i < (UDPNM_TIMEOUT_TIME - 1U); i++)
    {
        UdpNm_MainFunction();
    }
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, &pduInfo);

    /* Run another near-timeout period: should still be in NormalOperation */
    for (uint16 i = 0U; i < (UDPNM_TIMEOUT_TIME - 1U); i++)
    {
        UdpNm_MainFunction();
    }

    EXPECT_EQ(GetState(), UDPNM_STATE_NORMAL_OPERATION);
}

/* ============================================================
 * GetNodeIdentifier / GetLocalNodeIdentifier
 * ============================================================ */

TEST_F(UdpNmTests, GetLocalNodeIdentifierReturnsConfiguredId)
{
    uint8 nodeId = 0U;
    EXPECT_EQ(UdpNm_GetLocalNodeIdentifier(0U, &nodeId), E_OK);
    EXPECT_EQ(nodeId, (uint8)UDPNM_NODE_ID);
}

TEST_F(UdpNmTests, GetNodeIdentifierReturnsLastReceivedSourceId)
{
    uint8 rxBuf[UDPNM_PDU_LENGTH];
    PduInfoType pduInfo;

    BuildRxPdu(rxBuf, 0xABU, 0x00U);
    pduInfo.SduDataPtr  = rxBuf;
    pduInfo.SduLength   = UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, &pduInfo);

    uint8 nodeId = 0U;
    EXPECT_EQ(UdpNm_GetNodeIdentifier(0U, &nodeId), E_OK);
    EXPECT_EQ(nodeId, 0xABU);
}

/* ============================================================
 * SetUserData / GetUserData round-trip
 * ============================================================ */

TEST_F(UdpNmTests, SetAndGetUserDataRoundTrip)
{
    uint8 txData[UDPNM_USER_DATA_LENGTH];
    uint8 rxData[UDPNM_USER_DATA_LENGTH];

    for (uint8 i = 0U; i < UDPNM_USER_DATA_LENGTH; i++)
    {
        txData[i] = (uint8)(0x10U + i);
    }

    EXPECT_EQ(UdpNm_SetUserData(0U, txData), E_OK);

    /* GetUserData reads from the RX shadow buffer: simulate an incoming PDU */
    uint8 rxBuf[UDPNM_PDU_LENGTH];
    PduInfoType pduInfo;
    BuildRxPdu(rxBuf, 0x02U, 0x00U);
    for (uint8 i = 0U; i < UDPNM_USER_DATA_LENGTH; i++)
    {
        rxBuf[UDPNM_PDU_USERDATA_OFFSET + i] = txData[i];
    }
    pduInfo.SduDataPtr  = rxBuf;
    pduInfo.SduLength   = UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, &pduInfo);

    EXPECT_EQ(UdpNm_GetUserData(0U, rxData), E_OK);

    for (uint8 i = 0U; i < UDPNM_USER_DATA_LENGTH; i++)
    {
        EXPECT_EQ(rxData[i], txData[i]) << "Mismatch at byte " << (int)i;
    }
}

/* ============================================================
 * GetPduData
 * ============================================================ */

TEST_F(UdpNmTests, GetPduDataReturnsLastReceivedPdu)
{
    uint8 rxBuf[UDPNM_PDU_LENGTH];
    PduInfoType pduInfo;

    BuildRxPdu(rxBuf, 0x05U, UDPNM_CBV_REPEAT_MSG_REQUEST);
    rxBuf[UDPNM_PDU_USERDATA_OFFSET] = 0xDEU;

    pduInfo.SduDataPtr  = rxBuf;
    pduInfo.SduLength   = UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, &pduInfo);

    uint8 pduOut[UDPNM_PDU_LENGTH];
    EXPECT_EQ(UdpNm_GetPduData(0U, pduOut), E_OK);
    EXPECT_EQ(pduOut[UDPNM_PDU_NID_POSITION],    0x05U);
    EXPECT_EQ(pduOut[UDPNM_PDU_CBV_POSITION],    UDPNM_CBV_REPEAT_MSG_REQUEST);
    EXPECT_EQ(pduOut[UDPNM_PDU_USERDATA_OFFSET], 0xDEU);
}

/* ============================================================
 * RepeatMessageRequest: node detection disabled -> E_NOT_OK
 * ============================================================ */

TEST_F(UdpNmTests, RepeatMessageRequestWithNodeDetectionDisabledReturnsError)
{
    EXPECT_EQ(UdpNm_RepeatMessageRequest(0U), E_NOT_OK);
}

/* ============================================================
 * TxConfirmation (smoke – must not crash)
 * ============================================================ */

TEST_F(UdpNmTests, TxConfirmationDoesNotCrash)
{
    UdpNm_NetworkRequest(0U);
    UdpNm_TxConfirmation(UDPNM_TX_PDU_ID, E_OK);
}

/* ============================================================
 * Uninitialised module guard
 * ============================================================ */

TEST(UdpNmUninitTests, ApiCallsOnUninitModuleNoCrash)
{
    /* Do NOT call Init - verify calls don't crash */
    UdpNm_NmStateType state;
    UdpNm_ModeType    mode;
    (void)UdpNm_GetState(0U, &state, &mode);
    (void)UdpNm_NetworkRequest(0U);
    (void)UdpNm_NetworkRelease(0U);
    (void)UdpNm_PassiveStartUp(0U);
    (void)UdpNm_RepeatMessageRequest(0U);

    uint8 nodeId;
    (void)UdpNm_GetLocalNodeIdentifier(0U, &nodeId);
    (void)UdpNm_GetNodeIdentifier(0U, &nodeId);

    uint8 userData[UDPNM_USER_DATA_LENGTH] = {0};
    (void)UdpNm_SetUserData(0U, userData);
    (void)UdpNm_GetUserData(0U, userData);

    uint8 pduData[UDPNM_PDU_LENGTH];
    (void)UdpNm_GetPduData(0U, pduData);

    PduInfoType pduInfo;
    pduInfo.SduDataPtr  = userData;
    pduInfo.SduLength   = UDPNM_USER_DATA_LENGTH;
    pduInfo.MetaDataPtr = NULL;
    (void)UdpNm_Transmit(UDPNM_TX_PDU_ID, &pduInfo);
    SUCCEED();
}
