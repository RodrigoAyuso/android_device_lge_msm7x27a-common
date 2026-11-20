/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_BT_KEYSIZE_PROBE_H
#define LGE_BT_KEYSIZE_PROBE_H
#include <stdbool.h>
#include <stddef.h>
#include <bt_vendor_lib.h>

/* V1 qualification uses a real response on each enable; other devices
 * retain the explicit diagnostic flag. Link-key replies are unchanged. */
bool lge_bt_keysize_probe(const bt_vendor_callbacks_t *callbacks);
void lge_bt_keysize_cancel(void);
#endif
