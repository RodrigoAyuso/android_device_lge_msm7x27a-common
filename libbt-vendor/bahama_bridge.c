/* SPDX-License-Identifier: Apache-2.0 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define LOG_TAG "LgeBahamaH4"
#include "bahama_bridge.h"
#include "bahama_h4.h"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static pthread_mutex_t life_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t confirm_lock = PTHREAD_MUTEX_INITIALIZER;
static bool enabled, confirmed;
static struct {
    int uart, old_flags, host, peer, stop_read, stop_write;
    pthread_t rx, tx;
    bool active, rx_started, tx_started;
} bridge = { .uart = -1, .old_flags = -1, .host = -1, .peer = -1,
             .stop_read = -1, .stop_write = -1 };

void lge_bahama_clear(void)
{
    pthread_mutex_lock(&confirm_lock);
    confirmed = false;
    pthread_mutex_unlock(&confirm_lock);
}

void lge_bahama_prepare(void)
{
    char hardware[PROP_VALUE_MAX];
    property_get("ro.hardware", hardware, "");
    enabled = !strcmp(hardware, "v1");
    lge_bahama_clear();
    if (enabled) property_set("vendor.lge.bt.keysize.quirk", "waiting-probe");
}

bool lge_bahama_enabled(void) { return enabled; }

static bool is_confirmed(void)
{
    pthread_mutex_lock(&confirm_lock);
    bool value = confirmed;
    pthread_mutex_unlock(&confirm_lock);
    return value;
}

void lge_bahama_confirm(const uint8_t* data, size_t count)
{
    bool valid = enabled && lge_bahama_probe_reply(data, count);
    pthread_mutex_lock(&confirm_lock);
    confirmed = valid;
    pthread_mutex_unlock(&confirm_lock);
    if (enabled)
        property_set("vendor.lge.bt.keysize.quirk", valid ? "handler-confirmed" : "not-confirmed");
}

static int wait_fd(int fd, short events)
{
    struct pollfd p[2] = {{bridge.stop_read, POLLIN, 0}, {fd, events, 0}};
    for (;;) {
        int result = poll(p, 2, -1);
        if (result < 0 && errno == EINTR) continue;
        if (result < 0 || p[0].revents) return -1;
        if (p[1].revents & events) return 0;
        if (p[1].revents & (POLLHUP | POLLERR | POLLNVAL)) return -1;
    }
}

static int send_all(int fd, bool socket_fd, const uint8_t* data, size_t count)
{
    while (count) {
        if (wait_fd(fd, POLLOUT)) return -1;
        ssize_t n = socket_fd ? send(fd, data, count, MSG_NOSIGNAL) : write(fd, data, count);
        if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (n <= 0) return -1;
        data += n; count -= (size_t)n;
    }
    return 0;
}

static int emit_rx(void* opaque, const uint8_t* data, size_t count)
{
    (void)opaque;
    return send_all(bridge.peer, true, data, count);
}

static void signal_stop(void)
{
    if (bridge.stop_write >= 0) {
        uint8_t stop = 1;
        ssize_t ret;
        do { ret = write(bridge.stop_write, &stop, 1); } while (ret < 0 && errno == EINTR);
    }
}

static void* receive_controller(void* opaque)
{
    (void)opaque;
    struct lge_h4_rx state;
    memset(&state, 0, sizeof(state));
    uint8_t buffer[4096];
    while (!wait_fd(bridge.uart, POLLIN)) {
        ssize_t n = read(bridge.uart, buffer, sizeof(buffer));
        if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (n <= 0) break;
        unsigned old_count = state.corrected;
        bool old_version_seen = state.version_seen;
        if (lge_h4_feed(&state, buffer, (size_t)n, is_confirmed(), emit_rx, NULL)) break;
        if (!old_version_seen && state.version_seen) {
            ALOGI("Controller HCI=%u LMP=%u manufacturer=%u subversion=%u match=%d",
                  state.hci_version, state.lmp_version, state.manufacturer,
                  state.subversion, state.version_matches);
            if (!state.version_matches)
                property_set("vendor.lge.bt.keysize.quirk", "controller-mismatch");
        }
        if (state.corrected != old_count) {
            property_set("vendor.lge.bt.keysize.quirk", "active");
            ALOGI("Bahama B1: corrected command bitmap after real 0x1408 response; link key-size queries unchanged");
        }
    }
    signal_stop();
    shutdown(bridge.peer, SHUT_RDWR);
    return NULL;
}

static void* transmit_controller(void* opaque)
{
    (void)opaque;
    uint8_t buffer[4096];
    while (!wait_fd(bridge.peer, POLLIN)) {
        ssize_t n = recv(bridge.peer, buffer, sizeof(buffer), 0);
        if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (n <= 0 || send_all(bridge.uart, false, buffer, (size_t)n)) break;
    }
    signal_stop();
    shutdown(bridge.peer, SHUT_RDWR);
    return NULL;
}

static void close_locked(void)
{
    signal_stop();
    if (bridge.rx_started) pthread_join(bridge.rx, NULL);
    if (bridge.tx_started) pthread_join(bridge.tx, NULL);
    bridge.rx_started = bridge.tx_started = false;
    if (bridge.uart >= 0 && bridge.old_flags >= 0)
        fcntl(bridge.uart, F_SETFL, bridge.old_flags);
    int* fds[] = {&bridge.host, &bridge.peer, &bridge.stop_read, &bridge.stop_write};
    for (size_t i = 0; i < sizeof(fds)/sizeof(fds[0]); ++i) {
        if (*fds[i] >= 0) close(*fds[i]);
        *fds[i] = -1;
    }
    bridge.uart = bridge.old_flags = -1;
    bridge.active = false;
}

int lge_bahama_open(int uart_fd)
{
    if (!enabled) return uart_fd;
    pthread_mutex_lock(&life_lock);
    if (bridge.active || uart_fd < 0) {
        pthread_mutex_unlock(&life_lock);
        errno = EBUSY;
        return -1;
    }
    int sockets[2], stop[2];
    pthread_attr_t attr;
    bool attr_ready = false;
    int error = 0;
    bridge.uart = uart_fd;
    bridge.old_flags = fcntl(uart_fd, F_GETFL);
    if (bridge.old_flags < 0) goto fail;
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets)) goto fail;
    bridge.host = sockets[0]; bridge.peer = sockets[1];
    if (pipe2(stop, O_CLOEXEC | O_NONBLOCK)) goto fail;
    bridge.stop_read = stop[0]; bridge.stop_write = stop[1];
    if (fcntl(bridge.uart, F_SETFL, bridge.old_flags | O_NONBLOCK) ||
        fcntl(bridge.peer, F_SETFL, O_NONBLOCK)) goto fail;
    error = pthread_attr_init(&attr);
    if (error) { errno = error; goto fail; }
    attr_ready = true;
    error = pthread_attr_setstacksize(&attr, 64U * 1024U);
    if (error) { errno = error; goto fail; }
    error = pthread_create(&bridge.rx, &attr, receive_controller, NULL);
    if (error) { errno = error; goto fail; }
    bridge.rx_started = true;
    error = pthread_create(&bridge.tx, &attr, transmit_controller, NULL);
    if (error) { errno = error; goto fail; }
    bridge.tx_started = true;
    pthread_attr_destroy(&attr);
    bridge.active = true;
    int result = bridge.host;
    pthread_mutex_unlock(&life_lock);
    return result;
fail:
    error = errno;
    if (attr_ready) pthread_attr_destroy(&attr);
    close_locked();
    pthread_mutex_unlock(&life_lock);
    errno = error;
    ALOGE("Cannot open Bahama H4 bridge: %s", strerror(error));
    return -1;
}

void lge_bahama_close(void)
{
    if (!enabled) return;
    pthread_mutex_lock(&life_lock);
    close_locked();
    pthread_mutex_unlock(&life_lock);
}
