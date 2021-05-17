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
        os_network_api
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
        os_network_api
)

DeclareCAmkESComponent(
    NwStack1
    SOURCES
        components/NwStack/src/NwStack.c
    C_FLAGS
        -Wall
        -Werror
        -DNUM_SOCKETS=NETWORK_STACK_1_NUM_SOCKETS
        -DCFG_ETH_ADDR=ETH_1_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_1_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_1_SUBNET_MASK
    LIBS
        system_config
        os_core_api
        lib_debug
        os_network_lib
        TimeServer_client
)

DeclareCAmkESComponent(
    NwStack2
    SOURCES
        components/NwStack/src/NwStack.c
    C_FLAGS
        -Wall
        -Werror
        -DNUM_SOCKETS=NETWORK_STACK_2_NUM_SOCKETS
        -DCFG_ETH_ADDR=ETH_2_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_2_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_2_SUBNET_MASK
    LIBS
        system_config
        os_core_api
        lib_debug
        os_network_lib
        TimeServer_client
)

DeclareCAmkESComponent(
    Ticker
    SOURCES
        components/Ticker/src/Ticker.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        os_core_api
        lib_debug
)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

EntropySource_DeclareCAmkESComponent(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoNetworkFilter.camkes)
GenerateSimulateScript()
