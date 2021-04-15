#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Listen on a configured port for incoming messages and print the received
contents.
'''

import argparse
import hashlib
import socket
import sys

from protocol import filter_demo_protocol
from utils import filter_demo_utils

# Default docker bridge network gateway IP
LISTENER_ADDRESS = "172.17.0.1"
LISTENER_PORT = 6000


# ------------------------------------------------------------------------------
def verify_md5_checksum(message):
    """Calculate the checksum for the received message payload and verify it
    against the received checksum.

    Args:
        message (filter_demo_protocol.GPSDataMsg): Message to verify.
    """
    calculatedChecksum = hashlib.md5(
        message.serialize_payload_to_byte_array()).digest()

    if calculatedChecksum != message.checksum:
        print("Calculated and received checksum do not match!")
        print("Received checksum: ", message.checksum.hex())
        print("Calculated checksum: ", calculatedChecksum.hex())


# ------------------------------------------------------------------------------
def process_incoming_message(received_data):
    """Parse the received message data, verify it and print its contents.

    Args:
        received_data (bytes): Bytes received.
    """
    try:
        message = filter_demo_protocol.GPSDataMsg(
        ).parse_message_from_byte_array(received_data)
    except Exception as e:
        # If the Network Filter running in the TRENTOS system works as intended
        # and filters out all unspecified messages, this should never be
        # reached.
        print("Unable to decode received data!")
        filter_demo_utils.print_unspecified_content(received_data)
    else:
        verify_md5_checksum(message)
        filter_demo_protocol.print_message_content(message)


# ------------------------------------------------------------------------------
def run_listener(listener_addr):
    """Start a TCP server waiting for incoming messages.

    Args:
        listener_addr (tuple): Tuple containing the IP address and port to bind
        the listener to.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.bind(listener_addr)
    except OSError:
        print("Failed to bind on {}:{}, address already in use.".format(
            *listener_addr))
        raise

    sock.listen(1)

    while True:
        filter_demo_utils.print_banner()
        print("Waiting for a connection...")
        connection, client_address = sock.accept()

        try:
            print("Connection from {}:{}.".format((*client_address)))

            while True:
                received_data = connection.recv(1024)
                if not received_data:
                    break
                print("Received new message from client.")
                process_incoming_message(received_data)

        finally:
            connection.close()


# ------------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="""Listen on a configured port for incoming messages and
        print the received contents.""")
    parser.add_argument('--addr', required=False,
                        default=LISTENER_ADDRESS,
                        help='IP address to listen on')
    parser.add_argument('--port', required=False,
                        default=LISTENER_PORT, type=int,
                        help='port to listen on')

    args = parser.parse_args()

    listener_addr = (args.addr, args.port)

    print("Starting Receiver-App on {}:{}...".format(*listener_addr))

    try:
        run_listener(listener_addr)

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1


if __name__ == '__main__':
    sys.exit(main())
