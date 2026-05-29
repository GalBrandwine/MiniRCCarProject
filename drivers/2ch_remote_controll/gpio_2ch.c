/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT remote_controll_gpio_2ch

#include <zephyr/device.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/drivers/remote_controll_2ch.h>

LOG_MODULE_REGISTER(2ch_remote_controll, CONFIG_2CH_REMOTE_CONTROLL_LOG_LEVEL);

// struct blink_gpio_led_data
// {
// 	struct k_timer timer;
// };

struct remote_controll_2ch_gpio_config
{
	struct gpio_dt_spec left;
	struct gpio_dt_spec right;
	// unsigned int period_ms;
};

// static void blink_gpio_led_on_timer_expire(struct k_timer *timer)
// {
// 	const struct device *dev = k_timer_user_data_get(timer);
// 	const struct blink_gpio_led_config *config = dev->config;
// 	int ret;

// 	ret = gpio_pin_toggle_dt(&config->led);
// 	if (ret < 0)
// 	{
// 		LOG_ERR("Could not toggle LED GPIO (%d)", ret);
// 	}
// }

// static int blink_gpio_led_set_period_ms(const struct device *dev,
// 										unsigned int period_ms)
// {
// 	const struct blink_gpio_led_config *config = dev->config;
// 	struct blink_gpio_led_data *data = dev->data;

// 	if (period_ms == 0)
// 	{
// 		k_timer_stop(&data->timer);
// 		return gpio_pin_set_dt(&config->led, 0);
// 	}

// 	k_timer_start(&data->timer, K_MSEC(period_ms), K_MSEC(period_ms));

// 	return 0;
// }

static DEVICE_API(remote_controll_2ch, remote_controll_2ch_api) = {
	.turn_left = NULL,	 //&remote_controll_2ch_gpio_turn_left,
	.turn_right = NULL}; //&remote_controll_2ch_gpio_turn_right};

static int remote_controll_2ch_gpio_data_init(const struct device *dev)
{
	const struct remote_controll_2ch_gpio_config *config = dev->config;
	// struct remote_controll_2ch_gpio_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->left))
	{
		LOG_ERR("Left GPIO not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->right))
	{
		LOG_ERR("Right GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->right, GPIO_OUTPUT_INACTIVE);
	if (ret < 0)
	{
		LOG_ERR("Could not configure Right GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->left, GPIO_OUTPUT_INACTIVE);
	if (ret < 0)
	{
		LOG_ERR("Could not configure Left GPIO (%d)", ret);
		return ret;
	}
	// k_timer_init(&data->timer, blink_gpio_led_on_timer_expire, NULL);
	// k_timer_user_data_set(&data->timer, (void *)dev);

	// if (config->period_ms > 0)
	// {
	// 	k_timer_start(&data->timer, K_MSEC(config->period_ms),
	// 				  K_MSEC(config->period_ms));
	// }

	return 0;
}

#define REMOTE_CONTROLL_2CH_DEFINE(inst)                                 \
	static const struct remote_controll_2ch_gpio_config config##inst = { \
		.left = GPIO_DT_SPEC_INST_GET(inst, left_gpios),                 \
		.right = GPIO_DT_SPEC_INST_GET(inst, right_gpios),               \
	};                                                                   \
                                                                         \
	DEVICE_DT_INST_DEFINE(inst, remote_controll_2ch_gpio_data_init, NULL, NULL, &config##inst, POST_KERNEL, CONFIG_2CH_REMOTE_CONTROLL_INIT_PRIORITY, &remote_controll_2ch_api);

DT_INST_FOREACH_STATUS_OKAY(REMOTE_CONTROLL_2CH_DEFINE)
