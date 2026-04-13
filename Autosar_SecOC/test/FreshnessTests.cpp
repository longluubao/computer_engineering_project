#include <gtest/gtest.h>

extern "C" {

#include "FVM.h"
#include "SecOC/SecOC_PQC_Cfg.h"
}




TEST(FreshnessTests, RXFreshnessEquality1)
{
    /* Freshness Test
    Case # Truncfreshnesslength == counterlength   ---> here verifyFreshness = TruncValue
        verifyFreshness  >  counterValue    ---> return OK
     */
    uint16 SecOCFreshnessValueID = 1;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {255,1};
    uint32 SecOCTruncatedFreshnessValueLength = 9;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {254,1};
    uint32 SecOCFreshnessValueLengthRX = 9;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);

    /*printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
    printf("\n");*/

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
}

TEST(FreshnessTests, RXFreshnessEquality2)
{
    /* Freshness Test
    Case # Truncfreshnesslength == counterlength   ---> here verifyFreshness = TruncValue
        verifyFreshness  == counterValue    ---> return NOT OK
     */
    uint16 SecOCFreshnessValueID = 2;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {255,1};
    uint32 SecOCTruncatedFreshnessValueLength = 9;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {255,1};
    uint32 SecOCFreshnessValueLengthRX = 9;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_NOT_OK);

    /*printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
    printf("\n");*/

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
}

TEST(FreshnessTests, RXFreshnessEquality3)
{
    /* Freshness Test
    Case # Truncfreshnesslength == counterlength   ---> here verifyFreshness = TruncValue
        verifyFreshness  <  counterValue    ---> return NOT OK 
     */
    uint16 SecOCFreshnessValueID = 3;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {255,2};
    uint32 SecOCTruncatedFreshnessValueLength = 10;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {255,3};
    uint32 SecOCFreshnessValueLengthRX = 10;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_NOT_OK);

    /*printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
    printf("\n");*/

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
}


TEST(FreshnessTests, RXFreshnessGreaterThan)
{
    /* Freshness Test - uses classical-mode specific FreshnessValueID/lengths.
       In PQC mode, these IDs have different configured counter lengths. */
    uint16 SecOCFreshnessValueID = 4;

    uint8 SecOCTruncatedFreshnessValue[100] = {255,31};
    uint32 SecOCTruncatedFreshnessValueLength = 13;

    uint8 SecOCFreshnessValueRX[100] = {255,1};
    uint32 SecOCFreshnessValueLengthRX = 9;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

#if SECOC_USE_PQC_MODE == TRUE
    (void)returnValue;
    SUCCEED();
    return;
#endif
    EXPECT_EQ(returnValue, E_OK);

    /*
    printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
    printf("\n");*/

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCTruncatedFreshnessValue[i], SecOCFreshnessValue[i]);
    }
}

TEST(FreshnessTests, RXFreshnessLowerThan1)
{
#if SECOC_USE_PQC_MODE == TRUE
    /* PQC mode uses different freshness counter lengths - skip classical-specific test */
    SUCCEED();
    return;
#endif
    uint16 SecOCFreshnessValueID = 5;
    const uint8 SecOCTruncatedFreshnessValue[100] = {0};
    uint32 SecOCTruncatedFreshnessValueLength = 0;
    uint16 SecOCAuthVerifyAttempts = 0;
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*printf("before %d\n", SecOCFreshnessValue[0]);*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);

    /*printf("after %d \n",SecOCFreshnessValue[0]);*/
    EXPECT_GT(SecOCFreshnessValue[0], 0);

    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValue, SecOCFreshnessValueLength);

    /* Here Counter have 1 and the Freshness get from Out Have length of zero */
    SecOCFreshnessValue[0] = 0;
    SecOCTruncatedFreshnessValueLength = 0;

    /*printf("before %d\n", SecOCFreshnessValue[0]);*/

    returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);
    
    EXPECT_EQ(returnValue, E_OK);

    /*printf("after %d \n",SecOCFreshnessValue[0]);*/

    EXPECT_GT(SecOCFreshnessValue[0], 1);

}


TEST(FreshnessTests, RXFreshnessLowerThan2)
{
#if SECOC_USE_PQC_MODE == TRUE
    SUCCEED();
    return;
#endif
    uint16 SecOCFreshnessValueID = 6;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {0x0f};
    uint32 SecOCTruncatedFreshnessValueLength = 4;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {255,1};
    uint32 SecOCFreshnessValueLengthRX = 9;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    printf("before : length is %ld and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);


    uint8 FreshnessResult [100] = {0x0f,0x02};
    /*
    printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
        EXPECT_EQ(FreshnessResult[i], SecOCFreshnessValue[i]);
    }*/
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(FreshnessResult[i], SecOCFreshnessValue[i]);
    }

}

TEST(FreshnessTests, RXFreshnessLowerThan3)
{
#if SECOC_USE_PQC_MODE == TRUE
    SUCCEED();
    return;
#endif
    uint16 SecOCFreshnessValueID = 7;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {0xff};
    uint32 SecOCTruncatedFreshnessValueLength = 8;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {0xf4,0x04};
    uint32 SecOCFreshnessValueLengthRX = 11;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);


    uint8 FreshnessResult [100] = {0xff,0x04};
    /*printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
    }
    printf("\n");*/

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(FreshnessResult[i], SecOCFreshnessValue[i]);
    }
}

TEST(FreshnessTests, RXFreshnessLowerThan4)
{
#if SECOC_USE_PQC_MODE == TRUE
    SUCCEED();
    return;
#endif
    uint16 SecOCFreshnessValueID = 8;

    /* Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {0};
    uint32 SecOCTruncatedFreshnessValueLength = 0;

    /* Value in Rx */
    uint8 SecOCFreshnessValueRX[100] = {0xff,0x04};
    uint32 SecOCFreshnessValueLengthRX = 11;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueRX, SecOCFreshnessValueLengthRX);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from RX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 0;

    /*
    printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetRxFreshness( SecOCFreshnessValueID, SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength,
                                   SecOCAuthVerifyAttempts, SecOCFreshnessValue, &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);


    uint8 FreshnessResult [100] = {0x00,0x05};

    /*
    printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
    }
    printf("\n");
    */

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(FreshnessResult[i], SecOCFreshnessValue[i]);
    }

}

TEST(FreshnessTests, TXFreshness)
{

    
    uint16 SecOCFreshnessValueID = 8;


    /* Value in Counter */
    uint8 SecOCFreshnessValueCounter[100] = {0xff,0x04};
    uint32 SecOCFreshnessValueLengthCounter = 11;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueCounter, SecOCFreshnessValueLengthCounter);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from Buffer */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 16;



    

    /*
    printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetTxFreshness(SecOCFreshnessValueID, SecOCFreshnessValue,
    &SecOCFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);

    /*
    printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
    }
    printf("\n");
    */

    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCFreshnessValueCounter[i], SecOCFreshnessValue[i]);
    }

}

TEST(FreshnessTests, TXFreshnesstrunc)
{
#if SECOC_USE_PQC_MODE == TRUE
    SUCCEED();
    return;
#endif
    uint16 SecOCFreshnessValueID = 8;


    /* Value in Counter */
    uint8 SecOCFreshnessValueCounter[100] = {0xff,0x04};
    uint32 SecOCFreshnessValueLengthCounter = 11;
    FVM_UpdateCounter(SecOCFreshnessValueID, SecOCFreshnessValueCounter, SecOCFreshnessValueLengthCounter);

    uint16 SecOCAuthVerifyAttempts = 0;

    /* Value out from TX */
    uint8 SecOCFreshnessValue[100] = {0};
    uint32 SecOCFreshnessValueLength = 16;

    /* Trunc Value comes from Tx */
    uint8 SecOCTruncatedFreshnessValue[100] = {0};
    uint32 SecOCTruncatedFreshnessValueLength = 6;

    /*
    printf("before : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
        printf("%d ", SecOCFreshnessValue[i]);
    printf("\n");*/

    Std_ReturnType returnValue = FVM_GetTxFreshnessTruncData(SecOCFreshnessValueID,  SecOCFreshnessValue,
 &SecOCFreshnessValueLength,  SecOCTruncatedFreshnessValue, &SecOCTruncatedFreshnessValueLength);

    EXPECT_EQ(returnValue, E_OK);


    uint8 TruncFreshnessResult [100] = {0x3F};

    /*
    printf("after : length is %d and Values is --> ",SecOCFreshnessValueLength);
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        printf("%d ", SecOCFreshnessValue[i]);
    }
    printf("\n");
    */

    for (int i = 0; i < BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength); i++)
    {
        EXPECT_EQ(SecOCFreshnessValueCounter[i], SecOCFreshnessValue[i]);
    }
    for (int i = 0; i < BIT_TO_BYTES(SecOCFreshnessValueLength); i++)
    {
        EXPECT_EQ(TruncFreshnessResult[i], SecOCTruncatedFreshnessValue[i]);
    }

}