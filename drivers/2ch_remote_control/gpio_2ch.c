/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brandwine_remote_control_gpio_2ch

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/drivers/remote_control_2ch.h>

LOG_MODULE_REGISTER(2ch_remote_control, CONFIG_2CH_REMOTE_CONTROL_LOG_LEVEL);

struct remote_control_2ch_gpio_config
{
	struct gpio_dt_spec left;
	struct gpio_dt_spec right;
};

/* ── API implementation ─────────────────────────────────────────────────── */

static int remote_control_2ch_gpio_turn_left(const struct device *dev)
{
	const struct remote_control_2ch_gpio_config *config = dev->config;
	int ret;

	LOG_DBG("Turning left");

	ret = gpio_pin_set_dt(&config->left, 1);
	if (ret < 0)
	{
		LOG_ERR("Could not set left GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&config->right, 0);
	if (ret < 0)
	{
		LOG_ERR("Could not clear right GPIO (%d)", ret);
		return ret;
	}

	return 0;
}

static int remote_control_2ch_gpio_turn_right(const struct device *dev)
{
	const struct remote_control_2ch_gpio_config *config = dev->config;
	int ret;

	LOG_DBG("Turning right");

	ret = gpio_pin_set_dt(&config->right, 1);
	if (ret < 0)
	{
		LOG_ERR("Could not set right GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&config->left, 0);
	if (ret < 0)
	{
		LOG_ERR("Could not clear left GPIO (%d)", ret);
		return ret;
	}

	return 0;
}

static int remote_control_2ch_gpio_center(const struct device *dev)
{
	const struct remote_control_2ch_gpio_config *config = dev->config;
	int ret;

	LOG_DBG("Centering");

	ret = gpio_pin_set_dt(&config->left, 0);
	if (ret < 0)
	{
		LOG_ERR("Could not clear left GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&config->right, 0);
	if (ret < 0)
	{
		LOG_ERR("Could not clear right GPIO (%d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(remote_control_2ch, remote_control_2ch_api) = {
	.turn_left = remote_control_2ch_gpio_turn_left,
	.turn_right = remote_control_2ch_gpio_turn_right,
	.center = remote_control_2ch_gpio_center,
};

/* ── Init ───────────────────────────────────────────────────────────────── */

static int remote_control_2ch_gpio_data_init(const struct device *dev)
{
	const struct remote_control_2ch_gpio_config *config = dev->config;
	int ret;

	LOG_INF("Initialising 2ch remote control driver");
	LOG_INF("  Left  GPIO: port=%s pin=%d", config->left.port->name, config->left.pin);
	LOG_INF("  Right GPIO: port=%s pin=%d", config->right.port->name, config->right.pin);

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

	ret = gpio_pin_configure_dt(&config->left, GPIO_OUTPUT_INACTIVE);
	if (ret < 0)
	{
		LOG_ERR("Could not configure Left GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->right, GPIO_OUTPUT_INACTIVE);
	if (ret < 0)
	{
		LOG_ERR("Could not configure Right GPIO (%d)", ret);
		return ret;
	}

	LOG_INF("2ch remote control driver initialised successfully");

	return 0;
}

/* ── Device instantiation ───────────────────────────────────────────────── */

#define REMOTE_CONTROL_2CH_DEFINE(inst)                                 \
	static const struct remote_control_2ch_gpio_config config##inst = { \
		.left = GPIO_DT_SPEC_INST_GET(inst, left_gpios),                \
		.right = GPIO_DT_SPEC_INST_GET(inst, right_gpios),              \
	};                                                                  \
                                                                        \
	DEVICE_DT_INST_DEFINE(inst,                                         \
						  remote_control_2ch_gpio_data_init,            \
						  NULL,                                         \
						  NULL,                                         \
						  &config##inst,                                \
						  POST_KERNEL,                                  \
						  CONFIG_2CH_REMOTE_CONTROL_INIT_PRIORITY,      \
						  &remote_control_2ch_api);

DT_INST_FOREACH_STATUS_OKAY(REMOTE_CONTROL_2CH_DEFINE)