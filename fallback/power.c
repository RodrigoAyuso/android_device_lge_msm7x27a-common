/* SPDX-License-Identifier: Apache-2.0; no frequency or suspend tuning. */
#include <hardware/hardware.h>
#include <hardware/power.h>
static void init(struct power_module *m) { (void)m; }
static void interactive(struct power_module *m, int on) { (void)m; (void)on; }
static void hint(struct power_module *m, power_hint_t h, void *d) { (void)m; (void)h; (void)d; }
struct power_module HAL_MODULE_INFO_SYM = {
    .common = { .tag = HARDWARE_MODULE_TAG, .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION, .id = POWER_HARDWARE_MODULE_ID,
        .name = "LG first-boot power", .author = "Lineage port helper" },
    .init = init, .setInteractive = interactive, .powerHint = hint
};
