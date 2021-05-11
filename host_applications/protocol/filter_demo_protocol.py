#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Functions and classes related to the binary protocol used to send GPS-Data
messages.
'''

import struct

# The binary protocol this module provides is outlined in the table below.
# All bytes are sorted according to the network byte order (= big-endian).

# | Content | MessageType  | Latitude | Longitude | Altitude | MD5 Checksum |
# |---------|--------------|----------|-----------|----------|--------------|
# |   Type  | Unsigned Int |   Float  |   Float   |    Int   |     Char     |
# |   Byte  |      0-3     |    4-7   |    8-11   |   12-15  |     16-31    |
GPS_PAYLOAD_FMT = '!Iffi'
GPS_MD5_HASH_FMT = '16s'
GPS_FULL_MSG_FMT = GPS_PAYLOAD_FMT + GPS_MD5_HASH_FMT


# ------------------------------------------------------------------------------
class GPSDataMsg:
    def __init__(self, msg_type=None, latitude=None,
                 longitude=None, altitude=None, checksum=None):
        self.msg_type = msg_type
        self.latitude = latitude
        self.longitude = longitude
        self.altitude = altitude
        self.checksum = checksum

    def serialize_payload_to_byte_array(self):
        return struct.pack(
            GPS_PAYLOAD_FMT,
            self.msg_type,
            self.latitude,
            self.longitude,
            self.altitude)

    def serialize_message_to_byte_array(self):
        return struct.pack(
            GPS_FULL_MSG_FMT,
            self.msg_type,
            self.latitude,
            self.longitude,
            self.altitude,
            self.checksum)

    @staticmethod
    def get_size():
        return struct.calcsize(GPS_FULL_MSG_FMT)

    @classmethod
    def parse_message_from_byte_array(cls, byte_array):
        if len(byte_array) != GPSDataMsg.get_size():
            raise ValueError("Size of passed array does not match expected " +
                             "size of {} bytes.".format(
                                 GPSDataMsg.get_size()))
        (msg_type, latitude, longitude,
         altitude, checksum) = struct.unpack(GPS_FULL_MSG_FMT, byte_array)
        return cls(msg_type, latitude, longitude, altitude, checksum)


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
def print_message_content(message):
    print()
    print("Message Content:")
    print()
    print("Type: {}".format(decode_message_type(message.msg_type)))
    print("Latitude: {:.5f}°".format(message.latitude))
    print("Longitude: {:.5f}°".format(message.longitude))
    print("Altitude: {} m".format(message.altitude))
    print("Checksum: {}".format(message.checksum.hex()))
    print()
