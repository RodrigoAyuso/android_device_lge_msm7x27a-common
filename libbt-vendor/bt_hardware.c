/*
 * Copyright (C) 2013  Rudolf Tammekivi <rtammekivi@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see [http://www.gnu.org/licenses/].
 */

#define LOG_TAG "bt_vendor_qcom"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#include <bt_hardware.h>
#include <bt_helper.h>

/* Restart-safe Bahama initialization; the HIDL service may have died mid-enable. */
char bt_hardware_power_state_path[64];
static int bt_serial_fd = -1;

int bt_hardware_power(bool enable)
{
    if (bt_hardware_power_state_path[0] == '\0') return -1;
    if (!enable) {
        int ret = write_value(bt_hardware_power_state_path, "0", 1);
        property_set("vendor.lge.bt.firmware.status", ret < 0 ? "power-off-failed" : "off");
        return ret;
    }
    /* Never reset a UART that is already in use by this process. */
    if (bt_serial_fd >= 0) { errno = EBUSY; return -1; }
    property_set("vendor.lge.bt.firmware.status", "cold-start");
    if (write_value(bt_hardware_power_state_path, "0", 1) < 0) return -1;
    usleep(100000);
    if (write_value(bt_hardware_power_state_path, "1", 1) < 0) return -1;
    if (bt_hardware_download_firmware() < 0) {
        /* A failed download must not leave a warm, half-configured controller. */
        write_value(bt_hardware_power_state_path, "0", 1);
        property_set("vendor.lge.bt.firmware.status", "download-failed");
        return -1;
    }
    property_set("vendor.lge.bt.firmware.status", "ready");
    return 0;
}

int bt_hardware_serial(bool open_uart)
{
    struct termios term;
    if (!open_uart) {
        if (bt_serial_fd >= 0) close(bt_serial_fd);
        bt_serial_fd = -1;
        return 0;
    }
    if (bt_serial_fd >= 0) return bt_serial_fd;
    bt_serial_fd = open(BT_HS_UART_DEVICE, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (bt_serial_fd < 0) {
        ALOGE("open UART failed: %s", strerror(errno));
        return -1;
    }
    if (tcflush(bt_serial_fd, TCIOFLUSH) < 0 ||
        tcgetattr(bt_serial_fd, &term) < 0) goto fail;
    cfmakeraw(&term);
    term.c_cflag |= CRTSCTS | CLOCAL;
    if (tcsetattr(bt_serial_fd, TCSANOW, &term) < 0) goto fail;
    return bt_serial_fd;
fail:
    ALOGE("configure UART failed: %s", strerror(errno));
    close(bt_serial_fd);
    bt_serial_fd = -1;
    return -1;
}

int bt_hardware_download_firmware(void)
{
    /* Keep the original H4/IBS arguments and the actual local address.
     * Capture the tool's output instead of discarding the reason for failure.
     * Only fixed text and hexadecimal bytes enter this command. */
    char command[256];
    char line[256];
    snprintf(command, sizeof(command), "/system/bin/hci_qcomm_init"
        " --force-hw-sleep --board-address %02X:%02X:%02X:%02X:%02X:%02X 2>&1",
        bt_vendor_local_bdaddr[0], bt_vendor_local_bdaddr[1],
        bt_vendor_local_bdaddr[2], bt_vendor_local_bdaddr[3],
        bt_vendor_local_bdaddr[4], bt_vendor_local_bdaddr[5]);
    FILE *output = popen(command, "r");
    if (!output) {
        ALOGE("cannot launch hci_qcomm_init: %s", strerror(errno));
        return -1;
    }
    while (fgets(line, sizeof(line), output)) ALOGI("hci_qcomm_init: %s", line);
    int status = pclose(output);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        ALOGE("hci_qcomm_init failed, wait status=%d", status);
        return -1;
    }
    return 0;
}

int bt_hardware_init(void)
{
	return bt_get_rfkill_state_path(bt_hardware_power_state_path,
		sizeof(bt_hardware_power_state_path));
}
