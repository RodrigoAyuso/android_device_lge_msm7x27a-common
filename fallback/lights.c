/* SPDX-License-Identifier: Apache-2.0; minimal backlight bring-up. */
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int set_light(struct light_device_t *dev, const struct light_state_t *s) {
    (void)dev;
    unsigned c = s->color;
    unsigned lum = (77 * ((c >> 16) & 255) + 150 * ((c >> 8) & 255) + 29 * (c & 255)) >> 8;
    unsigned max = 255;
    FILE *f = fopen("/sys/class/leds/lcd-backlight/max_brightness", "r");
    if (f) { if (fscanf(f, "%u", &max) != 1) max = 255; fclose(f); }
    f = fopen("/sys/class/leds/lcd-backlight/brightness", "w");
    if (!f) return -errno;
    int r = fprintf(f, "%u\n", lum * max / 255);
    int e = r < 0 ? -EIO : 0;
    if (fclose(f) != 0) e = -EIO;
    return e;
}
static int close_light(struct hw_device_t *dev) { free(dev); return 0; }
static int open_light(const struct hw_module_t *mod, const char *name, struct hw_device_t **out) {
    if (strcmp(name, LIGHT_ID_BACKLIGHT)) return -ENOSYS;
    struct light_device_t *d = calloc(1, sizeof(*d));
    if (!d) return -ENOMEM;
    d->common.tag = HARDWARE_DEVICE_TAG;
    d->common.version = LIGHTS_DEVICE_API_VERSION_2_0;
    d->common.module = (struct hw_module_t *)mod;
    d->common.close = close_light;
    d->set_light = set_light;
    *out = &d->common;
    return 0;
}
static struct hw_module_methods_t methods = { .open = open_light };
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG, .module_api_version = HARDWARE_MODULE_API_VERSION(0, 1),
    .hal_api_version = HARDWARE_HAL_API_VERSION, .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "LG first-boot backlight", .author = "Lineage port helper", .methods = &methods
};
