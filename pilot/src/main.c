/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define TELEMETRY_IP   "192.0.2.0"
#define TELEMETRY_PORT 7742

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA_FOR_INPUTS(node_id, prop, idx)                                           \
	COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, prop, idx, input), \
		    (ADC_DT_SPEC_GET_BY_IDX(node_id, idx),), ())

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA_FOR_INPUTS)};

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

	int err;

	uint32_t buf = 0;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

	while (1) {
		printk("-----\n");
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			/*
			 * Clear buffer before reading.  This ensures the upper 16-bits will be zero
			 * when the adc uses a 16-bit buffer size.
			 */
			buf = 0;

			printk("- %s, channel %d: ", adc_channels[i].dev->name,
			       adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read_dt(&adc_channels[i], &sequence);
			if (err < 0) {
				printk("Could not read (%d)\n", err);
				continue;
			}

			/*
			 * If using differential mode, the 16 bit value
			 * in the ADC sample buffer should be a signed 2's
			 * complement value.
			 */
			if (adc_channels[i].channel_cfg.differential) {
				val_mv = (int32_t)((int16_t)buf);
			} else {
				val_mv = (int32_t)buf;
			}
			printk("%" PRId32, val_mv);
			err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
			/* conversion to mV may not be supported, skip if not */
			if (err < 0) {
				printk(" (value in mV not available)\n");
			} else {
				printk(" = %" PRId32 " mV\n", val_mv);
			}

			k_sleep(K_MSEC(1000));
		}
	}

	return 0;
}
