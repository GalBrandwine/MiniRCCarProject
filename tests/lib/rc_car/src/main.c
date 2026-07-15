/*
 * Copyright (c) 2021 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file test custom_lib library
 *
 * This suite verifies that the methods provided with the custom_lib
 * library works correctly.
 */

#include <limits.h>

#include <zephyr/ztest.h>

#include <app/lib/rc_car.h>

#include <app/drivers/remote_control_2ch.h>

ZTEST(rc_car_tests, test_turn_left)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(remote_control));
	int ret;

	ret = rc_car_init(dev);
	zassert_equal(ret, 0, "rc_car_init failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), CENTERED, "steering should be CENTERED after init");
	zassert_equal(rc_car_get_acceleration_status(), STOPPED, "acceleration should be STOPPED after init");

	ret = rc_car_turn_left();
	zassert_equal(ret, 0, "turn_left failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), TURN_LEFT, "steering should be TURN_LEFT after turn_left");
}

ZTEST(rc_car_tests, test_turn_right)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(remote_control));
	int ret;

	ret = rc_car_init(dev);
	zassert_equal(ret, 0, "rc_car_init failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), CENTERED, "steering should be %s after init", car_steering_to_str(CENTERED));
	zassert_equal(rc_car_get_acceleration_status(), STOPPED, "acceleration should be %s after init", car_acceleration_to_str(STOPPED));

	ret = rc_car_turn_right();
	zassert_equal(ret, 0, "turn_right failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), TURN_RIGHT, "steering should be %s after turn_right", car_steering_to_str(TURN_RIGHT));
}

ZTEST(rc_car_tests, test_center_wheels)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(remote_control));
	int ret;

	ret = rc_car_init(dev);
	zassert_equal(ret, 0, "rc_car_init failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), CENTERED, "steering should be %s after init", car_steering_to_str(CENTERED));
	zassert_equal(rc_car_get_acceleration_status(), STOPPED, "acceleration should be %s after init", car_acceleration_to_str(STOPPED));

	/* exercise center wheels after turning left to ensure sequence works */
	ret = rc_car_turn_left();
	zassert_equal(ret, 0, "turn_left failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), TURN_LEFT, "steering should be %s after turn_left", car_steering_to_str(TURN_LEFT));

	ret = rc_car_center_wheels();
	zassert_equal(ret, 0, "center_wheels failed: %d", ret);
	zassert_equal(rc_car_get_steering_status(), CENTERED, "steering should be %s after center_wheels", car_steering_to_str(CENTERED));
}

ZTEST(rc_car_tests, test_init_null_device)
{
	/* Test that init fails gracefully with NULL device */
	int ret = rc_car_init(NULL);
	zassert_not_equal(ret, 0, "init should fail with NULL device");
}

// ZTEST(rc_car_tests, test_get_value)
// {
// 	/* Verify standard behavior */
// 	zassert_equal(custom_get_value(INT_MIN), INT_MIN,
// 				  "get_value failed input of INT_MIN");
// 	zassert_equal(custom_get_value(INT_MIN + 1), INT_MIN + 1,
// 				  "get_value failed input of INT_MIN + 1");
// 	zassert_equal(custom_get_value(-1), -1,
// 				  "get_value failed input of -1");
// 	zassert_equal(custom_get_value(1), 1,
// 				  "get_value failed input of 1");
// 	zassert_equal(custom_get_value(INT_MAX - 1), INT_MAX - 1,
// 				  "get_value failed input of INT_MAX - 1");
// 	zassert_equal(custom_get_value(INT_MAX), INT_MAX,
// 				  "get_value failed input of INT_MAX");

// 	/* Verify override behavior */
// 	zassert_equal(custom_get_value(0),
// 				  CONFIG_CUSTOM_GET_VALUE_DEFAULT,
// 				  "get_value failed input of 0");
// }

ZTEST_SUITE(rc_car_tests, NULL, NULL, NULL, NULL, NULL);
