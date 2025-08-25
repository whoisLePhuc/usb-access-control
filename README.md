# USB Access Control System

A secure USB device access control system using Finite State Machine (FSM) for managing USB device lifecycle with certificate-based authentication for storage devices.

## Security Features

- **Certificate-based authentication** for USB storage devices
- **Input validation** to prevent buffer overflows and injection attacks
- **Path validation** to prevent directory traversal attacks
- **Configuration management** with environment variable overrides
- **Graceful shutdown** with signal handling
- **Memory leak protection** with proper cleanup

### 🏗️ Architecture

- **Multi-threaded design** with FSM and USB monitoring threads
- **Event-driven architecture** using thread-safe queues
- **Modular design** with separate concerns for USB handling, FSM, and validation
- **Configuration management** with fallback defaults

#### 📁 Project Structure

```
usb-access-control/
├── inc/                          # Header files
│   ├── config.h                  # Configuration management
│   ├── core/                     # Core USB functionality
│   │   ├── core.h               # Core module header
│   │   ├── usb_device.h         # USB device management
│   │   ├── usb_event.h          # Event definitions
│   │   ├── usb_queue.h          # Thread-safe event queue
│   │   └── usb_thread.h         # Thread management
│   ├── fsm/                      # Finite State Machine
│   │   ├── fsm.h                # FSM module header
│   │   └── usb_fsm.h            # FSM implementation
│   ├── security/                 # Security & validation
│   │   ├── security.h           # Security module header
│   │   └── validation.h         # Input validation
│   ├── interface/                # USB interfaces
│   │   ├── interface.h          # Interface module header
│   │   └── usbguard_interface.h # USBGuard DBus interface
│   ├── utils/                    # Utilities
│   │   ├── module.h             # Utils module header
│   │   └── utils.h              # Common utilities
│   ├── logger/                    # logger
│   │   └── logger.h 
│   └── usb_access_control.h     # Main project header
├── src/                          # Source files
│   ├── config.c                  # Configuration implementation
│   ├── core/                     # Core implementation
│   ├── fsm/                      # FSM implementation
│   ├── security/                 # Security implementation
│   ├── interface/                # Interface implementation
│   ├── logger/                   # Logger implementation
    └── utils/                    # Utilities implementation
├── cert/                         # Certificate files
├── build/                        # Build artifacts
├── CMakeLists.txt                # CMake build configuration
├── Makefile                      # Simple build system
└── README.md                     # This file
```

### 🔒 Security Considerations

- All file paths are validated against allowed directories
- Input strings are bounded and validated
- Certificate data is validated before processing
- Device properties are validated before use
- Graceful error handling prevents crashes

## Development

This project uses CMake for building. See `CMakeLists.txt` for build configuration.

## License

[Add your license here]
