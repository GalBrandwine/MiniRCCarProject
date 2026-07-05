/*
 * Copyright (c) 2021, Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <app/drivers/remote_control_2ch.h>
#include <app/lib/rc_car.h>

LOG_MODULE_REGISTER(rc_car, CONFIG_RC_CAR_LOG_LEVEL);

int custom_get_value(int val)
{
	return (val != 0) ? val : CONFIG_RC_CAR_GET_VALUE_DEFAULT;
}

static const struct device *rc_dev;

int rc_car_init(const struct device *dev)
{
	if (!device_is_ready(dev))
	{
		LOG_ERR("RC car device not ready");
		return -ENODEV;
	}
	rc_dev = dev;
	LOG_INF("RC car controller initialised");
	return 0;
}

int rc_car_turn_left(void)
{
	LOG_WRN("WORK_IN_PROGRESS - RC car turning left");
	return DEVICE_API_GET(remote_control_2ch, rc_dev)->turn_left(rc_dev);
}

int rc_car_center_wheels(void)
{
	LOG_DBG("RC car centering wheels");
	return DEVICE_API_GET(remote_control_2ch, rc_dev)->center(rc_dev);
}

int rc_car_turn_right(void)
{
	LOG_DBG("RC car turning right");
	return DEVICE_API_GET(remote_control_2ch, rc_dev)->turn_right(rc_dev);
}