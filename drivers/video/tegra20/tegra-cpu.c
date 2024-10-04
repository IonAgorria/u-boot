// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * This driver uses 8-bit CPU interface found in Tegra 2
 * and Tegra3 to drive MIPI DSI panel.
 */

#define LOG_DEBUG

#include <dm.h>
#include <log.h>
#include <mipi_display.h>
#include <mipi_dsi.h>
#include <backlight.h>
#include <panel.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/gpio.h>

struct tegra_cpu_bridge_priv {
	struct mipi_dsi_host host;
	struct mipi_dsi_device device;

	struct udevice *panel;
	struct display_timing timing;

	struct gpio_desc dc_gpio;
	struct gpio_desc rw_gpio;
	struct gpio_desc cs_gpio;

	struct gpio_desc data_gpios[8];
};

#define TEGRA_CPU_BRIDGE_COMM 0
#define TEGRA_CPU_BRIDGE_DATA 1

static void tegra_cpu_bridge_write(struct tegra_cpu_bridge_priv *priv,
				   u8 type, u8 value)
{
	int i;

	dm_gpio_set_value(&priv->dc_gpio, type);

	dm_gpio_set_value(&priv->cs_gpio, 0);
	dm_gpio_set_value(&priv->rw_gpio, 0);

	for (i = 0; i < 8; i++)
		dm_gpio_set_value(&priv->data_gpios[i],
			(value >> i) & 0x1);

	dm_gpio_set_value(&priv->cs_gpio, 1);
	dm_gpio_set_value(&priv->rw_gpio, 1);

	udelay(10);

	log_debug("%s: type 0x%x, val 0x%x\n",
		  __func__, type, value);
}

static ssize_t tegra_cpu_bridge_transfer(struct mipi_dsi_host *host,
					 const struct mipi_dsi_msg *msg)
{
	struct udevice *dev = (struct udevice *)host->dev;
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);
	u8 command = *(u8 *)msg->tx_buf;
	const u8 *data = msg->tx_buf;
	int i;

	tegra_cpu_bridge_write(priv, TEGRA_CPU_BRIDGE_COMM, command);

	for (i = 1; i < msg->tx_len; i++)
		tegra_cpu_bridge_write(priv, TEGRA_CPU_BRIDGE_DATA, data[i]);

	return 0;
}

static const struct mipi_dsi_host_ops tegra_cpu_bridge_host_ops = {
	.transfer	= tegra_cpu_bridge_transfer,
};

static int tegra_cpu_bridge_enable_panel(struct udevice *dev)
{
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);

	/* Perform panel setup */
	return panel_enable_backlight(priv->panel);
}

static int tegra_cpu_bridge_set_panel(struct udevice *dev, int percent)
{
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);

	return panel_set_backlight(priv->panel, percent);
}

static int tegra_cpu_bridge_panel_timings(struct udevice *dev,
					  struct display_timing *timing)
{
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);

	memcpy(timing, &priv->timing, sizeof(*timing));

	return 0;
}

static int tegra_cpu_bridge_hw_init(struct udevice *dev)
{
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);

	dm_gpio_set_value(&priv->cs_gpio, 0);
	dm_gpio_set_value(&priv->cs_gpio, 1);

	dm_gpio_set_value(&priv->rw_gpio, 1);
	dm_gpio_set_value(&priv->dc_gpio, 0);

	return 0;
}

static int tegra_cpu_bridge_probe(struct udevice *dev)
{
	struct tegra_cpu_bridge_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_device *device = &priv->device;
	struct mipi_dsi_panel_plat *mipi_plat;
	int ret;

	ret = uclass_get_device_by_phandle(UCLASS_PANEL, dev,
					   "panel", &priv->panel);
	if (ret) {
		log_debug("cannot get panel: ret=%d\n", ret);
		return ret;
	}

	panel_get_display_timing(priv->panel, &priv->timing);

	mipi_plat = dev_get_plat(priv->panel);
	mipi_plat->device = device;

	priv->host.dev = (struct device *)dev;
	priv->host.ops = &tegra_cpu_bridge_host_ops;

	device->host = &priv->host;
	device->lanes = mipi_plat->lanes;
	device->format = mipi_plat->format;
	device->mode_flags = mipi_plat->mode_flags;

	/* get control gpios */
	ret = gpio_request_by_name(dev, "dc-gpios", 0,
				   &priv->dc_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_debug("could not decode dc-gpios (%d)\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "rw-gpios", 0,
				   &priv->rw_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_debug("could not decode rw-gpios (%d)\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "cs-gpios", 0,
				   &priv->cs_gpio, GPIOD_IS_OUT);
	if (ret) {
		log_debug("could not decode cs-gpios (%d)\n", ret);
		return ret;
	}

	/* get data gpios */
	ret = gpio_request_list_by_name(dev, "data-gpios",
					priv->data_gpios, 8,
					GPIOD_IS_OUT);
	if (ret < 0) {
		log_debug("could not decode data-gpios (%d)\n", ret);
		return ret;
	}

	return tegra_cpu_bridge_hw_init(dev);
}

static const struct panel_ops tegra_cpu_bridge_ops = {
	.enable_backlight	= tegra_cpu_bridge_enable_panel,
	.set_backlight		= tegra_cpu_bridge_set_panel,
	.get_display_timing	= tegra_cpu_bridge_panel_timings,
};

static const struct udevice_id tegra_cpu_bridge_ids[] = {
	{ .compatible = "nvidia,tegra-cpu-8bit" },
	{ }
};

U_BOOT_DRIVER(tegra_cpu_8bit) = {
	.name		= "tegra_cpu_8bit",
	.id		= UCLASS_PANEL,
	.of_match	= tegra_cpu_bridge_ids,
	.ops		= &tegra_cpu_bridge_ops,
	.probe		= tegra_cpu_bridge_probe,
	.priv_auto	= sizeof(struct tegra_cpu_bridge_priv),
};
