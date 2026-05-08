

#data = [("Baseline", 2302935), ("Mutex", 457735), ("Lockfree", 1807718)]
#max_val = max(v for _, v in data)
#width = 100

#for label, val in data:
#    bar_len = int(val / max_val * width)
#    bar = "█" * bar_len + "░" * (width - bar_len)
#    print(f"{label:<10} {bar} {val:>5} tasks/sec")

import socket
import time

def test():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(('127.0.0.1', 8888))
    # 发送多条消息， 每条以\n结尾
    messages = ["hello","world","from","client"]
    #for msg in messages:
       # client.send((msg + "\n").encode())
       # time.sleep(0.1) #可选，模拟时间
    client.send(b"hello\nworld\n")
    client.send(b"hel")
    client.send(b"lo\nworld\n")

    #接收响应
    for i in range(len(messages)):
        data = client.recv(1024).decode()
        print(f"Received: {data.strip()}")
    client.close()

if __name__ == '__main__':
    test()
    