#
# Demo Network Filter
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

os_set_config_file(
    "system_config.h"
    CONFIG_PROJECT_NAME "system_config"
)

#-------------------------------------------------------------------------------
project(demo_network_filter C)

# Overwrite the default log level of the lower layers to ERROR as the output
# otherwise gets too cluttered with debug prints.
set(LibUtilsDefaultZfLogLevel 5 CACHE STRING "" FORCE)

include("plat/${PLATFORM}/plat_nic.cmake")

DeclareCAmkESComponent(
    FilterListener
    INCLUDES
        protocol
    SOURCES
        components/FilterListener/src/FilterListener.c
        protocol/MessageProtocol.c
    C_FLAGS
        -Wall -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        os_crypto
        os_socket_client
)

DeclareCAmkESComponent(
    FilterSender
    SOURCES
        components/FilterSender/src/FilterSender.c
    C_FLAGS
        -Wall -Werror
    LIBS
        system_config
        os_core_api
        lib_debug
        os_socket_client
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NwStack1
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR=ETH_1_ADDR
        -DGATEWAY_ADDR=ETH_1_GATEWAY_ADDR
        -DSUBNET_MASK=ETH_1_SUBNET_MASK
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=NETWORK_STACK_1_NUM_SOCKETS
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NwStack2
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR=ETH_2_ADDR
        -DGATEWAY_ADDR=ETH_2_GATEWAY_ADDR
        -DSUBNET_MASK=ETH_2_SUBNET_MASK
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=NETWORK_STACK_2_NUM_SOCKETS
)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

EntropySource_DeclareCAmkESComponent(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoNetworkFilter.camkes)
GenerateSimulateScript()
