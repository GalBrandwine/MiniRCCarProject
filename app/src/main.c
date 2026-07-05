/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/lib/rc_car.h>
// #include <app/drivers/remote_control_2ch.h>

#include <app_version.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(remote_control));

	printk("Zephyr Remote Controlled Car Application %s\n", APP_VERSION_STRING);

	if (0 != rc_car_init(dev))
	{
		LOG_ERR("Remote control 2ch not ready");
		return 0;
	}

	while (1)
	{
		LOG_INF("");
		LOG_INF("Turning left");
		rc_car_turn_left();
		// DEVICE_API_GET(remote_control_2ch, remote_control_2ch_dev)->turn_left(remote_control_2ch_dev);
		k_sleep(K_MSEC(1000));

		LOG_INF("Centering");
		// DEVICE_API_GET(remote_control_2ch, remote_control_2ch_dev)->center(remote_control_2ch_dev);
		rc_car_center_wheels();
		k_sleep(K_MSEC(1000));

		LOG_INF("Turning right");
		rc_car_turn_right();
		// DEVICE_API_GET(remote_control_2ch, remote_control_2ch_dev)->turn_right(remote_control_2ch_dev);
		k_sleep(K_MSEC(1000));

		LOG_INF("Centering");
		// DEVICE_API_GET(remote_control_2ch, remote_control_2ch_dev)->center(remote_control_2ch_dev);
		rc_car_center_wheels();
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
