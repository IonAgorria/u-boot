// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * Panel specific configuration of bridge and panel itself.
 */

#include <common.h>
#include <dm.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <log.h>
#include <misc.h>
#include <panel.h>
#include <power/regulator.h>
#include <asm/gpio.h>

#include "ssd2825.h"

struct koe_panel_priv {
	struct udevice *regulator;
	struct udevice *spi;

	struct gpio_desc panel_en_gpio;
	struct gpio_desc panel_rst_gpio;
};

static const u8 exit_sleep[] = { 0x11, 0x00 };
static const u8 addr_mode[] = { 0x36, 0x00 };
static const u8 pixel_format[] = { 0x3A, 0x70 };
static const u8 mcap_off[] = { 0xB0, 0x04 };
static const u8 mcap_on[] = { 0xB0, 0x03 };
static const u8 panel_enable[] = { 0x29, 0x00 };

static const u8 dig_contrast[] = {
	0xCC, 0xDC, 0xB4, 0xFF
};

static const u8 gamma_curve_3[] = {
	0xC8, 0x0B, 0x0D, 0x10, 0x14,
	0x13, 0x1D, 0x20, 0x18, 0x12,
	0x09, 0x07, 0x06, 0x0A, 0x0C,
	0x10, 0x14, 0x13, 0x1D, 0x20,
	0x18, 0x12, 0x09, 0x07, 0x06
};

static const u8 column_inversion[] = {
	0xC1, 0x00, 0x50, 0x03, 0x22,
	0x16, 0x06, 0x60, 0x11
};

static void write_hw_register(struct koe_panel_priv *priv, u8 reg,
			      u16 command)
{
	misc_write(priv->spi, reg, &command, SSD2825_CMD_SEND | SSD2825_DAT_SEND);
}

static void write_hw_dsi(struct koe_panel_priv *priv, const u8 *command,
			      int len)
{
	int i;

	misc_write(priv->spi, SSD2825_PACKET_SIZE_CTRL_REG_1, &len,
			SSD2825_CMD_SEND | SSD2825_DAT_SEND);

	misc_write(priv->spi, SSD2825_PACKET_DROP_REG, NULL,
			SSD2825_CMD_SEND);

	for (i = 0; i < len; i++) {
		misc_write(priv->spi, 0, &command[i], SSD2825_DSI_SEND);
	}
}

static int koe_panel_enable_backlight(struct udevice *dev)
{
	struct koe_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dm_gpio_set_value(&priv->panel_en_gpio, 1);
	if (ret) {
		printf("%s: error changing enable-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(5);

	ret = regulator_set_enable_if_allowed(priv->regulator, 1);
	if (ret) {
		printf("%s: error enabling power-supply (%d)\n", __func__, ret);
		return ret;
	}

	ret = dm_gpio_set_value(&priv->panel_rst_gpio, 0);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}
	mdelay(5);

	ret = dm_gpio_set_value(&priv->panel_rst_gpio, 1);
	if (ret) {
		printf("%s: error changing reset-gpios (%d)\n", __func__, ret);
		return ret;
	}

	mdelay(5);

	// clk here?

	/* Bridge configuration */
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_1, 0x0205);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_2, 0x0A56);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_3, 0x1874);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_4, 0x0300);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_5, 0x0400);
	write_hw_register(priv, SSD2825_RGB_INTERFACE_CTRL_REG_6, 0xE007);
	write_hw_register(priv, SSD2825_LANE_CONFIGURATION_REG, 0x0003);
	write_hw_register(priv, SSD2825_TEST_REG, 0x0004);

	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0000);
	write_hw_register(priv, SSD2825_LINE_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_DELAY_ADJ_REG_1, 0x2103);
	write_hw_register(priv, SSD2825_PLL_CONFIGURATION_REG, 0x8CD7);
	write_hw_register(priv, SSD2825_CLOCK_CTRL_REG, 0x0009);
	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	mdelay(10);

	/* Panel configuration */
	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_DCS |
			SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_dsi(priv, exit_sleep, sizeof(exit_sleep));
	mdelay(80);
	write_hw_dsi(priv, addr_mode, sizeof(addr_mode));
	mdelay(20);
	write_hw_dsi(priv, pixel_format, sizeof(pixel_format));

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_ECD |
			SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_dsi(priv, mcap_off, sizeof(mcap_off));
	write_hw_dsi(priv, dig_contrast, sizeof(dig_contrast));
	write_hw_dsi(priv, gamma_curve_3, sizeof(gamma_curve_3));
	write_hw_dsi(priv, column_inversion, sizeof(column_inversion));
	write_hw_dsi(priv, mcap_on, sizeof(mcap_on));

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_CKE | SSD2825_CONF_REG_DCS |
			SSD2825_CONF_REG_ECD | SSD2825_CONF_REG_EOT);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_dsi(priv, panel_enable, sizeof(panel_enable));
	mdelay(10);

	write_hw_register(priv, SSD2825_PLL_CTRL_REG, 0x0001);
	write_hw_register(priv, SSD2825_VC_CTRL_REG, 0x0000);

	write_hw_register(priv, SSD2825_CONFIGURATION_REG,
			SSD2825_CONF_REG_HS | SSD2825_CONF_REG_VEN |
			SSD2825_CONF_REG_DCS | SSD2825_CONF_REG_ECD |
			SSD2825_CONF_REG_EOT);

	return 0;
}

static int koe_panel_set_backlight(struct udevice *dev, int percent)
{
	return 0;
}

static int koe_panel_probe(struct udevice *dev)
{
	struct koe_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_MISC, dev,
					   "bridge-spi", &priv->spi);
	if (ret) {
		debug("%s: Cannot get backlight: ret = %d\n", __func__, ret);
		return log_ret(ret);
	}

	if (IS_ENABLED(CONFIG_DM_REGULATOR)) {
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
						   "power-supply", &priv->regulator);
		if (ret) {
			debug("%s: Warning: cannot get power supply: ret = %d\n",
			      __func__, ret);
			if (ret != -ENOENT)
				return ret;
		}
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &priv->panel_en_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode enable-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 0,
				   &priv->panel_rst_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode reser-gpios (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct panel_ops koe_panel_ops = {
	.enable_backlight	= koe_panel_enable_backlight,
	.set_backlight		= koe_panel_set_backlight,
};

static const struct udevice_id koe_panel_ids[] = {
	{ .compatible = "koe,tx13d100vm0eaa" },
	{ .compatible = "hitachi,tx13d100vm0eaa" },
	{ }
};

U_BOOT_DRIVER(koe_tx13d100vm0eaa) = {
	.name		= "koe_tx13d100vm0eaa",
	.id			= UCLASS_PANEL,
	.of_match	= koe_panel_ids,
	.ops		= &koe_panel_ops,
	.probe		= koe_panel_probe,
	.priv_auto	= sizeof(struct koe_panel_priv),
};
