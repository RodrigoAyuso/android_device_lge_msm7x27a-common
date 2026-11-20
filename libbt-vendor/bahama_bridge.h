/* SPDX-License-Identifier: Apache-2.0 */
#ifndef LGE_BAHAMA_BRIDGE_H
#define LGE_BAHAMA_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void lge_bahama_prepare(void);
bool lge_bahama_enabled(void);
void lge_bahama_confirm(const uint8_t*, size_t);
void lge_bahama_clear(void);
int lge_bahama_open(int uart_fd);
void lge_bahama_close(void);
#endif
