/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define TELEMETRY_IP   "192.0.2.0"
#define TELEMETRY_PORT 7742

int main(void)
{
	int sock;
	struct sockaddr_in destination_addr;
	char msg[] = "Hello from Zephyr";

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printk("Failed to create socket\n");
		return -1;
	}
	printk("Opened socket\n");

	destination_addr.sin_family = AF_INET;
	destination_addr.sin_port = htons(TELEMETRY_PORT);
	zsock_inet_pton(AF_INET, TELEMETRY_IP, &destination_addr.sin_addr);

	printk("Sending bytes\n");
	ssize_t bytes_sent =
		zsock_sendto(sock, msg, sizeof(msg), 0, (struct sockaddr *)&destination_addr,
			     sizeof(destination_addr));

	if (bytes_sent < 0) {
		printk("Failed to send packet\n");
	} else {
		printk("Sent %d bytes\n", bytes_sent);
	}

	zsock_close(sock);

	return 0;
}
