import socket
import argparse
import time
import subprocess
import signal
import sys

RECV_BUFFER_SIZE = 32 * 1024 * 1024


def receive_udp_file(host, port, num_trials):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, RECV_BUFFER_SIZE)
    sock.bind((host, port))

    print(f"[SSLAB] UDP Server listening on {host}:{port}")

    start_ts = {}
    packet_count = {}
    mpstat_proc = None
    trial_count = 0
    upload_started = False

    while True:
        data, addr = sock.recvfrom(1500)
        now = time.time()

        if data == b"/":
            msg = f"[SSLAB] Hello from {host}:{port}\n".encode()
            sock.sendto(msg, addr)

        if data == b"/upload":
            if not upload_started:
                upload_started = True

                mpstat_proc = subprocess.Popen(
                    ["mpstat", "1"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_IGN)
                )

            print(f"[SSLAB] START from {addr}")
            start_ts[addr] = now
            packet_count[addr] = 0
            continue

        if data == b"END":
            end_ts = now
            duration = end_ts - start_ts.get(addr, end_ts)
            total_packets = packet_count.get(addr, 0)

            print(f"[SSLAB] END from {addr} | Packets: {total_packets} | Duration: {duration*1000:.2f} ms")

            sock.sendto(b"ACK", addr)

            start_ts.pop(addr, None)
            packet_count.pop(addr, None)

            trial_count += 1

            if trial_count >= num_trials:
                print(f"[SSLAB] All {num_trials} trials complete. Stopping mpstat and exiting.")
                if mpstat_proc:
                    mpstat_proc.send_signal(signal.SIGINT)
                    stdout, _ = mpstat_proc.communicate()
                    print(f"[SSLAB] mpstat output:\n{stdout.decode()}")
                break

            continue

        if addr in packet_count:
            packet_count[addr] += 1

    sock.close()
    sys.exit(0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', type=str, required=True)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--num_trials', type=int, default=5, required=False)
    args = parser.parse_args()

    receive_udp_file(
        host=args.host,
        port=args.port,
        num_trials=args.num_trials
    )


if __name__ == '__main__':
    main()
