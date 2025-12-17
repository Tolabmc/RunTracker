# MAX Firmware

Firmware for the MAX32655 microcontroller with FreeRTOS and BLE support.

## Project Structure

```
max_firmware/
│
├── app/                    # Main application
│   ├── main.c              # Entry point
│   ├── app_init.c          # Application initialization
│   └── app_init.h
│
├── workout/                # Workout tracking module
│   ├── workout_types.h     # Core data structures
│   ├── workout_state.c     # State management
│   ├── workout_state.h
│   ├── workout_control.c   # Control functions
│   └── workout_control.h
│
├── input/                  # User input handling
│   ├── buttons.c           # Button processing
│   └── buttons.h
│
├── comms/                  # BLE communications
│   ├── ble_manager.c       # BLE application logic
│   ├── ble_manager.h
│   ├── ble_stack.c         # BLE stack initialization
│   ├── ble_uuid.h          # Custom service UUIDs
│   ├── svc_custom.c        # Custom GATT service
│   ├── svc_custom.h
│   ├── protocol.c          # Communication protocol
│   └── protocol.h
│
├── storage/                # Data storage
│   ├── buffer.c            # Data buffering
│   └── buffer.h
│
├── rtos/                   # FreeRTOS configuration
│   ├── tasks.c             # Task definitions
│   ├── tasks.h
│   └── freertos_tickless.c # Tickless idle support
│
├── utils/                  # Utility functions
│   ├── time_utils.c        # Time utilities
│   └── time_utils.h
│
├── FreeRTOSConfig.h        # FreeRTOS configuration
├── Makefile                # Build system
├── project.mk              # Project-specific settings
└── README.md               # This file
```

## Building

### Prerequisites

- MaximSDK installed
- ARM GCC toolchain
- Make

### Build Commands

```bash
# Set MAXIM_PATH to your SDK location
export MAXIM_PATH=/path/to/MaximSDK

# Build the project
make

# Clean build artifacts
make clean

# Full clean including libraries
make distclean
```

## BLE Communication

The firmware implements a custom BLE service for communication with an ESP32 or other BLE central device.

### Service UUIDs

- **Service UUID**: `12345678-1234-5678-1234-56789ABCDEF0`
- **TX Characteristic**: `12345678-1234-5678-1234-56789ABCDEF1` (Notify)
- **RX Characteristic**: `12345678-1234-5678-1234-56789ABCDEF2` (Write)

### Device Name

The device advertises as "MAX32655".

## Module Overview

### app/
Main application entry point and initialization.

### workout/
Core workout tracking logic including exercise management, set tracking, and workout state.

### input/
Button handling with debouncing and event detection (short press, long press, etc.).

### comms/
BLE communication stack including custom GATT service for bidirectional data transfer.

### storage/
Data buffering and storage management for workout data.

### rtos/
FreeRTOS task definitions and tickless idle support for power management.

### utils/
Common utility functions including time management.

## License

Licensed under the Apache License, Version 2.0.

