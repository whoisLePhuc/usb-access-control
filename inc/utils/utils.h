#ifndef UTILS_H
#define UTILS_H

int parse_id_string(const char *id_string, char *vid, char *pid);
char *find_usb_sig_partition(const char *vid_str, const char *pid_str);
unsigned char *read_usb_cert(const char *path, size_t *out_len);
int verify_usb_cert(const unsigned char *cert_buf, size_t cert_len, const char *ca_path);

#endif // UTILS_H