/* SPDX-License-Identifier: Apache-2.0 */
#define LOG_TAG "LgeBtKeysizeProbe"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "keysize_probe.h"
#include "bahama_bridge.h"

#define KEY_SIZE_OPCODE 0x1408
#define PROBE_FLAG "debug.lge.bt.keysize_probe"
#define PROBE_STATUS "vendor.lge.bt.keysize.status"
#define PROBE_RESPONSE "vendor.lge.bt.keysize.response"

/* Public bt_vendor_lib.h documents this 8-byte HCI transport header. */
typedef struct {
    uint16_t event;
    uint16_t len;
    uint16_t offset;
    uint16_t layer_specific;
    uint8_t data[];
} lge_bt_packet;
typedef char header_must_be_eight_bytes[sizeof(lge_bt_packet) == 8 ? 1 : -1];

static pthread_mutex_t probe_lock = PTHREAD_MUTEX_INITIALIZER;
static const bt_vendor_callbacks_t *probe_callbacks;
static bool probe_pending;

static void finish_probe(const char *status, const char *response) {
    const bt_vendor_callbacks_t *callbacks;
    pthread_mutex_lock(&probe_lock);
    if (!probe_pending) {
        pthread_mutex_unlock(&probe_lock);
        return;
    }
    probe_pending = false;
    callbacks = probe_callbacks;
    pthread_mutex_unlock(&probe_lock);
    property_set(PROBE_RESPONSE, response);
    property_set(PROBE_STATUS, status);
    ALOGI("Read Encryption Key Size, reserved handle 0x0fff: %s response=%s",
          status, response);
    /* Firmware was configured before this diagnostic. Report its real result.
     * A diagnostic failure does not certify the controller's key size. */
    callbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
}

static void probe_reply(void *memory) {
    const bt_vendor_callbacks_t *callbacks;
    pthread_mutex_lock(&probe_lock);
    callbacks = probe_callbacks;
    bool active = probe_pending;
    if (active) {
        const lge_bt_packet *packet = memory;
        if (packet && packet->offset == 0 && packet->len == 9)
            lge_bahama_confirm(packet->data, packet->len);
        else lge_bahama_confirm(NULL, 0);
    }
    pthread_mutex_unlock(&probe_lock);
    char status[32] = "malformed-event";
    char response[41] = "";
    if (memory) {
        const lge_bt_packet *packet = memory;
        if (packet->offset == 0 && packet->len >= 6 && packet->len <= 257) {
            const uint8_t *data = packet->data + packet->offset;
            if (data[0] == 0x0e && (unsigned)data[1] + 2 == packet->len &&
                    data[3] == 0x08 && data[4] == 0x14) {
                snprintf(status, sizeof(status), "status-0x%02x", data[5]);
                unsigned count = packet->len < 20 ? packet->len : 20;
                for (unsigned i = 0; i < count; ++i)
                    snprintf(response + 2 * i, sizeof(response) - 2 * i, "%02x", data[i]);
            }
        }
        /* The HIDL bridge owns command buffers; vendor owns reply buffers. */
        callbacks->dealloc(memory);
    }
    finish_probe(status, response);
}

bool lge_bt_keysize_probe(const bt_vendor_callbacks_t *callbacks) {
    char requested[PROP_VALUE_MAX];
    property_get(PROBE_FLAG, requested, "0");
    if (strcmp(requested, "1") && !lge_bahama_enabled())
        return false;
    if (!callbacks || callbacks->size < offsetof(bt_vendor_callbacks_t, xmit_cb) + sizeof(callbacks->xmit_cb) ||
            !callbacks->alloc || !callbacks->dealloc || !callbacks->xmit_cb || !callbacks->fwcfg_cb) {
        property_set(PROBE_STATUS, "missing-callbacks");
        return false;
    }
    pthread_mutex_lock(&probe_lock);
    if (probe_pending) {
        pthread_mutex_unlock(&probe_lock);
        return true;
    }
    lge_bahama_clear();
    probe_pending = true;
    probe_callbacks = callbacks;
    pthread_mutex_unlock(&probe_lock);
    lge_bt_packet *command = callbacks->alloc(sizeof(lge_bt_packet) + 5);
    if (!command) {
        finish_probe("allocation-failed", "");
        return true;
    }
    memset(command, 0, sizeof(lge_bt_packet) + 5);
    command->event = 0x2000;
    command->len = 5;
    command->data[0] = 0x08;
    command->data[1] = 0x14;
    command->data[2] = 2;
    command->data[3] = 0xff;
    command->data[4] = 0x0f;
    property_set(PROBE_RESPONSE, "");
    property_set(PROBE_STATUS, "pending");
    /* Request only an invalid/reserved handle: never read an active link key.
     * FW_CFG completes from the actual controller reply. A V1 enable qualifies
     * only the bitmap quirk; real connected-link responses remain untouched. */
    if (!callbacks->xmit_cb(KEY_SIZE_OPCODE, command, probe_reply)) {
        callbacks->dealloc(command);
        finish_probe("transmit-failed", "");
    }
    return true;
}

void lge_bt_keysize_cancel(void) {
    pthread_mutex_lock(&probe_lock);
    probe_pending = false;
    lge_bahama_clear();
    pthread_mutex_unlock(&probe_lock);
}
