/*
 * Copyright (c) 2021, Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LIB_RC_CAR_H_
#define APP_LIB_RC_CAR_H_

#include <zephyr/device.h>

/**
 * @defgroup lib_rc_car Car Remote Controll library
 * @ingroup lib
 * @{
 *
 * @brief My own out-of-tree library for controlling a remote controll car
 *
 * This library illustrates how create custom out-of-tree libraries. Splitting
 * code in libraries enables code reuse and modularity, also easing testing.
 */

/**
 * @brief Return @p val if non-zero, or Kconfig-controlled default.
 *
 * Function returns the provided value if non-zero, or a Kconfig-controlled
 * default value if the parameter is zero. This trivial function is provided in
 * order to have a library interface example that is trivial to test.
 *
 * @param val Value to return if non-zero
 *
 * @retval val if @p val is non-zero
 * @retval CONFIG_CUSTOM_GET_VALUE_DEFAULT if @p val is zero
 */
int custom_get_value(int val);

/**
 * @brief Initialize the RC car controller.
 *
 * @param dev Remote control 2ch device
 * @return 0 on success, negative errno on failure
 */
int rc_car_init(const struct device *dev);

/**
 * @brief Turn the RC car left.
 * @return 0 on success, negative errno on failure
 */
int rc_car_turn_left(void);

/**
 * @brief Turn the RC car right.
 * @return 0 on success, negative errno on failure
 */
int rc_car_turn_right(void);

/**
 * @brief Center both wheels.
 * @return 0 on successs, negative errno on failure
 */
int rc_car_center_wheels(void);

/** @} */

#endif /* APP_LIB_RC_CAR_H_ */
