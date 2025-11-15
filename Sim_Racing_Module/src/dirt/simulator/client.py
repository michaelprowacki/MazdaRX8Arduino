import argparse
import socket
import time

from packet import DashCodec, DashPacket


def process_arguments():
    parser = argparse.ArgumentParser(description="Dirt Rally 2.0 UDP client test utility")
    parser.add_argument("--ip", metavar="IP address", help="The UDP server's IP address (default: 127.0.0.1)")
    parser.add_argument("--port", metavar="port", type=int, help="The UDP server's port (default: 5600)")

    return parser.parse_args()


def main():
    # defaults if none provided
    ip = "127.0.0.1"
    port = 5600

    args = process_arguments()
    if args.ip:
        ip = args.ip
    
    if args.port:
        port = args.port

    codec = DashCodec()

    # test rx-8 instrument cluster with fixed values
    data = DashPacket(engine_rate=903.4, speed=27.77)

    message = codec.pack(*data.to_tuple())
    sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

    print("Sending data to UDP server {ip}:{port}".format(ip=ip, port=port))
    try:
        while(True):
            sock.sendto(message, (ip, port))
            time.sleep(0.01666) # simulate data transfer at 60fps
    except KeyboardInterrupt:
        print("\nExiting...")


if __name__ == "__main__":
    main()
