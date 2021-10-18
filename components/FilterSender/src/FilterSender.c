/*
 * Filter Sender
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "OS_Error.h"
#include "OS_Socket.h"

//------------------------------------------------------------------------------
typedef struct
{
    OS_Dataport_t           dataport;
    OS_Socket_Addr_t dstAddr;
}
FilterSender_t;

static FilterSender_t ctx =
{
    .dataport = OS_DATAPORT_ASSIGN(filterSender_port),
    .dstAddr  =
    {
        .addr = FILTER_SENDER_IP_ADDR,
        .port = FILTER_SENDER_PORT
    },
};

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

//------------------------------------------------------------------------------
static OS_Error_t
connectSocket(
    OS_Socket_Handle_t* const socketHandle,
    const OS_Socket_Addr_t* const dstAddr)
{
    OS_Error_t ret = OS_Socket_create(
                         &networkStackCtx,
                         socketHandle,
                         OS_AF_INET,
                         OS_SOCK_STREAM);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed with: %d", ret);
        return ret;
    }

    ret = OS_Socket_connect(*socketHandle, dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_connect() failed, code %d", ret);
        OS_Socket_close(*socketHandle);
        return ret;
    }

    static char evtBuffer[128];
    const size_t evtBufferSize = sizeof(evtBuffer);
    int numberOfSocketsWithEvents;

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

        if (event.socketHandle != socketHandle->handleID)
        {
            Debug_LOG_ERROR("Unexpected handle received: %d, expected: %d",
                            event.socketHandle, socketHandle->handleID);
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

        // Connection successfully established.
        if (event.eventMask & OS_SOCK_EV_CONN_EST)
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

    if (ret != OS_SUCCESS)
    {
        OS_Socket_close(*socketHandle);
    }

    return ret;
}

//------------------------------------------------------------------------------
OS_Error_t
filterSender_rpc_forwardRecvData(
    const size_t requestedLen,
    size_t* const actualLen)
{
    OS_Socket_Handle_t hSocket;

    OS_Error_t ret = connectSocket(&hSocket, &ctx.dstAddr);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("connectSocket() failed with err %d", ret);
        return ret;
    }

    const void* offset = OS_Dataport_getBuf(ctx.dataport);
    const size_t dpSize = OS_Dataport_getSize(ctx.dataport);

    if (requestedLen > dpSize)
    {
        Debug_LOG_ERROR("Requested length %zu exceeds dataport size %zu",
                        requestedLen, dpSize);
        return OS_ERROR_INVALID_PARAMETER;
    }

    size_t sumLenWritten = 0;

    while (sumLenWritten < requestedLen)
    {
        size_t lenWritten = 0;

        ret = OS_Socket_write(
                  hSocket,
                  offset,
                  requestedLen - sumLenWritten,
                  &lenWritten);

        sumLenWritten += lenWritten;

        if (ret == OS_ERROR_TRY_AGAIN)
        {
            continue;
        }
        else if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_write() failed with %d", ret);
            OS_Socket_close(hSocket);
            *actualLen = sumLenWritten;
            return ret;
        }

        offset += sumLenWritten;
    }

    OS_Socket_close(hSocket);

    *actualLen = sumLenWritten;

    return OS_SUCCESS;
}
