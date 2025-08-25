cmake_minimum_required(VERSION 3.10)

project(UsbAccessControlProject VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    src/core/usb_device.c
    src/core/usb_event.c
    src/core/usb_queue.c
    src/core/usb_thread.c
    src/fsm/usb_fsm.c
    src/interface/usbguard_interface.c
    src/utils/utils.c
    src/config.c
    src/security/validation.c
    src/logger/logger.c
    main.c
)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS_1 REQUIRED dbus-1)
pkg_check_modules(UDEV REQUIRED libudev)
find_package(OpenSSL REQUIRED)   # Dùng cách này thay cho pkg_check_modules

add_executable(usb_access_control ${PROJECT_SOURCES})

target_include_directories(usb_access_control
    PRIVATE
        inc
        ${DBUS_1_INCLUDE_DIRS}
        ${UDEV_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}   # từ find_package(OpenSSL)
)

target_compile_options(usb_access_control
    PRIVATE
        ${DBUS_1_CFLAGS_OTHER}
        -Wall -Wextra -Wpedantic
)

target_link_libraries(usb_access_control
    PRIVATE
        Threads::Threads
        ${DBUS_1_LIBRARIES}
        ${UDEV_LIBRARIES}
        ${OPENSSL_LIBRARIES}
)
