/*
 * Filter Sender
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"

//------------------------------------------------------------------------------
typedef struct
{
    OS_Dataport_t       dataport;
    OS_Network_Socket_t tcpSocket;
}
FilterSender_t;

static FilterSender_t ctx =
{
    .dataport   = OS_DATAPORT_ASSIGN(filterSender_port),
    .tcpSocket  =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = FILTER_SENDER_IP_ADDR,
        .port   = FILTER_SENDER_PORT
    },
};

//------------------------------------------------------------------------------
static void
init_network_client_api(void)
{
    static OS_Dataport_t dataports[FILTER_SENDER_NUM_SOCKETS] =
    {
        OS_DATAPORT_ASSIGN(socket_1_port)
    };

    static OS_NetworkStackClient_SocketDataports_t config =
    {
        .number_of_sockets = ARRAY_SIZE(dataports),
        .dataport = dataports
    };

    OS_NetworkStackClient_init(&config);
}

//------------------------------------------------------------------------------
OS_Error_t
filterSender_rpc_forwardRecvData(
    const size_t requestedLen,
    size_t* const actualLen)
{
    OS_NetworkSocket_Handle_t hClient;
    OS_Error_t ret = OS_NetworkSocket_create(
                         NULL,
                         &ctx.tcpSocket,
                         &hClient);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_create() failed, code %d", ret);
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

        ret = OS_NetworkSocket_write(
                  hClient,
                  offset,
                  requestedLen - sumLenWritten,
                  &lenWritten);

        sumLenWritten += lenWritten;

        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR(
                "OS_NetworkSocket_write() failed, code %d", ret);
            OS_NetworkSocket_close(hClient);
            *actualLen = sumLenWritten;
            return ret;
        }

        offset += sumLenWritten;
    }

    OS_NetworkSocket_close(hClient);

    *actualLen = sumLenWritten;

    return OS_SUCCESS;
}

//------------------------------------------------------------------------------
void
post_init(void)
{
    Debug_LOG_INFO("Initializing Filter Sender");

    init_network_client_api();
}
