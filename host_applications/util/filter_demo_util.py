#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Helper functions used by modules of the Network Filter Demo.
'''

from . import filter_demo_msg_pb2


# ------------------------------------------------------------------------------
def decode_message_type(message_type):
    """Return the decoded message type.

    Args:
        message_type (int): Encoded type of the message.

    Returns:
        str: Decoded label of the message type.
    """
    message_decoder = {
        22: 'GPS-Status',
        23: 'GPS-Data',
    }

    return message_decoder.get(message_type, "Unknown Message Type")


# ------------------------------------------------------------------------------
def print_banner():
    print()
    print("##" + (78 * "="))


# ------------------------------------------------------------------------------
def print_message_content(message):
    print()
    print("Message Content:")
    print("Type: {}".format(decode_message_type(message.payload.type)))
    print("Latitude: {:.5f}°".format(message.payload.latitude))
    print("Longitude: {:.5f}°".format(message.payload.longitude))
    print("Altitude: {} m".format(message.payload.altitude))
    print("Serialized to String: ",
          message.payload.SerializeToString())
    print("Checksum: {}".format(message.checksum))
    print()


# ------------------------------------------------------------------------------
def print_unspecified_content(unspecified_data):
    print()
    print("Unspecified content:")
    print()
    print(unspecified_data)
    print()
