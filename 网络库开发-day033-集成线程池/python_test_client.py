import socket

def test_big_data():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(('127.0.0.1', 8888))
    # 生成 1MB 数据（例如 1_000_000 个 'a'）
    line = b'a' * 1_000_000 + b'\n'
    client.send(line)
    # 可以再发一个换行符，让服务器按行处理（如果你按行解析）
    # 如果不按行解析，直接发送即可
    # 接收回显（注意循环接收，因为数据可能分多次到达）
    received = b''
    while len(received) < len(line):
        chunk = client.recv(8192)
        if not chunk:
            break
        received += chunk
    print(f"Sent {len(line)} bytes, received {len(received)} bytes")
    client.close()

if __name__ == '__main__':
    test_big_data()
    