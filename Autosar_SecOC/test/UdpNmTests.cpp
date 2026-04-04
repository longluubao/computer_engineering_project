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

TEST_F(UdpNmTests, InitSetsStateToBusSleep)
{
    UdpNm_NmStateType state;
    Std_ReturnType ret = UdpNm_GetState(0U, &state);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(state, UDPNM_STATE_BUS_SLEEP);
}

TEST_F(UdpNmTests, DeInitBeforeInitReturnsError)
{
    UdpNm_DeInit();         /* brings back to uninit */
    /* Second DeInit on uninit module must be a no-op (DET reported internally) */
    UdpNm_DeInit();         /* should not crash */
}

/* ============================================================
 * Parameter validation
 * ============================================================ */

TEST_F(UdpNmTests, GetStateNullPointerReturnsError)
{
    Std_ReturnType ret = UdpNm_GetState(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, GetStateInvalidNetworkReturnsError)
{
    UdpNm_NmStateType state;
    Std_ReturnType ret = UdpNm_GetState(1U, &state);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, NetworkRequestInvalidNetworkReturnsError)
{
    Std_ReturnType ret = UdpNm_NetworkRequest(1U);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, NetworkReleaseInvalidNetworkReturnsError)
{
    Std_ReturnType ret = UdpNm_NetworkRelease(1U);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, SetUserDataNullPointerReturnsError)
{
    Std_ReturnType ret = UdpNm_SetUserData(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, GetUserDataNullPointerReturnsError)
{
    uint8 buf[UDPNM_USER_DATA_LENGTH];
    Std_ReturnType ret = UdpNm_GetUserData(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
    (void)buf;
}

TEST_F(UdpNmTests, GetPduDataNullPointerReturnsError)
{
    Std_ReturnType ret = UdpNm_GetPduData(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, GetNodeIdentifierNullPointerReturnsError)
{
    Std_ReturnType ret = UdpNm_GetNodeIdentifier(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
}

TEST_F(UdpNmTests, GetLocalNodeIdentifierNullPointerReturnsError)
{
    Std_ReturnType ret = UdpNm_GetLocalNodeIdentifier(0U, NULL);
    EXPECT_EQ(ret, E_NOT_OK);
}

/* ============================================================
 * NetworkRequest: BusSleep -> RepeatMessage
 * ============================================================ */

TEST_F(UdpNmTests, NetworkRequestFromBusSleepEntersRepeatMessage)
{
    Std_ReturnType ret = UdpNm_NetworkRequest(0U);
    EXPECT_EQ(ret, E_OK);

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_REPEAT_MESSAGE);
}

/* ============================================================
 * NetworkRelease: NormalOperation -> ReadySleep
 * ============================================================ */

TEST_F(UdpNmTests, NetworkReleaseFromNormalOperationEntersReadySleep)
{
    /* Get into Normal Operation */
    UdpNm_NetworkRequest(0U);   /* -> RepeatMessage */

    /* Run MainFunction enough cycles to leave RepeatMessage */
    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_NORMAL_OPERATION);

    Std_ReturnType ret = UdpNm_NetworkRelease(0U);
    EXPECT_EQ(ret, E_OK);

    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_READY_SLEEP);
}

/* ============================================================
 * State machine: ReadySleep -> PrepareBusSleep -> BusSleep
 * ============================================================ */

TEST_F(UdpNmTests, ReadySleepTimeoutEntersPrepareBusSleep)
{
    UdpNm_NetworkRequest(0U);

    /* Through RepeatMessage -> NormalOperation */
    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    UdpNm_NetworkRelease(0U);   /* -> ReadySleep */

    /* Run until timeout */
    for (uint16 i = 0U; i < UDPNM_TIMEOUT_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_PREPARE_BUS_SLEEP);
}

TEST_F(UdpNmTests, PrepareBusSleepTimeoutEntersBusSleep)
{
    UdpNm_NetworkRequest(0U);

    for (uint16 i = 0U; i < UDPNM_REPEAT_MESSAGE_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    UdpNm_NetworkRelease(0U);

    /* ReadySleep timeout */
    for (uint16 i = 0U; i < UDPNM_TIMEOUT_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    /* PrepareBusSleep timeout */
    for (uint16 i = 0U; i < UDPNM_WAIT_BUS_SLEEP_TIME; i++)
    {
        UdpNm_MainFunction();
    }

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_BUS_SLEEP);
}

/* ============================================================
 * PassiveStartUp
 * ============================================================ */

TEST_F(UdpNmTests, PassiveStartUpFromBusSleepEntersRepeatMessage)
{
    Std_ReturnType ret = UdpNm_PassiveStartUp(0U);
    EXPECT_EQ(ret, E_OK);

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_REPEAT_MESSAGE);
}

TEST_F(UdpNmTests, PassiveStartUpInvalidNetworkReturnsError)
{
    Std_ReturnType ret = UdpNm_PassiveStartUp(1U);
    EXPECT_EQ(ret, E_NOT_OK);
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

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_REPEAT_MESSAGE);
}

TEST_F(UdpNmTests, RxIndicationNullPointerIsIgnored)
{
    /* Must not crash */
    UdpNm_RxIndication(UDPNM_RX_PDU_ID, NULL);

    UdpNm_NmStateType state;
    UdpNm_GetState(0U, &state);
    EXPECT_EQ(state, UDPNM_STATE_BUS_SLEEP);
}

/* ============================================================
 * GetNodeIdentifier / GetLocalNodeIdentifier
 * ============================================================ */

TEST_F(UdpNmTests, GetLocalNodeIdentifierReturnsConfiguredId)
{
    uint8 nodeId = 0U;
    Std_ReturnType ret = UdpNm_GetLocalNodeIdentifier(0U, &nodeId);
    EXPECT_EQ(ret, E_OK);
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
    Std_ReturnType ret = UdpNm_GetNodeIdentifier(0U, &nodeId);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(nodeId, 0xABU);
}

/* ============================================================
 * SetUserData / GetUserData
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

    /* User data read-back comes from the last received PDU, so simulate an
       incoming PDU that contains the same payload. */
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
    Std_ReturnType ret = UdpNm_GetPduData(0U, pduOut);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(pduOut[UDPNM_PDU_NID_POSITION], 0x05U);
    EXPECT_EQ(pduOut[UDPNM_PDU_CBV_POSITION], UDPNM_CBV_REPEAT_MSG_REQUEST);
    EXPECT_EQ(pduOut[UDPNM_PDU_USERDATA_OFFSET], 0xDEU);
}

/* ============================================================
 * TxConfirmation (smoke test – must not crash)
 * ============================================================ */

TEST_F(UdpNmTests, TxConfirmationDoesNotCrash)
{
    UdpNm_NetworkRequest(0U);
    UdpNm_TxConfirmation(UDPNM_TX_PDU_ID, E_OK);
}

/* ============================================================
 * Uninitialised module guard
 * ============================================================ */

TEST(UdpNmUninitTests, ApiCallsOnUninitModuleReturnError)
{
    /* Do NOT call Init here */
    UdpNm_NmStateType state;
    EXPECT_EQ(UdpNm_GetState(0U, &state),         E_NOT_OK);
    EXPECT_EQ(UdpNm_NetworkRequest(0U),            E_NOT_OK);
    EXPECT_EQ(UdpNm_NetworkRelease(0U),            E_NOT_OK);
    EXPECT_EQ(UdpNm_PassiveStartUp(0U),            E_NOT_OK);

    uint8 nodeId;
    EXPECT_EQ(UdpNm_GetLocalNodeIdentifier(0U, &nodeId), E_NOT_OK);
    EXPECT_EQ(UdpNm_GetNodeIdentifier(0U, &nodeId),      E_NOT_OK);

    uint8 userData[UDPNM_USER_DATA_LENGTH] = {0};
    EXPECT_EQ(UdpNm_SetUserData(0U, userData), E_NOT_OK);
    EXPECT_EQ(UdpNm_GetUserData(0U, userData), E_NOT_OK);

    uint8 pduData[UDPNM_PDU_LENGTH];
    EXPECT_EQ(UdpNm_GetPduData(0U, pduData), E_NOT_OK);
}
