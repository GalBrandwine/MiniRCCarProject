/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_DRIVERS_2CH_REMOTE_CONTROLL_H_
#define APP_DRIVERS_2CH_REMOTE_CONTROLL_H_

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup drivers_2ch_remote_controll 2CH Remote Control drivers
 * @ingroup drivers
 * @{
 *
 * @brief A custom driver class for 2CH remote control
 *
 * This driver class is provided as an example of how to create custom driver
 * classes. It provides an interface to blink an LED at a configurable rate.
 * Implementations could include simple GPIO-controlled LEDs, addressable LEDs,
 * etc.
 */

/**
 * @defgroup drivers_2ch_remote_controll_ops 2CH Remote Control driver operations
 * @{
 *
 * @brief Operations of the 2CH Remote Control driver class.
 *
 * Each driver class tipically provides a set of operations that need to be
 * implemented by each driver. These are used to implement the public API. If
 * support for system calls is needed, the operations structure must be tagged
 * with `__subsystem` and follow the `${class}_driver_api` naming scheme.
 */

/** @brief Blink driver class operations */
__subsystem struct remote_controll_2ch_driver_api
{
	// /**
	//  * @brief Configure the LED blink period.
	//  *
	//  * @param dev Blink device instance.
	//  * @param period_ms Period of the LED blink in milliseconds, 0 to
	//  * disable blinking.
	//  *
	//  * @retval 0 if successful.
	//  * @retval -EINVAL if @p period_ms can not be set.
	//  * @retval -errno Other negative errno code on failure.
	//  */
	// int (*set_period_ms)(const struct device *dev, unsigned int period_ms);

	/**
	 * @brief Turn the remote control left.
	 *
	 * @param dev 2ch_remote_controll device instance.
	 *
	 * @retval 0 if successful.
	 * @retval -EINVAL if @p period_ms can not be set.
	 * @retval -errno Other negative errno code on failure.
	 */
	int (*turn_left)(const struct device *dev);
	int (*turn_right)(const struct device *dev);
};

/** @} */

/** @} */

#endif /* APP_DRIVERS_2CH_REMOTE_CONTROLL_H_ */
