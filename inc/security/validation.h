#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>
#include <stdint.h>

// Input validation functions
int validate_string(const char *str, size_t max_len);
int validate_device_properties(const void *properties, int count, size_t max_count);
int validate_device_id(unsigned int id);
int validate_file_path(const char *path);
int validate_certificate_data(const unsigned char *data, size_t len);
// Safe string operations
int safe_strncpy(char *dst, size_t dst_size, const char *src, size_t src_len);
int safe_strncat(char *dst, size_t dst_size, const char *src, size_t src_len);

// Buffer overflow protection
#define SAFE_COPY(dst, src, max_len) \
    safe_strncpy((dst), sizeof(dst), (src), (max_len))

#define SAFE_CAT(dst, src, max_len) \
    safe_strncat((dst), sizeof(dst), (src), (max_len))

#endif // VALIDATION_H
