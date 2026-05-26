import socket
import struct
import json

def encode_request(req_id, method, params):
    """
    将 RPC 请求编码为二进制格式：
    [4字节请求ID][4字节方法名长度][方法名][4字节参数长度][参数]
    """
    method_bytes = method.encode('utf-8')
    params_bytes = json.dumps(params).encode('utf-8')  # 参数转为 JSON 字符串
    fmt = f'!I I {len(method_bytes)}s I {len(params_bytes)}s'
    return struct.pack(fmt, req_id, len(method_bytes), method_bytes, len(params_bytes), params_bytes)

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 8888))
    req = encode_request(1, "add", {"a": 20, "b": 123})
    sock.send(req)
    data = sock.recv(4096)
    print(f"Received {len(data)} Content:{data}  bytes: {data.hex()}")
    sock.close()

if __name__ == '__main__':
    main()