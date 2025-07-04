
import socket
import threading
import time

def test_client(client_id):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 8080))
        
        message = f"Hello from client  madrid {client_id}\n"
        sock.send(message.encode())
        
        response = sock.recv(1024).decode()
        print(f"Client {client_id} received: {len(response)} bytes")
        
        sock.close()
    except Exception as e:
        print(f"Client {client_id} error: {e}")

# Test with multiple concurrent clients
threads = []
for i in range(8):
    t = threading.Thread(target=test_client, args=(i,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print("All clients finished")