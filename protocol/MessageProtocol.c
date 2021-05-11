/*
 * Implementation of the message protocol
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "MessageProtocol.h"

#include "lib_debug/Debug.h"

#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

//------------------------------------------------------------------------------
// Supported MessageType.
#define GPS_DATA 23

// Latitude values must be in the range of [-90;90].
#define MAX_LATITUDE  90
#define MIN_LATITUDE  (-MAX_LATITUDE)

// Longitude values must be in the range of [-180;180].
#define MAX_LONGITUDE 180
#define MIN_LONGITUDE (-MAX_LONGITUDE)

// Altitude values must be in the range of [-1000;10000].
#define MAX_ALTITUDE  10000
#define MIN_ALTITUDE  -1000

//------------------------------------------------------------------------------
// This struct is used to allow for an easier network to host byte order
// conversion.
typedef struct __attribute__ ((__packed__))
{
    uint32_t messageType;
    uint32_t latitude;
    uint32_t longitude;
    uint32_t altitude;
} GPSDataConvHelper_t;

Debug_STATIC_ASSERT(sizeof(MessageProtocol_GPSData_t) == sizeof(
                        GPSDataConvHelper_t));

void
MessageProtocol_printMsgContent(
    const MessageProtocol_GPSData_t* const message)
{
    if (NULL == message)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return;
    }

    Debug_LOG_INFO("Message Content:");
    Debug_LOG_INFO("Type %u", message->messageType);
    Debug_LOG_INFO("Latitude %.5f°", message->latitude);
    Debug_LOG_INFO("Longitude %.5f°", message->longitude);
    Debug_LOG_INFO("Altitude %d m", message->altitude);
}

bool
MessageProtocol_isRecvDataLenValid(
    const size_t len)
{
    if (len != MESSAGE_LENGTH)
    {
        return false;
    }

    return true;
}

OS_Error_t
MessageProtocol_decodeMsgFromRecvData(
    const void* const receivedData,
    MessageProtocol_GPSData_t* const message)
{
    if (NULL == receivedData || NULL == message)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return OS_ERROR_INVALID_PARAMETER;
    }

    GPSDataConvHelper_t gpsDataConvHelper = {0};

    memcpy(&gpsDataConvHelper, receivedData, sizeof(GPSDataConvHelper_t));

    // Convert the received data from network byte order (= big-endian) to the
    // host byte order.
    gpsDataConvHelper.messageType = ntohl(gpsDataConvHelper.messageType);
    gpsDataConvHelper.latitude = ntohl(gpsDataConvHelper.latitude);
    gpsDataConvHelper.longitude = ntohl(gpsDataConvHelper.longitude);
    gpsDataConvHelper.altitude = ntohl(gpsDataConvHelper.altitude);

    memcpy(message, &gpsDataConvHelper, sizeof(MessageProtocol_GPSData_t));

    return OS_SUCCESS;
}

bool
MessageProtocol_isMsgTypeValid(
    const MessageProtocol_GPSData_t* const message)
{
    if (NULL == message)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return false;
    }

    if (message->messageType != GPS_DATA)
    {
        return false;
    }

    return true;
}

bool
MessageProtocol_isMsgPayloadValid(
    const MessageProtocol_GPSData_t* const message)
{
    if (NULL == message)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return false;
    }

    if (message->latitude > MAX_LATITUDE
        || message->latitude < MIN_LATITUDE)
    {
        Debug_LOG_DEBUG("Invalid latitude of: %.5f°",
                        message->latitude);
        return false;
    }

    if (message->longitude > MAX_LONGITUDE
        || message->longitude < MIN_LONGITUDE)
    {
        Debug_LOG_DEBUG("Invalid longitude of: %.5f°",
                        message->longitude);
        return false;
    }

    if (message->altitude > MAX_ALTITUDE
        || message->altitude < MIN_ALTITUDE)
    {
        Debug_LOG_DEBUG("Invalid altitude of: %d m",
                        message->altitude);
        return false;
    }

    return true;
}

OS_Error_t
MessageProtocol_generateMsgChecksum(
    const OS_Crypto_Handle_t hCrypto,
    const void* const receivedData,
    void* const digest,
    size_t* const digestSize)
{
    if (NULL == hCrypto || NULL == receivedData || NULL == digest
        || NULL == digestSize)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return OS_ERROR_INVALID_PARAMETER;
    }

    OS_CryptoDigest_Handle_t hDigest;

    OS_Error_t ret = OS_CryptoDigest_init(
                         &hDigest,
                         hCrypto,
                         CHECKSUM_ALG);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_CryptoDigest_init() failed with: %d", ret);
        return ret;
    }

    ret = OS_CryptoDigest_process(
              hDigest,
              receivedData,
              PAYLOAD_LENGTH);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_CryptoDigest_process() failed with: %d", ret);
        OS_CryptoDigest_free(hDigest);
        return ret;
    }

    ret = OS_CryptoDigest_finalize(hDigest, digest, digestSize);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_CryptoDigest_finalize() failed with: %d", ret);
        OS_CryptoDigest_free(hDigest);
        return ret;
    }

    OS_CryptoDigest_free(hDigest);

    return OS_SUCCESS;
}

bool
MessageProtocol_isMsgChecksumValid(
    const void* const receivedData,
    const void* const digest)
{
    if (NULL == digest || NULL == receivedData)
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL", __func__);
        return false;
    }

    const uint8_t* bytePointer = receivedData;

    int ret = memcmp(
                  &bytePointer[PAYLOAD_LENGTH],
                  digest,
                  CHECKSUM_LENGTH);
    if (ret != 0)
    {
        return false;
    }

    return true;
}
