import socket
import time
import os
import re

from pathlib import Path
from dataloaders import IMG_FORMATS, VID_FORMATS


class SocketClient:
    def __init__(self, server_address="/tmp/uds_socket", socket_family = socket.AF_UNIX, socket_type = socket.SOCK_STREAM) -> None:
        # Make sure the socket already exist
        if not os.path.exists(server_address):
            raise("uds socket server does not exist")
        self.server_address = server_address
        self.socket_family = socket_family
        self.socket_type = socket_type

    def connect_to_server(self):
        # Create a UDS socket
        self.sock = socket.socket(self.socket_family, self.socket_type)

        # Connect the socket to the port where the server is listening
        print('connecting to %s' % self.server_address)
        self.sock.connect(self.server_address)

    # interval:间隔秒数,count：消息发送次数
    def send(self, msg, interval, count):
        msg += "\n"
        num = 0
        while num < count:
            # Send data
            self.sock.sendall(bytes(msg, encoding='UTF-8'))
            print('sending "%s"' % msg)
            num += 1
            time.sleep(interval)

            
    def close(self):

        self.sock.close()
        print('closing socket')


def get_rtsp_ip(source):
    is_file = Path(source).suffix[1:] in (IMG_FORMATS + VID_FORMATS)
    if is_file:
        return "192.168.17.12"
    is_url = source.lower().startswith(('rtsp://', 'rtmp://', 'http://', 'https://'))
    if is_url:
        g = re.match(r"(([01]{0,1}\d{0,1}\d|2[0-4]\d|25[0-5]\d)\.){3}([01]{0,1}\d{0,1}\d|2[0-4]\d|25[0-5]\d)", source)
        return g.group(0)


if __name__ == "__main__":
    import json
    client = SocketClient()
    client.connect_to_server()
    # 有火
    data = {
        "CheckFire": True,
        "DeviceIp": "192.168.17.12"
    }
    msg = json.dumps(data)

    # 间隔3秒发送三次，防止接收端应用挂了，接收不到
    client.send(msg, 3, 3)
    # 无火
    time.sleep(10)
    data = {
        "CheckFire": False,
        "DeviceIp": "192.168.17.12"
    }
    msg = json.dumps(data)
    client.send(msg, 3, 3)
    client.close()