import os
import can
import time
import threading
from datetime import datetime

log_file = "main_log.txt"

def log_info(msg):
    with open(log_file, "a") as f:
        f.write(f"[{datetime.now().strftime('%a %b %d %H:%M:%S %Y')}] INFO: {msg}\n")

def log_error(msg):
    with open(log_file, "a") as f:
        f.write(f"[{datetime.now().strftime('%a %b %d %H:%M:%S %Y')}] ERROR: {msg}\n")

def setup_can_interface():
    os.system("ip link set can0 down")
    os.system("ip link set can0 type can bitrate 50000")
    os.system("ip link set can0 up")

def receive_data(bus):
    while True:
        try:
            msg = bus.recv()
            if msg is not None:
                data = msg.data.decode('utf-8', errors='ignore')
                print(f"\n[Received] From Module -> ID:{msg.arbitration_id} -> {data}")
                log_info(f"Received from Module ID:{msg.arbitration_id} -> {data}")
        except Exception as e:
            log_error(f"Receive Error: {str(e)}")

def main():
    setup_can_interface()
    time.sleep(0.5)  # Short delay after bringing up interface

    try:
        bus = can.interface.Bus(channel='can0', bustype='socketcan')
    except OSError as e:
        print("Socket creation failed:", e)
        log_error("Socket creation failed.")
        return

    threading.Thread(target=receive_data, args=(bus,), daemon=True).start()

    module_ids = [11, 13, 59]
    current = 0
    start_time = time.monotonic()

    while True:
        now = time.monotonic()
        elapsed_ms = (now - start_time) * 1000

        if elapsed_ms >= 100:
            start_time = now
            target = module_ids[current]
            msg_str = f"M{target}"
            msg_data = msg_str.encode('utf-8')

            try:
                msg = can.Message(arbitration_id=target, data=msg_data, is_extended_id=False)
                bus.send(msg)
                print(f"[Sent] To Module {target} -> {msg_str}")
                log_info(f"Sent to Module {target} -> {msg_str}")
            except can.CanError as e:
                print("Write failed:", e)
                log_error("Failed to send CAN frame to module.")

            current = (current + 1) % len(module_ids)

        time.sleep(0.01)  # 10ms sleep

if __name__ == "__main__":
    main()

