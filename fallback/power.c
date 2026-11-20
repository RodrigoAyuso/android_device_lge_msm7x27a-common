/* SPDX-License-Identifier: Apache-2.0
 * Complete legacy early-suspend wake transitions for the LG 3.4 platform.
 */
#define LOG_TAG "power.msm7x27a"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hardware/power.h>
#include <log/log.h>

static int resume_early_suspend(void)
{
    int fd;
    do {
        fd = open("/sys/power/state", O_WRONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
        return -errno;

    static const char state[] = "on\n";
    ssize_t written;
    do {
        written = write(fd, state, sizeof(state) - 1);
    } while (written < 0 && errno == EINTR);
    const int error = written < 0 ? errno :
        (written == (ssize_t)(sizeof(state) - 1) ? 0 : EIO);
    close(fd);
    return -error;
}

static void power_init(struct power_module *module)
{
    (void)module;
    ALOGI("LG MSM7x27a legacy early-suspend wake bridge loaded");
}

static void set_interactive(struct power_module *module, int on)
{
    (void)module;
    if (!on)
        return; /* Oreo autosuspend owns the transition to mem. */
    const int error = resume_early_suspend();
    if (error)
        ALOGE("interactive=1: late resume failed: %s", strerror(-error));
    else
        ALOGI("interactive=1: requested late resume");
}

static void power_hint(struct power_module *module, power_hint_t hint, void *data)
{
    (void)module;
    (void)hint;
    (void)data;
}

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "LG MSM7x27a early-suspend power HAL",
        .author = "LineageOS LG MSM7x27a",
    },
    .init = power_init,
    .setInteractive = set_interactive,
    .powerHint = power_hint,
};
