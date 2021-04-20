/*
 * Ticker
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "lib_debug/Debug.h"
#include <camkes.h>

//------------------------------------------------------------------------------
int
run(void)
{
    Debug_LOG_INFO("Ticker running");

    // Set up a tick every second.
    int ret = timeServer_rpc_periodic(0, NS_IN_S);
    if (0 != ret)
    {
        Debug_LOG_ERROR("timeServer_rpc_periodic() failed, code %d", ret);
        return -1;
    }

    for (;;)
    {
        timeServer_notify_wait();

        // Send ticks to the connected Network Stack components.
        e_timeout_nwstacktick1_emit();
        e_timeout_nwstacktick2_emit();
    }
}
