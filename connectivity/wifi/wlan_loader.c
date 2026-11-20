/* SPDX-License-Identifier: Apache-2.0 */
#define LOG_TAG "lge-wlan-loader"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef LGE_MODULE_DIR
#define LGE_MODULE_DIR "/system/lib/modules"
#endif
#ifndef LGE_VENDOR_MODULE_DIR
#define LGE_VENDOR_MODULE_DIR "/vendor/lib/modules"
#endif
#ifndef LGE_PROC_MODULES
#define LGE_PROC_MODULES "/proc/modules"
#endif
#ifndef LGE_INTERFACE
#define LGE_INTERFACE "/sys/class/net/wlan0"
#endif
#ifndef LGE_WAIT_TRIES
#define LGE_WAIT_TRIES 20
#endif

static bool loaded(const char *name) {
    FILE *file = fopen(LGE_PROC_MODULES, "r");
    if (!file) return false;
    char line[512];
    size_t size = strlen(name);
    bool found = false;
    while (fgets(line, sizeof(line), file)) {
        if (!strncmp(line, name, size) && line[size] == ' ') {
            found = true;
            break;
        }
    }
    fclose(file);
    return found;
}

static int load_module(const char *name, const char *path) {
    if (loaded(name)) return 0;
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        ALOGE("open %s: %s", path, strerror(errno));
        return -1;
    }
    struct stat info;
    if (fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_size <= 0 ||
            info.st_size > 32 * 1024 * 1024) {
        ALOGE("Invalid module file %s", path);
        close(fd);
        return -1;
    }
    size_t size = (size_t)info.st_size, offset = 0;
    char *data = malloc(size);
    if (!data) {
        close(fd);
        return -1;
    }
    while (offset < size) {
        ssize_t count = read(fd, data + offset, size - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        offset += (size_t)count;
    }
    close(fd);
    int result = -1;
    if (offset == size && size >= 4 && !memcmp(data, "\177ELF", 4)) {
        result = (int)syscall(__NR_init_module, data, (unsigned long)size, "");
        if (result < 0 && errno == EEXIST && loaded(name)) result = 0;
        if (result < 0) ALOGE("init_module %s: %s", path, strerror(errno));
    } else {
        ALOGE("Incomplete or non-ELF module %s", path);
    }
    free(data);
    return result;
}

int main(void) {
    property_set("wlan.driver.status", "loading");
    /* Select one complete module directory; never mix two builds. */
    const char *dir = LGE_MODULE_DIR;
    char rasdio[512], wlan[512];
    snprintf(rasdio, sizeof(rasdio), "%s/librasdioif.ko", dir);
    snprintf(wlan, sizeof(wlan), "%s/wlan.ko", dir);
    if (access(rasdio, F_OK) < 0 && errno == ENOENT &&
            access(wlan, F_OK) < 0 && errno == ENOENT) {
        dir = LGE_VENDOR_MODULE_DIR;
        snprintf(rasdio, sizeof(rasdio), "%s/librasdioif.ko", dir);
        snprintf(wlan, sizeof(wlan), "%s/wlan.ko", dir);
    }
    ALOGI("Loading Volans modules from %s", dir);
    if (load_module("librasdioif", rasdio) || load_module("wlan", wlan)) {
        property_set("wlan.driver.status", "failed");
        return 1;
    }
    for (unsigned i = 0; i < LGE_WAIT_TRIES; ++i) {
        if (access(LGE_INTERFACE, F_OK) == 0) {
            property_set("wlan.driver.status", "ok");
            ALOGI("Volans modules loaded and wlan0 present");
            return 0;
        }
        usleep(100000);
    }
    ALOGE("Modules loaded but wlan0 is absent; inspect kernel/firmware logs");
    property_set("wlan.driver.status", "failed");
    return 1;
}
