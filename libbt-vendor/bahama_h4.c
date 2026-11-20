/* SPDX-License-Identifier: Apache-2.0 */
#include "bahama_h4.h"
#include <string.h>

bool lge_bahama_probe_reply(const uint8_t* p, size_t n)
{
    /* Real Command Complete: opcode 0x1408, Unknown Connection Identifier,
     * reserved handle 0x0fff. No success/key-size response is invented. */
    return p && n == 9 && p[0] == 0x0e && p[1] == 7 && p[2] != 0 &&
        p[3] == 0x08 && p[4] == 0x14 && p[5] == 0x02 &&
        p[6] == 0xff && p[7] == 0x0f && p[8] == 0;
}

static void inspect_event(struct lge_h4_rx* s, bool confirmed)
{
    uint8_t* p = s->event;
    size_t n = s->event_have;
    if (n < 7 || p[0] != 4 || p[1] != 0x0e || p[2] + 3U != n || !p[3])
        return;
    unsigned opcode = (unsigned)p[4] | ((unsigned)p[5] << 8);
    if (opcode == 0x1001) s->version_matches = s->version_seen = false;
    if (p[6]) return;
    if (opcode == 0x0c03) s->version_matches = s->version_seen = false;
    if (opcode == 0x1001) {
        /* Observed Qualcomm Bahama B1: HCI/LMP 4.0, manufacturer 29,
         * LMP subversion 0x0014. Other controllers are passed unchanged. */
        s->version_seen = n == 15;
        s->version_matches = n == 15 && p[7] == 6 && !p[8] && !p[9] && p[10] == 6 &&
            p[11] == 29 && p[12] == 0 && p[13] == 20 && p[14] == 0;
        if (n == 15) {
            s->hci_version = p[7]; s->lmp_version = p[10];
            s->manufacturer = (uint16_t)p[11] | ((uint16_t)p[12] << 8);
            s->subversion = (uint16_t)p[13] | ((uint16_t)p[14] << 8);
        }
    }
    if (opcode == 0x1002 && n == 71 && confirmed && s->version_matches &&
        !(p[7 + 20] & 0x10)) {
        p[7 + 20] |= 0x10;
        ++s->corrected;
    }
}

int lge_h4_feed(struct lge_h4_rx* s, const uint8_t* data, size_t count,
                bool confirmed, lge_h4_emit emit, void* opaque)
{
    while (count) {
        if (s->remaining) {
            size_t n = count < s->remaining ? count : s->remaining;
            if (s->event_packet) {
                memcpy(s->event + s->event_have, data, n);
                s->event_have += n;
            } else if (emit(opaque, data, n)) return -1;
            s->remaining -= (uint32_t)n;
            data += n; count -= n;
            if (!s->remaining && s->event_packet) {
                inspect_event(s, confirmed);
                if (emit(opaque, s->event, s->event_have)) return -1;
            }
            continue;
        }
        if (!s->header_have) {
            uint8_t type = *data++; --count;
            s->header[0] = type; s->header_have = 1;
            s->header_need = type == 4 ? 3 :
                (type == 2 || type == 5) ? 5 :
                (type == 1 || type == 3) ? 4 : 1;
            if (s->header_need == 1) {
                if (emit(opaque, s->header, 1)) return -1;
                s->header_have = 0;
                continue;
            }
        }
        size_t n = s->header_need - s->header_have;
        if (n > count) n = count;
        memcpy(s->header + s->header_have, data, n);
        s->header_have += n; data += n; count -= n;
        if (s->header_have != s->header_need) continue;
        uint8_t type = s->header[0];
        s->remaining = type == 4 ? s->header[2] :
            (type == 2 || type == 5) ?
                (uint32_t)s->header[3] | ((uint32_t)s->header[4] << 8) :
                s->header[3];
        if (type == 5) s->remaining &= 0x3fff;
        s->event_packet = type == 4;
        if (s->event_packet) {
            memcpy(s->event, s->header, s->header_need);
            s->event_have = s->header_need;
        } else if (emit(opaque, s->header, s->header_need)) return -1;
        s->header_have = 0;
        if (!s->remaining && s->event_packet) {
            if (emit(opaque, s->event, s->event_have)) return -1;
        }
    }
    return 0;
}
