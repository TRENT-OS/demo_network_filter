#!/usr/bin/env python3

# ------------------------------------------------------------------------------
#
# Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#
# ------------------------------------------------------------------------------

'''
Helper functions used by modules of the Network Filter Demo.
'''


# ------------------------------------------------------------------------------
def print_banner():
    print()
    print("##" + (78 * "="))


# ------------------------------------------------------------------------------
def print_unspecified_content(unspecified_data):
    print()
    print("Unspecified content:")
    print()
    print(unspecified_data)
    print()
