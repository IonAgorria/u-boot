// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 */

#define LOG_CATEGORY UCLASS_PANEL_BACKLIGHT

#include <backlight.h>
#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <log.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/gpio.h>

#define LM3533_BL_DEFAULT_BRIGHTNESS 		0x71
#define LM3533_BL_MAX_BRIGHTNESS		0xFF

struct lm3533_backlight_priv {
	struct gpio_desc enable_gpio;
};

static int lm3533_backlight_write_reg(struct udevice *dev, u8 addr, u8 data)
{
	struct dm_i2c_chip *chip = dev_get_parent_plat(dev);
	u8 buf[2];
	struct i2c_msg msg[1];
	int err;

	buf[0] = addr;
	buf[1] = data;

	msg->addr = chip->chip_addr;
	msg->flags = 0;
	msg->len = ARRAY_SIZE(buf);
	msg->buf = buf;

	err = dm_i2c_xfer(dev, msg, ARRAY_SIZE(msg));
	if (err) {
		printf("%s: write failed, err = %d, addr = %#x, data = %#x\n",
			__func__, err, addr, data);
		return err;
	}
	
	return 0;
}

static int lm3533_backlight_enable(struct udevice *dev)
{
	struct lm3533_backlight_priv *priv = dev_get_priv(dev);
	int ret;

	dm_gpio_set_value(&priv->enable_gpio, 1);
	mdelay(5);

	/* HVLED 1 & 2 are controlled by Bank A */
	ret = lm3533_backlight_write_reg(dev, 0x10, 0x00); 
	if (ret)
		return ret;

	/* PWM input is disabled for CABC */
	ret = lm3533_backlight_write_reg(dev, 0x14, 0x00);
	if (ret)
		return ret;

	/* Linear & Control Bank A is configured for register Current control */
	ret = lm3533_backlight_write_reg(dev, 0x1A, 0x02);
	if (ret)
		return ret;
	
	/* Full-Scale Current (20.2mA) */
	ret = lm3533_backlight_write_reg(dev, 0x1F, 0x13);
	if (ret)
		return ret;
	
	/* Control Bank A is enable */
	ret = lm3533_backlight_write_reg(dev, 0x27, 0x01); 
	if (ret)
		return ret;

	ret = lm3533_backlight_write_reg(dev, 0x2C, 0x0A);
	if (ret)
		return ret;

	/* Set default brightness level */
	ret = lm3533_backlight_write_reg(dev, 0x40, LM3533_BL_DEFAULT_BRIGHTNESS);
	if (ret)
		return ret;

	return 0;
}

static int lm3533_backlight_set_brightness(struct udevice *dev, int percent)
{
	/* Default backlight level is set on enable */
	return 0;
}

static int lm3533_backlight_probe(struct udevice *dev)
{
	struct lm3533_backlight_priv *priv = dev_get_priv(dev);
	int ret;

	if (device_get_uclass_id(dev->parent) != UCLASS_I2C)
		return -EPROTONOSUPPORT;

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->enable_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode gate-gpios (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct backlight_ops lm3533_backlight_ops = {
	.enable = lm3533_backlight_enable,
	.set_brightness = lm3533_backlight_set_brightness,
};

static const struct udevice_id lm3533_backlight_ids[] = {
	{ .compatible = "ti,lm3533" },
	{ }
};

U_BOOT_DRIVER(lm3533_backlight) = {
	.name		= "lm3533_backlight",
	.id		= UCLASS_PANEL_BACKLIGHT,
	.of_match	= lm3533_backlight_ids,
	.probe		= lm3533_backlight_probe,
	.ops		= &lm3533_backlight_ops,
	.priv_auto	= sizeof(struct lm3533_backlight_priv),
};
