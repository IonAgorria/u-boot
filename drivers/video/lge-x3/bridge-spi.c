// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2022 Svyatoslav Ryhel <clamor95@gmail.com>
 *
 * Bitbang emulation of spi.
 */

#include <common.h>
#include <dm.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <log.h>
#include <misc.h>
#include <asm/gpio.h>

#include "ssd2825.h"

/*
 * According to the "8 Bit 3 Wire SPI Interface Timing Characteristics"
 * and "TX_CLK Timing Characteristics" tables in the SSD2825 datasheet,
 * the lowest possible 'tx_clk' clock frequency is 8MHz, and SPI runs
 * at 1/8 of that after reset. So using 1 microsecond delays is safe in
 * the main loop. But the delays around chip select pin manipulations
 * need to be longer (up to 16 'tx_clk' cycles, or 2 microseconds in
 * the worst case).
 */
#define SPI_DELAY_US		1
#define SPI_CS_DELAY_US		2

struct bridge_spi_priv {
	struct gpio_desc csx_gpio;
	struct gpio_desc sck_gpio;
	struct gpio_desc sdi_gpio;
	struct gpio_desc sdo_gpio;
};

/*
 * SPI transfer, using the "8-bit 3 wire" mode (that's how it is called in
 * the SSD2825 documentation). The 'dout' input parameter specifies 9-bits
 * of data to be written to SSD2825.
 */
static void soft_spi_write_8bit_3wire(struct bridge_spi_priv *priv, u16 dout)
{
	int j;

	dm_gpio_set_value(&priv->csx_gpio, 0);
	udelay(SPI_CS_DELAY_US);

	for (j = 8; j >= 0; j--) {
		dm_gpio_set_value(&priv->sck_gpio, 0);

		dm_gpio_set_value(&priv->sdi_gpio, (dout & BIT(j)) != 0);
		udelay(SPI_DELAY_US);

		dm_gpio_set_value(&priv->sck_gpio, 1);
		udelay(SPI_DELAY_US);
	}

	udelay(SPI_CS_DELAY_US);
	dm_gpio_set_value(&priv->csx_gpio, 1);
	udelay(SPI_CS_DELAY_US);
}

static void soft_spi_read_8bit_3wire(struct bridge_spi_priv *priv, u16 *din)
{
	int j;
	u16 tmpdin = 0, low_byte, high_byte;

	dm_gpio_set_value(&priv->csx_gpio, 0);
	udelay(SPI_CS_DELAY_US);

	for (j = 16; j > 0; j--) {
		dm_gpio_set_value(&priv->sck_gpio, 0);

		tmpdin = (tmpdin << 1) | dm_gpio_get_value(&priv->sdo_gpio);
		udelay(SPI_DELAY_US);

		dm_gpio_set_value(&priv->sck_gpio, 1);
		udelay(SPI_DELAY_US);
	}

	udelay(SPI_CS_DELAY_US);
	dm_gpio_set_value(&priv->csx_gpio, 1);
	udelay(SPI_CS_DELAY_US);

	low_byte = (tmpdin & 0xFF00) >> 8;
	high_byte = (tmpdin & 0x00FF) << 8;

	printf("%s: spi-read (0x%x)\n", __func__, (high_byte | low_byte));

	*din = high_byte | low_byte;
}

static int bridge_spi_write(struct udevice *dev, int reg,
					const void *buf, int flags)
{
	struct bridge_spi_priv *priv = dev_get_priv(dev);

	if (flags & SSD2825_CMD_SEND)
		soft_spi_write_8bit_3wire(priv, SSD2825_CMD_MASK | reg);

	if (flags & SSD2825_DAT_SEND) {
		u16 data = *(u16 *)buf;
		u8 cmd1, cmd2;

		/* send low byte first and then high byte */
		cmd1 = (data & 0x00FF);
		cmd2 = (data & 0xFF00) >> 8;

		printf("%s: reg (0x%x), command (0x%x%x)\n", __func__, reg, cmd2, cmd1);

		soft_spi_write_8bit_3wire(priv, SSD2825_DAT_MASK | cmd1);
		soft_spi_write_8bit_3wire(priv, SSD2825_DAT_MASK | cmd2);
	}

	if (flags & SSD2825_DSI_SEND) {
		u16 data = *(u16 *)buf;
		data &= 0x00FF;

		printf("%s: dsi command (0x%x)\n", __func__, data);

		soft_spi_write_8bit_3wire(priv, SSD2825_DAT_MASK | data);
	}

	return 0;
}

static int bridge_spi_read(struct udevice *dev, int reg,
				void *data, int flags)
{
	struct bridge_spi_priv *priv = dev_get_priv(dev);

	soft_spi_write_8bit_3wire(priv, SSD2825_CMD_MASK | SSD2825_SPI_READ_REG);
	soft_spi_write_8bit_3wire(priv, SSD2825_DAT_MASK | SSD2825_SPI_READ_REG_RESET);
	soft_spi_write_8bit_3wire(priv, SSD2825_DAT_MASK);

	soft_spi_write_8bit_3wire(priv, SSD2825_CMD_MASK | reg);
	soft_spi_write_8bit_3wire(priv, SSD2825_CMD_MASK | SSD2825_SPI_READ_REG_RESET);

	soft_spi_read_8bit_3wire(priv, (u16 *)data);

	return 0;
}

static int bridge_spi_probe(struct udevice *dev)
{
	struct bridge_spi_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "csx-gpios", 0,
				   &priv->csx_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode csx-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "sck-gpios", 0,
				   &priv->sck_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode sck-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "sdi-gpios", 0,
				   &priv->sdi_gpio, GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Could not decode sdi-gpios (%d)\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "sdo-gpios", 0,
				   &priv->sdo_gpio, GPIOD_IS_IN);
	if (ret) {
		printf("%s: Could not decode sdo-gpios (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static const struct misc_ops bridge_spi_ops = {
	.write	= bridge_spi_write,
	.read	= bridge_spi_read,
};

static const struct udevice_id bridge_spi_ids[] = {
	{ .compatible = "lge,bridge-spi" },
	{ }
};

U_BOOT_DRIVER(bridge_spi) = {
	.name		= "bridge_spi",
	.id			= UCLASS_MISC,
	.of_match	= bridge_spi_ids,
	.ops		= &bridge_spi_ops,
	.probe		= bridge_spi_probe,
	.priv_auto	= sizeof(struct bridge_spi_priv),
};
