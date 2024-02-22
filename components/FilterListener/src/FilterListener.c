/*
 * Filter Listener
 *
 * Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "MessageProtocol.h"
#include "system_config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "OS_Crypto.h"
#include "OS_Error.h"
#include "OS_Socket.h"

//------------------------------------------------------------------------------
static OS_Crypto_Handle_t hCrypto;

static const OS_Crypto_Config_t cryptoCfg =
{
    .mode = OS_Crypto_MODE_LIBRARY,
    .entropy = IF_OS_ENTROPY_ASSIGN(
        entropy_rpc,
        entropy_port),
};

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

//------------------------------------------------------------------------------
static OS_Error_t
waitForNetworkStackInit(
    const if_OS_Socket_t* const ctx)
{
    OS_NetworkStack_State_t networkStackState;

    for (;;)
    {
        networkStackState = OS_Socket_getStatus(ctx);
        if (networkStackState == RUNNING)
        {
            // NetworkStack up and running.
            return OS_SUCCESS;
        }
        else if (networkStackState == FATAL_ERROR)
        {
            // NetworkStack will not come up.
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack component.");
            return OS_ERROR_ABORTED;
        }

        // Yield to wait until the stack is up and running.
        seL4_Yield();
    }
}

static OS_Error_t
waitForIncomingConnection(
    const int srvHandleId)
{
    OS_Error_t ret;

    // Wait for the event letting us know that the connection was successfully
    // established.
    for (;;)
    {
        ret = OS_Socket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_wait() failed, code %d", ret);
            break;
        }

        char evtBuffer[128];
        const size_t evtBufferSize = sizeof(evtBuffer);
        int numberOfSocketsWithEvents;

        ret = OS_Socket_getPendingEvents(
                  &networkStackCtx,
                  evtBuffer,
                  evtBufferSize,
                  &numberOfSocketsWithEvents);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() failed, code %d",
                            ret);
            break;
        }

        if (numberOfSocketsWithEvents == 0)
        {
            Debug_LOG_TRACE("OS_Socket_getPendingEvents() returned "
                            "without any pending events");
            continue;
        }

        // We only opened one socket, so if we get more events, this is not ok.
        if (numberOfSocketsWithEvents != 1)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            ret = OS_ERROR_INVALID_STATE;
            break;
        }

        OS_Socket_Evt_t event;
        memcpy(&event, evtBuffer, sizeof(event));

        if (event.socketHandle != srvHandleId)
        {
            Debug_LOG_ERROR("Unexpected handle received: %d, expected: %d",
                            event.socketHandle, srvHandleId);
            ret = OS_ERROR_INVALID_HANDLE;
            break;
        }

        // Socket has been closed by NetworkStack component.
        if (event.eventMask & OS_SOCK_EV_FIN)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_FIN for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_NETWORK_CONN_REFUSED;
            break;
        }

        // Incoming connection received.
        if (event.eventMask & OS_SOCK_EV_CONN_ACPT)
        {
            Debug_LOG_DEBUG("OS_Socket_getPendingEvents() returned "
                            "connection established for handle: %d",
                            event.socketHandle);
            ret = OS_SUCCESS;
            break;
        }

        // Remote socket requested to be closed only valid for clients.
        if (event.eventMask & OS_SOCK_EV_CLOSE)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_CLOSE for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_CONNECTION_CLOSED;
            break;
        }

        // Error received - print error.
        if (event.eventMask & OS_SOCK_EV_ERROR)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_ERROR for handle: %d, code: %d",
                            event.socketHandle, event.currentError);
            ret = event.currentError;
            break;
        }
    }

    return ret;
}

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

//------------------------------------------------------------------------------
int
run(void)
{
    Debug_LOG_INFO("Starting Filter Listener");

    OS_Error_t ret = OS_Crypto_init(&hCrypto, &cryptoCfg);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Crypto_init() failed with: %d", ret);
        return -1;
    }

    // Check and wait until the NetworkStack component is up and running.
    ret = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != ret)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", ret);
        return -1;
    }

    OS_Socket_Handle_t hServer;
    ret = OS_Socket_create(
              &networkStackCtx,
              &hServer,
              OS_AF_INET,
              OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed, code %d", ret);
        return -1;
    }

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = FILTER_LISTENER_PORT
    };

    ret = OS_Socket_bind(
              hServer,
              &dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_bind() failed, code %d", ret);
        OS_Socket_close(hServer);
        return -1;
    }

    ret = OS_Socket_listen(
              hServer,
              1);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_listen() failed, code %d", ret);
        OS_Socket_close(hServer);
        return -1;
    }

    static uint8_t receivedData[OS_DATAPORT_DEFAULT_SIZE];

    for (;;)
    {
        Debug_LOG_INFO("Accepting new connection");
        OS_Socket_Handle_t hSocket;
        OS_Socket_Addr_t srcAddr = {0};

        do
        {
            ret = waitForIncomingConnection(hServer.handleID);
            if (ret != OS_SUCCESS)
            {
                Debug_LOG_ERROR("waitForIncomingConnection() failed, error %d", ret);
                OS_Socket_close(hSocket);
                return -1;
            }

            ret = OS_Socket_accept(
                      hServer,
                      &hSocket,
                      &srcAddr);
        }
        while (ret == OS_ERROR_TRY_AGAIN);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_accept() failed, error %d", ret);
            OS_Socket_close(hSocket);
            return -1;
        }

        // Loop until an error occurs.
        do
        {
            Debug_LOG_INFO("Waiting for a new message");

            size_t actualLenRecv = 0;

            ret = OS_Socket_read(
                      hSocket,
                      receivedData,
                      sizeof(receivedData),
                      &actualLenRecv);

            switch (ret)
            {
            case OS_SUCCESS:
                Debug_LOG_DEBUG(
                    "OS_Socket_read() received %zu bytes of data",
                    actualLenRecv);
                processRecvData(receivedData, actualLenRecv);
                break;

            case OS_ERROR_TRY_AGAIN:
                Debug_LOG_TRACE(
                    "OS_Socket_read() reported try again");

                // Donate the remaining timeslice to a thread of the same
                // priority and try to read again with the next turn.
                seL4_Yield();
                break;

            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_INFO(
                    "OS_Socket_read() reported connection closed");
                break;

            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_DEBUG(
                    "OS_Socket_read() reported connection closed");
                break;

            default:
                Debug_LOG_ERROR(
                    "OS_Socket_read() failed, error %d", ret);
                break;
            }
        }
        while (ret == OS_SUCCESS || ret == OS_ERROR_TRY_AGAIN);

        OS_Socket_close(hSocket);
    }

    OS_Crypto_free(hCrypto);

    return 0;
}
