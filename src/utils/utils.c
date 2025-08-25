#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libudev.h>
#include <mntent.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/x509_vfy.h>
#include "utils/utils.h"
#define MAX_KEY_LEN 64
#define MAX_CERT_SIZE (1024*1024) // 1MiB

// Read entire certificate file into memory buffer
unsigned char *read_usb_cert(const char *path, size_t *out_len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }
    unsigned char *buf = malloc(MAX_CERT_SIZE);
    if (!buf) {
        close(fd);
        return NULL;
    }
    ssize_t r = read(fd, buf, MAX_CERT_SIZE);
    close(fd);
    if (r <= 0) {
        perror("read");
        free(buf);
        return NULL;
    }
    size_t len = (size_t)r;
    while (len > 0 && buf[len-1] == 0x00) len--;
    *out_len = len;
    return buf;
}

// Parse the "id" string to extract vendor and product IDs
// id_string: "048d:1234"
// vid: output buffer for vendor ID
// pid: output buffer for product ID
int parse_id_string(const char *id_string, char *vid, char *pid) {
    if (!id_string) return -1;
    const char *colon = strchr(id_string, ':');
    if (!colon) return -2; // Không có dấu ':'

    size_t vid_len = colon - id_string;
    size_t pid_len = strlen(colon + 1);
    if (vid_len >= MAX_KEY_LEN || pid_len >= MAX_KEY_LEN)
        return -3;

    strncpy(vid, id_string, vid_len);
    vid[vid_len] = '\0';
    strcpy(pid, colon + 1);
    return 0;
}

// Find USB partition with label "USB_SIG" and specific VID/PID
// Returns a dynamically allocated string with the device path (e.g., "/dev/sdb1
char *find_usb_sig_partition(const char *vid_str, const char *pid_str) {
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Can't create udev context\n");
        return NULL;
    }
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;
    char *result_path = NULL;
    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);
        struct udev_device *usb = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (usb) {
            const char *vid = udev_device_get_sysattr_value(usb, "idVendor");
            const char *pid = udev_device_get_sysattr_value(usb, "idProduct");
            const char *devnode = udev_device_get_devnode(dev);
            const char *label = udev_device_get_property_value(dev, "ID_PART_ENTRY_NAME");
            if (vid && pid && devnode && label &&
                strcasecmp(vid, vid_str) == 0 &&
                strcasecmp(pid, pid_str) == 0 &&
                strcmp(label, "USB_SIG") == 0) {
                result_path = strdup(devnode);
                udev_device_unref(dev);
                break;
            }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return result_path; 
}

// Verify USB certificate against CA certificate
int verify_usb_cert(const unsigned char *cert_buf, size_t cert_len, const char *ca_path) {
    BIO *cert_bio = BIO_new_mem_buf(cert_buf, (int)cert_len);
    if (!cert_bio) return 0;
    X509 *usb_cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
    BIO_free(cert_bio);
    if (!usb_cert) {
        fprintf(stderr, "Failed to parse USB cert\n");
        return 0;
    }
    // load CA cert
    FILE *caf = fopen(ca_path, "r");
    if (!caf) {
        perror("fopen ca.crt");
        X509_free(usb_cert);
        return 0;
    }
    X509 *ca_cert = PEM_read_X509(caf, NULL, NULL, NULL);
    fclose(caf);
    if (!ca_cert) {
        fprintf(stderr, "Failed to parse CA cert\n");
        X509_free(usb_cert);
        return 0;
    }
    // Create cert store and verify
    X509_STORE *store = X509_STORE_new();
    X509_STORE_add_cert(store, ca_cert);

    X509_STORE_CTX *ctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(ctx, store, usb_cert, NULL);

    int ret = X509_verify_cert(ctx); // 1 = OK, 0 = fail
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    X509_free(usb_cert);
    X509_free(ca_cert);
    return ret;
}