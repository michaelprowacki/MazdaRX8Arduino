import argparse
from datetime import datetime
import socket

from packet import DashCodec, DashPacket


def process_arguments():
    parser = argparse.ArgumentParser(description="Dirt Rally 2.0 UDP server test utility")
    parser.add_argument("--ip", metavar="IP address", help="The listening address (default: 127.0.0.1)")
    parser.add_argument("--port", metavar="port", type=int, help="The listening port (default: 5600)")

    return parser.parse_args()


def main():
    ip = "127.0.0.1"
    port = 5600

    args = process_arguments()
    if args.ip:
        ip = args.ip
    
    if args.port:
        port = args.port

    sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    sock.bind((ip, port))

    print("UDP server and listening on {ip}:{port}".format(ip=ip, port=port))

    codec = DashCodec()

    try:
        while(True):
            message, address = sock.recvfrom(codec.buffer_size())
            data = codec.unpack(message)
            print("{timestamp} -- {data}".format(timestamp=datetime.now(), data=data))
    except KeyboardInterrupt:
        print("\nExiting...")


if __name__ == "__main__":
    main()
