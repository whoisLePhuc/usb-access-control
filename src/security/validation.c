#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
#include "security/validation.h"
#include "core/usb_device.h"

// Validate a string is non-null, within max length, and properly null-terminated
int validate_string(const char *str, size_t max_len) {
    if (!str) {
        return -1; // NULL pointer
    }
    if (max_len == 0) {
        return -2; // Invalid max length
    }
    // Check for null terminator within max_len
    if (strnlen(str, max_len + 1) > max_len) {
        return -3; // String too long
    }
    return 0; // Valid
}
// Validate an array of usb_property_t structures
int validate_device_properties(const void *properties, int count, size_t max_count) {
    if (!properties) {
        return -1; // NULL pointer
    }
    if (count < 0 || (size_t)count > max_count) {
        return -2; // Invalid count
    }
    if (max_count == 0) {
        return -3; // Invalid max count
    }
    // Cast to usb_property_t for validation
    const usb_property_t *props = (const usb_property_t *)properties;
    for (int i = 0; i < count; i++) {
        // Validate key
        if (validate_string(props[i].key, MAX_KEY_LEN) != 0) {
            printf("Error: Invalid key in property index %d\n", validate_string(props[i].key, MAX_KEY_LEN));
            return -4; // Invalid key at index i
        }
        // Validate value
        if (validate_string(props[i].value, MAX_VALUE_LEN) != 0) {
            return -5; // Invalid value at index i
        }
        // Check for empty strings
        if (strlen(props[i].key) == 0) {
            return -6; // Empty key at index i
        }
    }
    return 0; // Valid
}
// Validate device ID is non-zero and within reasonable bounds
int validate_device_id(unsigned int id) {
    // Device ID should not be 0 (reserved for invalid/unknown)
    if (id == 0) {
        return -1;
    }
    // Check for reasonable upper bound (prevent integer overflow)
    if (id > 0x7FFFFFFF) {
        return -2;
    }
    return 0; // Valid
}
// Parse ID string of format "VVVV:PPPP" into vendor_id and product_id
int validate_file_path(const char *path) {
    if (!path) {
        return -1; // NULL pointer
    }
    // Trim leading and trailing whitespace
    while (isspace((unsigned char)*path)) path++; // skip leading spaces
    size_t len = strlen(path);
    while (len > 0 && isspace((unsigned char)path[len - 1])) len--; // trim trailing spaces
    if (len == 0) {
        return -2; // Empty path after trim
    }
    if (len > 4096) {
        return -3; // Path too long
    }
    // Check for illegal characters (control characters or NULL in middle)
    for (size_t i = 0; i < len; i++) {
        unsigned char c = path[i];
        if (c == '\0' || c == '\n' || c == '\r' || c == '\t') {
            return -4; // Invalid character in path
        }
    }
    // Check for directory traversal
    if (strstr(path, "..") || strstr(path, "//")) {
        return -5; // Potential directory traversal
    }
    if (path[0] == '/') {
    // Cho phép các thư mục an toàn hoặc thiết bị /dev
    if (strncmp(path, "/etc/", 5) != 0 &&
        strncmp(path, "/var/", 5) != 0 &&
        strncmp(path, "/home/", 6) != 0 &&
        strncmp(path, "/dev/", 5) != 0) {
        return -6; // Path not in allowed directories
    }
}
    return 0; // Valid path
}
// Validate certificate data buffer
int validate_certificate_data(const unsigned char *data, size_t len) {
    if (!data) {
        return -1; // NULL pointer
    }
    if (len == 0) {
        return -2; // Empty data
    }
    if (len > 1024 * 1024) { // 1MB max
        return -3; // Data too large
    }
    // Check for null bytes (common in binary data)
    if (memchr(data, '\0', len)) {
        return -4; // Contains null bytes
    }
    return 0; // Valid
}
// Safe string copy ensuring null termination and no overflow
int safe_strncpy(char *dst, size_t dst_size, const char *src, size_t src_len) {
    if (!dst || !src || dst_size == 0) {
        return -1; // Invalid parameters
    }
    // Ensure we don't exceed destination buffer
    size_t copy_len = (src_len < dst_size - 1) ? src_len : dst_size - 1;
    // Copy the string
    memcpy(dst, src, copy_len);
    // Ensure null termination
    dst[copy_len] = '\0';
    return 0;
}

// Safe string concatenation ensuring null termination and no overflow
int safe_strncat(char *dst, size_t dst_size, const char *src, size_t src_len) {
    if (!dst || !src || dst_size == 0) {
        return -1; // Invalid parameters
    }
    // Find current string length
    size_t dst_len = strnlen(dst, dst_size);
    // Calculate remaining space
    size_t remaining = dst_size - dst_len - 1;
    if (remaining == 0) {
        return -2; // No space left
    }
    // Calculate how much we can copy
    size_t copy_len = (src_len < remaining) ? src_len : remaining;
    // Append the string
    memcpy(dst + dst_len, src, copy_len);
    // Ensure null termination
    dst[dst_len + copy_len] = '\0';
    return 0;
}
