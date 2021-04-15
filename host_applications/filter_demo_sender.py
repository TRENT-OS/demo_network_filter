#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Read in a data set from a file and send the messages to the destination address.
'''

import argparse
import hashlib
import socket
import sys
import time

from protocol import filter_demo_protocol
from utils import filter_demo_utils

# Default docker bridge network gateway IP
RECEIVER_ADDRESS = "172.17.0.1"
RECEIVER_PORT = 5560


# ------------------------------------------------------------------------------
def send_message_queue(message_queue, receiver_addr):
    """Iterate through the passed message queue and send the messages to the
    specified receiver address.

    Args:
        message_queue (list): List containing all the messages to be sent.
        receiver_addr (tuple): Tuple containing the destination IP address and
        port.
    """
    for message in message_queue:
        if isinstance(message, filter_demo_protocol.GPSDataMsg):
            outgoing_data = message.serialize_message_to_byte_array()
            send_data(outgoing_data, receiver_addr)
            filter_demo_protocol.print_message_content(message)
        else:
            send_data(message, receiver_addr)
            filter_demo_utils.print_unspecified_content(message)

        # Sleep for a few seconds to make it easier to follow the events on
        # the console.
        time.sleep(3)


# ------------------------------------------------------------------------------
def send_data(outgoing_data, receiver_addr):
    """Establish a connection to the specified IP address and send the data to
    it.

    Args:
        outgoing_data (bytes): Bytes to be sent.
        receiver_addr (tuple): Tuple containing the destination IP address and
        port.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect(receiver_addr)
        filter_demo_utils.print_banner()
        print("Established connection to {}:{}.".format(*receiver_addr))

        print("Sending message...")
        sock.sendall(outgoing_data)

    except ConnectionError:
        print("Could not connect to {}:{}.".format(*receiver_addr))
        print("Check if the receiving application is running.")
        raise

    finally:
        sock.close()


# ------------------------------------------------------------------------------
def get_message_from_parsed_data(parsed_data):
    """Create a filter_demo_protocol.GPSDataMsg object, fill it with the
    parsed_data and return it.

    Args:
        parsed_data (dict): A dictionary filled with parsed input data for a
        single message.

    Returns:
        filter_demo_protocol.GPSDataMsg: The created and initialized
        message.
    """
    message = filter_demo_protocol.GPSDataMsg(
        int(parsed_data.get("Type")),
        float(parsed_data.get("Latitude")),
        float(parsed_data.get("Longitude")),
        int(parsed_data.get("Altitude")))

    message.checksum = hashlib.md5(
        message.serialize_payload_to_byte_array()).digest()

    return message


# ------------------------------------------------------------------------------
def queue_input_data_from_file(input_data_file, message_queue):
    """Parse the input data from the provided file and append it to the provided
    message queue.

    Args:
        input_data_file (str): File to read the input data from.
        message_queue (list): List to be filled with the messages parsed from
        the file.
    """
    try:
        with open(input_data_file, "r") as f:
            # Filter out any blank lines.
            lines = filter(None, (line.rstrip() for line in f))

            for line in lines:
                if not line.startswith("#"):  # Skip lines containing comments.
                    try:
                        parsed_message = dict(
                            map(lambda x: x.split(':'), line.split(', ')))
                        message_queue.append(
                            get_message_from_parsed_data(parsed_message))
                    except ValueError:
                        # Queue up the intentionally unspecified "garbage" data
                        # as well.
                        if line.startswith('0x'):
                            # Strip '0x' from hex string.
                            message_queue.append(bytes.fromhex(line[2:]))
                        else:
                            message_queue.append(line.encode())

    except IOError:
        print(input_data_file + ": Could not open file.")
        raise


# ------------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="""Read in a data set from a file and send the messages to
        the destination address.""")
    parser.add_argument('--input', required=True,
                        help='file to read the data set from')
    parser.add_argument('--addr', required=False,
                        default=RECEIVER_ADDRESS,
                        help='destination IP address')
    parser.add_argument('--port', required=False,
                        default=RECEIVER_PORT, type=int,
                        help='destination port')

    args = parser.parse_args()

    try:
        # Parse provided input data into the message queue to be transmitted.
        message_queue = []
        queue_input_data_from_file(args.input, message_queue)

        # Send all the collected messages to the configured receiver.
        receiver_addr = (args.addr, args.port)
        print("Sending all messages to {}:{}...".format(*receiver_addr))

        send_message_queue(message_queue, receiver_addr)
        print("All messages successfully transmitted.")

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
