# Simple Multi CAN Testing Code

## Overview

This project demonstrates a basic CAN (Controller Area Network) communication setup using Python. It includes a **main controller** and multiple **CAN modules** for bidirectional communication.

* The **Main Controller (ID: 0x07)** sends and receives CAN messages continuously.
* **Modules (IDs: 0x0B, 0x0D, 0x3B)** send data to the main controller and also receive data from it.
* Communication interval for the main controller is **every 10ms**.

## Components

### 1. `main_can.py`

* Acts as the **main CAN controller**.
* Sends a CAN message every 10 milliseconds.
* Continuously listens for messages from modules.

### 2. `module_can.py`

* Simulates CAN modules with IDs: **0x0B (11)**, **0x0D (13)**, and **0x3B (59)**.
* Sends data to the main controller.
* Listens for responses from the main controller.

Each module should be launched separately with its specific CAN ID.

## Features

* Full-duplex communication (send & receive).
* Configurable module ID.
* Built-in logging and timestamping (optional).
* Uses Python's `python-can` library for CAN bus access.

## Requirements

* Python 3.6+

* `python-can` library
  Install with:

  ```bash
  pip install python-can
  ```

* A CAN interface (e.g., SocketCAN on Linux)

## Setup Instructions

1. **Configure your CAN interface**, for example:

   ```bash
   sudo ip link set can0 up type can bitrate 500000
   sudo ifconfig can0 up
   ```

2. **Run the main controller**:

   ```bash
   python main_can.py
   ```

3. **Run each module separately**, specifying their module ID:

   ```bash
   python module_can.py --id 0x0B
   python module_can.py --id 0x0D
   python module_can.py --id 0x3B
   ```

4. **Observe communication logs** showing periodic messages exchanged between main and modules.

## File Structure

```
.
├── main_can.py       # Main CAN controller script
├── module_can.py     # Module script for all 3 CAN devices
├── README.md         # This file
```

## Example Output

```
[MAIN] Sent to 0x0B: b'\x01\x02\x03'
[MAIN] Received from 0x0D: b'\x11\x22\x33'
[MODULE 0x3B] Sent data to main
[MODULE 0x3B] Received: b'\x01\x02\x03'
```

## Notes

* Ensure no other application is using the CAN interface.
* You can extend this code with error logging or message filtering.

---

