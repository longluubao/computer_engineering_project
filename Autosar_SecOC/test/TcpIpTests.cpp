/********************************************************************************************************/
/*****************************************TcpIpTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file TcpIpTests.cpp
 * @brief Unit tests for TcpIp Stack Abstraction module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "TcpIp/TcpIp.h"
}

class TcpIpTests : public ::testing::Test {
protected:
    void SetUp() override {
        TcpIp_Init(NULL);
    }
    void TearDown() override {
        TcpIp_Shutdown();
    }
};

/* --- Initialization Tests --- */
TEST_F(TcpIpTests, Init_NullConfig) {
    TcpIp_Shutdown();
    TcpIp_Init(NULL);
    SUCCEED();
}

TEST_F(TcpIpTests, Init_WithConfig) {
    TcpIp_ConfigType cfg;
    cfg.dummy = 0;
    TcpIp_Shutdown();
    TcpIp_Init(&cfg);
    SUCCEED();
}

TEST_F(TcpIpTests, Shutdown_NoCrash) {
    TcpIp_Shutdown();
    SUCCEED();
}

/* --- Socket Management Tests --- */
TEST_F(TcpIpTests, GetSocket_UDP) {
    TcpIp_SocketIdType socketId;
    Std_ReturnType result = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &socketId);
    EXPECT_EQ(result, E_OK);
    EXPECT_NE(socketId, TCPIP_SOCKET_ID_INVALID);
    TcpIp_Close(socketId, TRUE);
}

TEST_F(TcpIpTests, GetSocket_TCP) {
    TcpIp_SocketIdType socketId;
    Std_ReturnType result = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_TCP, &socketId);
    EXPECT_EQ(result, E_OK);
    TcpIp_Close(socketId, TRUE);
}

TEST_F(TcpIpTests, GetSocket_NullPtr) {
    Std_ReturnType result = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(TcpIpTests, Close_ValidSocket) {
    TcpIp_SocketIdType socketId;
    ASSERT_EQ(TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &socketId), E_OK);
    Std_ReturnType result = TcpIp_Close(socketId, FALSE);
    EXPECT_EQ(result, E_OK);
}

TEST_F(TcpIpTests, Close_InvalidSocket) {
    Std_ReturnType result = TcpIp_Close(TCPIP_SOCKET_ID_INVALID, TRUE);
    EXPECT_NE(result, E_OK);
}

/* --- Bind Tests --- */
TEST_F(TcpIpTests, Bind_ValidSocket) {
    TcpIp_SocketIdType socketId;
    ASSERT_EQ(TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &socketId), E_OK);
    uint16 port = 5000U;
    Std_ReturnType result = TcpIp_Bind(socketId, 0U, &port);
    EXPECT_EQ(result, E_OK);
    TcpIp_Close(socketId, TRUE);
}

TEST_F(TcpIpTests, Bind_AutoPort) {
    TcpIp_SocketIdType socketId;
    ASSERT_EQ(TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &socketId), E_OK);
    uint16 port = 0U; /* Auto-assign */
    Std_ReturnType result = TcpIp_Bind(socketId, 0U, &port);
    EXPECT_EQ(result, E_OK);
    TcpIp_Close(socketId, TRUE);
}

/* --- IP Address Management Tests --- */
TEST_F(TcpIpTests, RequestIpAddrAssignment_Static) {
    TcpIp_SockAddrType addr;
    addr.domain = TCPIP_AF_INET;
    addr.addr[0] = 192U;
    addr.addr[1] = 168U;
    addr.addr[2] = 1U;
    addr.addr[3] = 100U;
    addr.port = 0U;
    Std_ReturnType result = TcpIp_RequestIpAddrAssignment(0U, TCPIP_IPADDR_ASSIGNMENT_STATIC, &addr);
    EXPECT_EQ(result, E_OK);
}

TEST_F(TcpIpTests, GetIpAddr_Valid) {
    /* Set an address first */
    TcpIp_SockAddrType addr;
    addr.domain = TCPIP_AF_INET;
    addr.addr[0] = 10U;
    addr.addr[1] = 0U;
    addr.addr[2] = 0U;
    addr.addr[3] = 1U;
    addr.port = 0U;
    TcpIp_RequestIpAddrAssignment(0U, TCPIP_IPADDR_ASSIGNMENT_STATIC, &addr);

    TcpIp_SockAddrType outAddr;
    uint8 netmask = 0U;
    TcpIp_SockAddrType router;
    Std_ReturnType result = TcpIp_GetIpAddr(0U, &outAddr, &netmask, &router);
    EXPECT_EQ(result, E_OK);
}

/* --- GetVersionInfo Tests --- */
TEST_F(TcpIpTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    TcpIp_GetVersionInfo(&versionInfo);
    EXPECT_EQ(versionInfo.moduleID, TCPIP_MODULE_ID);
}

TEST_F(TcpIpTests, GetVersionInfo_NullPtr) {
    TcpIp_GetVersionInfo(NULL);
    SUCCEED();
}

/* --- RxIndication Tests --- */
TEST_F(TcpIpTests, RxIndication_NoCrash) {
    TcpIp_SockAddrType remoteAddr;
    remoteAddr.domain = TCPIP_AF_INET;
    remoteAddr.addr[0] = 10U;
    remoteAddr.addr[1] = 0U;
    remoteAddr.addr[2] = 0U;
    remoteAddr.addr[3] = 2U;
    remoteAddr.port = 4000U;
    uint8 data[16] = {0};
    TcpIp_RxIndication(0U, &remoteAddr, data, 16U);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(TcpIpTests, MainFunction_NoCrash) {
    TcpIp_MainFunction();
    SUCCEED();
}

/* --- Multiple Socket Allocation --- */
TEST_F(TcpIpTests, MultipleSocketAllocation) {
    TcpIp_SocketIdType sockets[TCPIP_MAX_SOCKETS];
    uint8 allocated = 0U;
    for (uint8 i = 0; i < TCPIP_MAX_SOCKETS; i++) {
        if (TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &sockets[i]) == E_OK) {
            allocated++;
        }
    }
    EXPECT_EQ(allocated, TCPIP_MAX_SOCKETS);
    /* Cleanup */
    for (uint8 i = 0; i < allocated; i++) {
        TcpIp_Close(sockets[i], TRUE);
    }
}

TEST_F(TcpIpTests, SocketExhaustion) {
    TcpIp_SocketIdType sockets[TCPIP_MAX_SOCKETS];
    for (uint8 i = 0; i < TCPIP_MAX_SOCKETS; i++) {
        ASSERT_EQ(TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &sockets[i]), E_OK);
    }
    /* Next allocation should fail */
    TcpIp_SocketIdType extraSocket;
    Std_ReturnType result = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &extraSocket);
    EXPECT_NE(result, E_OK);
    /* Cleanup */
    for (uint8 i = 0; i < TCPIP_MAX_SOCKETS; i++) {
        TcpIp_Close(sockets[i], TRUE);
    }
}
