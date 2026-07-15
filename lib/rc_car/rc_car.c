/*
 * Copyright (c) 2021, Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/logging/log.h>
#include <app/drivers/remote_control_2ch.h>
#include <app/lib/rc_car.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(rc_car, CONFIG_RC_CAR_LOG_LEVEL);

int custom_get_value(int val)
{
	return (val != 0) ? val : CONFIG_RC_CAR_GET_VALUE_DEFAULT;
}

enum CAR_ACCELERATION
{
	FORWARD,
	BACKWARD,
	STOPPED
};

static const char *car_acceleration_to_str(enum CAR_ACCELERATION status)
{
	switch (status)
	{
	case FORWARD:
		return "forward";
	case BACKWARD:
		return "backward";
	case STOPPED:
		return "stopped";
	default:
		return "NOT_SUPPORTED";
	}
}

enum CAR_STEERING
{
	CENTERED,
	TURN_LEFT,
	TURN_RIGHT
};

static const char *car_steering_to_str(enum CAR_STEERING status)
{
	switch (status)
	{
	case CENTERED:
		return "centered";
	case TURN_LEFT:
		return "turning left";
	case TURN_RIGHT:
		return "turning right";
	default:
		return "NOT_SUPPORTED";
	}
}

static struct rc_car
{
	enum CAR_STEERING steering_status;
	enum CAR_ACCELERATION acceleration_status;
} rc_car;

static const struct device *rc_dev;

int rc_car_init(const struct device *dev)
{
	if (!device_is_ready(dev))
	{
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}
	rc_dev = dev;
	rc_car.steering_status = CENTERED;
	rc_car.acceleration_status = STOPPED;
	LOG_INF("RC car controller initialised;");
	LOG_INF("Its steering status: %s", car_steering_to_str(rc_car.steering_status));
	LOG_INF("Its acceleration status: %s", car_acceleration_to_str(rc_car.acceleration_status));
	return 0;
}

int rc_car_turn_left(void)
{
	int ret = DEVICE_API_GET(remote_control_2ch, rc_dev)->turn_left(rc_dev);
	if (ret == 0)
	{
		rc_car.steering_status = TURN_LEFT;
		return 0;
	}

	return ret;
}

int rc_car_center_wheels(void)
{
	int ret = DEVICE_API_GET(remote_control_2ch, rc_dev)->center(rc_dev);
	if (ret == 0)
	{
		rc_car.steering_status = CENTERED;
		return 0;
	}
	return ret;
}

int rc_car_turn_right(void)
{
	int ret = DEVICE_API_GET(remote_control_2ch, rc_dev)->turn_right(rc_dev);
	if (ret == 0)
	{
		rc_car.steering_status = TURN_RIGHT;
		return 0;
	}
	return ret;
}

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0)
	{
		return;
	}

	printk("Button %d %s at %" PRIu32 "\n",
		   evt->code,
		   evt->value ? "pressed" : "released",
		   k_cycle_get_32());
	switch (evt->code)
	{
	case INPUT_KEY_0:
		if (evt->value)
		{
			LOG_DBG("Button Steer Left Pressed!");
			rc_car_turn_left();
		}
		else
		{
			LOG_DBG("Button Steer Left Released!");
			rc_car_center_wheels();
		}
		break;
	case INPUT_KEY_1:
		if (evt->value)
		{
			LOG_DBG("Button Steer Right Pressed!");
			rc_car_turn_right();
		}
		else
		{
			LOG_DBG("Button Steer Right Released!");
			rc_car_center_wheels();
		}
		break;
	case INPUT_KEY_2:
		if (evt->value)
		{
			LOG_WRN("WIP - Button Move Forward Pressed!");
		}
		else
		{
			LOG_WRN("WIP - Button Move Forward Released!");
		}
		break;
	case INPUT_KEY_3:
		if (evt->value)
		{
			LOG_WRN("WIP - Button Move Backward Pressed!");
		}
		else
		{
			LOG_WRN("WIP - Button Move Backward Released!");
		}
		break;
	default:
		break;
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);