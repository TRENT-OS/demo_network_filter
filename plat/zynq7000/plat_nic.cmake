#
# zynq7000 QEMU configuration
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

ChanMux_UART_DeclareCAmkESComponents(
    ChanMux_UART
    components/ChanMux/ChanMux_config.c
    system_config
)

NIC_ChanMux_DeclareCAmkESComponent(
    NwDriver1
    CHANMUX_CHANNEL_NIC_1_CTRL
    CHANMUX_CHANNEL_NIC_1_DATA
)

NIC_ChanMux_DeclareCAmkESComponent(
    NwDriver2
    CHANMUX_CHANNEL_NIC_2_CTRL
    CHANMUX_CHANNEL_NIC_2_DATA
)
