/*
 * Specification of the message protocol. This specification can also be found
 * in the filter_demo_protocol.py, that defines this protocol for the Python
 * applications running on the host.
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "OS_Crypto.h"

//------------------------------------------------------------------------------
// The binary protocol this module provides is outlined in the table below.
// All bytes are sorted according to the network byte order (= big-endian).

// | Content | MessageType  | Latitude | Longitude | Altitude | MD5 Checksum |
// |---------|--------------|----------|-----------|----------|--------------|
// |   Type  | Unsigned Int |   Float  |   Float   |    Int   |     Char     |
// |   Byte  |      0-3     |    4-7   |    8-11   |   12-15  |     16-31    |
#define CHECKSUM_ALG    OS_CryptoDigest_ALG_MD5
#define CHECKSUM_LENGTH OS_CryptoDigest_SIZE_MD5

#define PAYLOAD_LENGTH sizeof(MessageProtocol_GPSData_t)

#define MESSAGE_LENGTH (PAYLOAD_LENGTH + CHECKSUM_LENGTH)

typedef struct __attribute__ ((__packed__))
{
    uint32_t messageType;
    float latitude;
    float longitude;
    int32_t altitude;
} MessageProtocol_GPSData_t;

//------------------------------------------------------------------------------
void
MessageProtocol_printMsgContent(
    const MessageProtocol_GPSData_t* const message);

OS_Error_t
MessageProtocol_decodeMsgFromRecvData(
    const void* const receivedData,
    MessageProtocol_GPSData_t* const message);

bool
MessageProtocol_isRecvDataLenValid(
    const size_t len);

bool
MessageProtocol_isMsgTypeValid(
    const MessageProtocol_GPSData_t* const message);

bool
MessageProtocol_isMsgPayloadValid(
    const MessageProtocol_GPSData_t* const message);

OS_Error_t
MessageProtocol_generateMsgChecksum(
    const OS_Crypto_Handle_t hCrypto,
    const void* const receivedData,
    void* const digest,
    size_t* const digestSize);

bool
MessageProtocol_isMsgChecksumValid(
    const void* const receivedData,
    const void* const digest);
