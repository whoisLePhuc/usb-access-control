CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -O2 -Iinc -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
LDFLAGS = -lpthread -ldbus-1 -ludev -lssl -lcrypto

# Source files
SRCDIR = src
INCDIR = inc
SOURCES = $(shell find $(SRCDIR) -name "*.c") main.c
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = usb_access_control

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libdbus-1-dev libudev-dev libssl-dev

# Run with environment variables
run: $(TARGET)
	USB_ACCESS_CA_CERT_PATH=./cert/ca.crt \
	USB_ACCESS_LOG_LEVEL=2 \
	USB_ACCESS_SIG_RETRY_COUNT=3 \
	USB_ACCESS_SIG_RETRY_DELAY_MS=500 \
	./$(TARGET)

# Run with aggressive retry (for slow mounting devices)
run-aggressive: $(TARGET)
	USB_ACCESS_CA_CERT_PATH=./cert/ca.crt \
	USB_ACCESS_LOG_LEVEL=2 \
	USB_ACCESS_SIG_RETRY_COUNT=10 \
	USB_ACCESS_SIG_RETRY_DELAY_MS=2000 \
	./$(TARGET)

# Run with minimal retry (for fast systems)
run-fast: $(TARGET)
	USB_ACCESS_CA_CERT_PATH=./cert/ca.crt \
	USB_ACCESS_LOG_LEVEL=2 \
	USB_ACCESS_SIG_RETRY_COUNT=2 \
	USB_ACCESS_SIG_RETRY_DELAY_MS=100 \
	./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)

# Test build
test: CFLAGS += -DTESTING
test: $(TARGET)

.PHONY: all clean install-deps run debug test
