/*
 * Filter Listener
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "MessageProtocol.h"
#include "system_config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "OS_Crypto.h"
#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"

//------------------------------------------------------------------------------
static OS_Crypto_Handle_t hCrypto;

static const OS_Crypto_Config_t cryptoCfg =
{
    .mode = OS_Crypto_MODE_LIBRARY,
    .entropy = IF_OS_ENTROPY_ASSIGN(
        entropy_rpc,
        entropy_port),
};

//------------------------------------------------------------------------------
static void
forwardRecvData(
    const void* const receivedData,
    const size_t len)
{
    if ((NULL == receivedData) || (len == 0))
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL or input length is 0", __func__);
        return;
    }

    OS_Dataport_t dataport = OS_DATAPORT_ASSIGN(filterSender_port);

    memcpy(OS_Dataport_getBuf(dataport), receivedData, len);

    size_t actualLen = 0;

    OS_Error_t ret = filterSender_rpc_forwardRecvData(len, &actualLen);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("filterSender_rpc_forwardRecvData() failed with: %d",
                        ret);
    }
    if (actualLen < len)
    {
        Debug_LOG_ERROR("filterSender_rpc_forwardRecvData() failed, length "
                        "requested: %zu, length written: %zu", len, actualLen);
    }
    else
    {
        Debug_LOG_INFO("Message successfully sent");
    }
}

static bool
isRecvDataValid(
    const void* const receivedData,
    const size_t len)
{
    if ((NULL == receivedData) || (len == 0))
    {
        Debug_LOG_ERROR("%s: Parameter check failed! "
                        "Input pointer is NULL or input length is 0", __func__);
        return false;
    }

    if (!MessageProtocol_isRecvDataLenValid(len))
    {
        // At this point we already know that the data received does not match
        // our expected message length and we can drop the received data.
        Debug_LOG_INFO("Received data has an invalid length of %zu bytes", len);
        return false;
    }

    uint8_t digest[CHECKSUM_LENGTH];
    size_t digestSize = sizeof(digest);

    OS_Error_t ret = MessageProtocol_generateMsgChecksum(
                         hCrypto,
                         receivedData,
                         digest,
                         &digestSize);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("MessageProtocol_generateMsgChecksum() failed with: %d",
                        ret);
        // We have encountered an internal error trying to generate the message
        // checksum and thereby the validity of the data cannot be checked.
        // For the scope of this demo, the data will be treated as invalid and
        // discarded.
        return false;
    }

    if (!MessageProtocol_isMsgChecksumValid(receivedData, digest))
    {
        Debug_LOG_INFO("Received checksum does not match calculated checksum");
        return false;
    }

    MessageProtocol_GPSData_t message = {0};

    ret = MessageProtocol_decodeMsgFromRecvData(
              receivedData,
              &message);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("MessageProtocol_decodeMsgFromRecvData() failed with: "
                        "%d", ret);
        return false;
    }

    MessageProtocol_printMsgContent(&message);

    if (!MessageProtocol_isMsgTypeValid(&message))
    {
        Debug_LOG_INFO("Unsupported message type: %u",
                       message.messageType);
        return false;
    }

    if (!MessageProtocol_isMsgPayloadValid(&message))
    {
        Debug_LOG_INFO("Received invalid message payload");
        return false;
    }

    return true;
}

static void
processRecvData(
    void* const receivedData,
    const size_t len)
{
    if (!isRecvDataValid(receivedData, len))
    {
        Debug_LOG_INFO("Dropping received message");
    }
    else
    {
        Debug_LOG_INFO("Forwarding received message");
        forwardRecvData(receivedData, len);
    }
}

static void
init_network_client_api(void)
{
    static OS_Dataport_t dataports[FILTER_LISTENER_NUM_SOCKETS] =
    {
        OS_DATAPORT_ASSIGN(socket_1_port),
        OS_DATAPORT_ASSIGN(socket_2_port)
    };

    static OS_NetworkStackClient_SocketDataports_t config =
    {
        .number_of_sockets = ARRAY_SIZE(dataports),
        .dataport = dataports
    };

    OS_NetworkStackClient_init(&config);
}

//------------------------------------------------------------------------------
int
run(void)
{
    Debug_LOG_INFO("Starting Filter Listener");

    init_network_client_api();

    OS_Error_t ret = OS_Crypto_init(&hCrypto, &cryptoCfg);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Crypto_init() failed with: %d", ret);
        return -1;
    }

    OS_NetworkServer_Socket_t tcp_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = FILTER_LISTENER_PORT,
        .backlog   = 1,
    };

    OS_NetworkServer_Handle_t hServer;
    ret = OS_NetworkServerSocket_create(
              NULL,
              &tcp_socket,
              &hServer);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkServerSocket_create() failed, code %d", ret);
        return -1;
    }

    static uint8_t receivedData[OS_DATAPORT_DEFAULT_SIZE];

    for (;;)
    {
        Debug_LOG_INFO("Accepting new connection");
        OS_NetworkSocket_Handle_t hSocket;
        ret = OS_NetworkServerSocket_accept(
                  hServer,
                  &hSocket);

        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkServerSocket_accept() failed, error %d",
                            ret);
            return -1;
        }

        // Loop until an error occurs.
        do
        {
            Debug_LOG_INFO("Waiting for a new message");

            size_t actualLenRecv = 0;

            ret = OS_NetworkSocket_read(
                      hSocket,
                      receivedData,
                      sizeof(receivedData),
                      &actualLenRecv);
            switch (ret)
            {
            case OS_SUCCESS:
                Debug_LOG_DEBUG(
                    "OS_NetworkSocket_read() received %zu bytes of data",
                    actualLenRecv);
                processRecvData(receivedData, actualLenRecv);
                continue;

            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_INFO(
                    "OS_NetworkSocket_read() reported connection closed");
                break;

            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_DEBUG(
                    "OS_NetworkSocket_read() reported connection closed");
                break;

            default:
                Debug_LOG_ERROR(
                    "OS_NetworkSocket_read() failed, error %d", ret);
                break;
            }
        }
        while (ret == OS_SUCCESS);

        OS_NetworkSocket_close(hSocket);
    }

    OS_Crypto_free(hCrypto);

    return 0;
}
