import socket
import os
import time
import argparse

FILE_SIZE = 1024 * 1024 * 20
CHUNK_SIZE = 1450


def send_udp_file(server_ip, server_port, num_trials):
    payload = os.urandom(FILE_SIZE)
    server_addr = (server_ip, server_port)

    for i in range(num_trials):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(("0.0.0.0", 10000 + i))
        sock.settimeout(60)

        start = time.time()
        sock.sendto(b"/upload", server_addr)

        for j in range(0, len(payload), CHUNK_SIZE):
            sock.sendto(payload[j:j + CHUNK_SIZE], server_addr)
        #time.sleep(1)
        sock.sendto(b"END", server_addr)

        try:
            data, addr = sock.recvfrom(1024)
            src_ip, src_port = addr
            if data == b"ACK":
                end = time.time()
                duration = end - start
                print(f"Trial {i+1}: Succeed with {duration*1000:.2f} ms")
            else:
                print(f"Trial {i+1}: Unexpected response from {src_ip}:{src_port}: {data}")
        except socket.timeout:
            print(f"Trial {i+1}: Timeout")

        sock.close()
        time.sleep(0.0005)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', type=str, required=True)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--num_trials', type=int, default=5, required=False)
    args = parser.parse_args()

    send_udp_file(
        server_ip=args.host,
        server_port=args.port,
        num_trials=args.num_trials
    )


if __name__ == "__main__":
    main()
