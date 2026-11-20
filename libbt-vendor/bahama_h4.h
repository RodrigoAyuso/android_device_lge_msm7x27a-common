/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_BAHAMA_H4_H
#define LGE_BAHAMA_H4_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ACL/SCO payload is streamed; only a small HCI event is buffered. */
struct lge_h4_rx {
    uint8_t header[5];
    size_t header_have, header_need;
    uint32_t remaining;
    uint8_t event[258];
    size_t event_have;
    bool event_packet;
    bool version_matches;
    bool version_seen;
    uint8_t hci_version, lmp_version;
    uint16_t manufacturer, subversion;
    unsigned corrected;
};
typedef int (*lge_h4_emit)(void*, const uint8_t*, size_t);
int lge_h4_feed(struct lge_h4_rx*, const uint8_t*, size_t,
                bool handler_confirmed, lge_h4_emit, void*);
bool lge_bahama_probe_reply(const uint8_t*, size_t);
#endif
