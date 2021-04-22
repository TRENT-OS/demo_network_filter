/*
 * Filter Listener
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
static void
init_network_client_api(void)
{
    static OS_Dataport_t dataports[FILTER_LISTENER_NUM_SOCKETS] = {
        OS_DATAPORT_ASSIGN(socket_1_port),
        OS_DATAPORT_ASSIGN(socket_2_port)
    };

    static OS_NetworkStackClient_SocketDataports_t config = {
        .number_of_sockets = ARRAY_SIZE(dataports),
        .dataport = dataports
    };

    OS_NetworkStackClient_init(&config);
}

//------------------------------------------------------------------------------
int
run(void)
{
    // TODO: All of the following code is meant as a simple test and will be
    // replaced with the actual demo application code once we get to that step
    // with SEOS-2659. The idea for now is to have a simple echo server running
    // to test that the system infrastructure is working for the incoming
    // traffic.
    Debug_LOG_INFO("Starting Filter Listener...");

    init_network_client_api();

    OS_NetworkServer_Socket_t tcp_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = FILTER_LISTENER_PORT,
        .backlog   = 1,
    };

    OS_NetworkServer_Handle_t hServer;
    OS_Error_t ret = OS_NetworkServerSocket_create(
                         NULL,
                         &tcp_socket,
                         &hServer);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkServerSocket_create() failed, code %d", ret);
        return -1;
    }

    for (;;)
    {
        Debug_LOG_DEBUG("Accepting new connection");
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

        static char buffer[4096];

        // Loop until an error occurs.
        for (;;)
        {
            Debug_LOG_INFO("Waiting for a new message");

            size_t actualLenRecv = 0;

            ret = OS_NetworkSocket_read(
                      hSocket,
                      buffer,
                      sizeof(buffer),
                      &actualLenRecv);

            if (OS_SUCCESS != ret)
            {
                Debug_LOG_ERROR("OS_NetworkSocket_read() failed, error %d",
                                ret);
                break;
            }

            size_t sumLenWritten = 0;
            // Write back received bytes.
            while (sumLenWritten < actualLenRecv)
            {
                size_t actualLenWritten = 0;

                ret = OS_NetworkSocket_write(
                          hSocket,
                          &buffer[sumLenWritten],
                          actualLenRecv - sumLenWritten,
                          &actualLenWritten);

                if (ret != OS_SUCCESS)
                {
                    Debug_LOG_ERROR("OS_NetworkSocket_write() failed, error %d",
                                    ret);
                    break;
                }
                sumLenWritten += actualLenWritten;

                Debug_LOG_INFO(
                    "OS_NetworkSocket_write() ok, received: %d, send_current: %d,send_total: %d",
                    actualLenRecv, actualLenWritten, sumLenWritten);
            }
        }

        OS_NetworkSocket_close(hSocket);
    }

    return 0;
}
