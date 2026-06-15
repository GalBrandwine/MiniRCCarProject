/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app/drivers/remote_controll_2ch.h>

#include <app_version.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	int ret;
	// unsigned int period_ms = BLINK_PERIOD_MS_MAX;
	const struct device *remote_controll_2ch_dev;
	struct sensor_value last_val = {0}, val;

	printk("Zephyr Remote Controlled Car Application %s\n", APP_VERSION_STRING);

	remote_controll_2ch_dev = DEVICE_DT_GET(DT_NODELABEL(remote_controll));
	if (!device_is_ready(remote_controll_2ch_dev))
	{
		LOG_ERR("Remote controll 2ch not ready");
		return 0;
	}

	DEVICE_API_GET(remote_controll_2ch, remote_controll_2ch_dev)->turn_left(remote_controll_2ch_dev);

	printk("Use the sensor to change LED blinking period\n");

	while (1)
	{
		LOG_INF("Turning left");
		DEVICE_API_GET(remote_controll_2ch, remote_controll_2ch_dev)->turn_left(remote_controll_2ch_dev);
		k_sleep(K_MSEC(1000));

		DEVICE_API_GET(remote_controll_2ch, remote_controll_2ch_dev)->center(remote_controll_2ch_dev);
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
