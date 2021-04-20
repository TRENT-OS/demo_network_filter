/*
 * Filter Sender
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"

//------------------------------------------------------------------------------
#define FILTER_SENDER_SOCKET_NO 1

#if FILTER_SENDER_SOCKET_NO > OS_NETWORK_MAXIMUM_SOCKET_NO
#error "Number of defined sockets exceeds maximum available sockets!"
#endif

//------------------------------------------------------------------------------
static void
init_network_client_api(void)
{
    static OS_NetworkStackClient_SocketDataports_t config;

    config.number_of_sockets = FILTER_SENDER_SOCKET_NO;

    static OS_Dataport_t dataport = OS_DATAPORT_ASSIGN(socket_1_port);

    config.dataport = &dataport;

    OS_NetworkStackClient_init(&config);
}

//------------------------------------------------------------------------------
int
run(void)
{
    // TODO: All of the following code is meant as a simple test and will be
    // replaced with the actual demo application code once we get to that step
    // with SEOS-2659. The idea for now is to send a few test messages that can
    // be received by the Receiver Python App running on the host.
    Debug_LOG_INFO("Starting Filter Client...");

    init_network_client_api();

    OS_Network_Socket_t tcpSocket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .name   = FILTER_SENDER_IP_ADDR,
        .port   = FILTER_SENDER_PORT
    };

    OS_NetworkSocket_Handle_t hClient;
    OS_Error_t ret = OS_NetworkSocket_create(
                         NULL,
                         &tcpSocket,
                         &hClient);
    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkServerSocket_create() failed, code %d", ret);
        return -1;
    }

    // Send a few test messages to verify the system setup is functional.
    for (unsigned int i = 0; i < 10; i++)
    {
        char request[26];

        DECL_UNUSED_VAR(int lenNeeded) = snprintf(
                                             request,
                                             sizeof(request),
                                             "Client Hello Message #%u\n",
                                             (i + 1));

        Debug_ASSERT((lenNeeded > 0) && (lenNeeded < sizeof(request)));

        const size_t lenRequest = strlen(request);
        size_t sumLenWritten = 0;

        while (sumLenWritten < lenRequest)
        {
            size_t actualLenWritten = 0;

            ret = OS_NetworkSocket_write(
                      hClient,
                      &request[sumLenWritten],
                      lenRequest - sumLenWritten,
                      &actualLenWritten);

            if (ret != OS_SUCCESS)
            {
                Debug_LOG_ERROR(
                    "OS_NetworkSocket_write() failed, code %d", ret);
                OS_NetworkSocket_close(hClient);
                return -1;
            }

            sumLenWritten += actualLenWritten;
        }

        Debug_LOG_INFO("Test message %u successfully sent", (i + 1));
    }

    Debug_LOG_INFO("All test messages successfully sent");

    OS_NetworkSocket_close(hClient);

    return 0;
}
