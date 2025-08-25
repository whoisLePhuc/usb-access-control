# USB Access Control System

### 🔒 Introduction

USB Access Control is a secure framework designed to regulate and monitor the lifecycle of USB storage devices.
The system leverages a Finite State Machine (FSM) for lifecycle management and applies certificate-based authentication to ensure only trusted devices gain access.

This project aims to mitigate common USB-related security threats such as unauthorized access, data exfiltration, and malware injection.

### 🎯 Key Features

- **Finite State Machine (FSM):** Manages the entire USB device lifecycle (e.g., initialization, authentication, authorization, revocation).
- **Certificate-Based Authentication:** Enforces cryptographic validation of storage devices using X.509 certificates.
- **Secure Multi-Threaded Architecture:** Separate threads for FSM execution and USB monitoring with event-driven queues.
- **Robust Security Mechanisms:** Input validation against buffer overflow & injection attacks, Path canonicalization to prevent directory traversal.
- **Configurable Runtime Parameters:** Easily managed via environment variables or configuration files.

### 🏗️ Architecture

- **Multi-threaded design** with FSM and USB monitoring threads
- **Event-driven architecture** using thread-safe queues
- **Modular design** with separate concerns for USB handling, FSM, and validation
- **Configuration management** with fallback defaults

### 🏗️ Architecture Overview


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

### ⚙️ Build & Run
**Dependencies**
- C compiler (GCC/Clang)
- CMake ≥ 3.10 or Make
- OpenSSL (for certificate handling)
- DBus (for device events)
**Build (CMake)**
```
mkdir build && cd build
cmake ..
make
```
**Build (Makefile)**
```
make
```
**Run**
```
sudo ./usb-access-control
```
### 🔑 Certificate Management


### 🧪 Testing
- Plug in a USB storage device
- Observe lifecycle transitions in logs (FSM state changes)
- Verify certificate authentication results
- Attempt unauthorized devices → should be denied
### 👤 Author
- Phuc Le Tuan
- GitHub: whoisLePhuc

