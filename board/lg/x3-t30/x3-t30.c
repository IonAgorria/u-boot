// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  (C) Copyright 2010-2013
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2021
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <log.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include "pinmux-config-x3.h"

#define PMC_CLK_OUT_CNTRL_0		0x01A8

#define MAX77663_I2C_ADDR		0x1C	/* 7 bit */
#define MAX77663_REG_SD2		0x18
#define MAX77663_REG_LDO3		0x29
#define MAX77663_REG_LDO5		0x2D

#define MAX14526_I2C_ADDR		0x44	/* 7 bit */
#define CONTROL_1			0x01
#define SW_CONTROL			0x03
#define ID_200				0x10
#define ADC_EN				0x02
#define CP_EN				0x01
#define DP_USB				0x00
#define DP_UART				0x08
#define DP_AUDIO			0x10
#define DP_OPEN				0x38
#define DM_USB				0x00
#define DM_UART				0x01
#define DM_AUDIO			0x02
#define DM_OPEN				0x07

#ifdef CONFIG_CMD_POWEROFF
int do_poweroff(struct cmd_tbl *cmdtp, int flag,
	        int argc, char *const argv[])
{
	/* To be added */

	// wait some time and then print error
	mdelay(5000);
	printf("Failed to power off!!!\n");
	return 1;
}
#endif

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_config_pingrp_table(tegra3_x3_pinmux_common,
		ARRAY_SIZE(tegra3_x3_pinmux_common));

#ifdef CONFIG_DEVICE_P880
	pinmux_config_pingrp_table(tegra3_p880_pinmux,
		ARRAY_SIZE(tegra3_p880_pinmux));
#endif

#ifdef CONFIG_DEVICE_P895
	pinmux_config_pingrp_table(tegra3_p895_pinmux,
		ARRAY_SIZE(tegra3_p895_pinmux));
#endif
}

#ifdef CONFIG_MMC_SDHCI_TEGRA
/*
 * Do I2C/PMU writes to bring up SD card and eMMC bus power
 */
void board_sdmmc_voltage_init(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	ret = i2c_get_chip_for_busnum(0, MAX77663_I2C_ADDR, 1, &dev);
	if (ret) {
		debug("%s: Cannot find PMIC I2C chip\n", __func__);
		return;
	}

	/* 0x60 for 1.8v, bit7:0 = voltage */
	val = 0x60;
	ret = dm_i2c_write(dev, MAX77663_REG_SD2, &val, 1);
	if (ret)
		printf("vdd_1v8_vio set failed: %d\n", ret);

	/* 0xEC for 3.00v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xEC;
	ret = dm_i2c_write(dev, MAX77663_REG_LDO3, &val, 1);
	if (ret)
		printf("vdd_usd set failed: %d\n", ret);

	/* 0xE9 for 2.85v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xE9;
	ret = dm_i2c_write(dev, MAX77663_REG_LDO5, &val, 1);
	if (ret)
		printf("vcore_emmc set failed: %d\n", ret);

#ifdef CONFIG_DEVICE_P880
	/* Enable keyboard led */
	gpio_request(TEGRA_GPIO(R, 2), "KB_LED");
	gpio_direction_output(TEGRA_GPIO(R, 2), 1);
#endif

#ifdef CONFIG_DEVICE_P895
	/* Enable power button led */
	gpio_request(TEGRA_GPIO(R, 3), "POW_LED");
	gpio_direction_output(TEGRA_GPIO(R, 3), 1);
#endif
}

/*
 * Routine: pin_mux_mmc
 * Description: setup the MMC muxes, power rails, etc.
 */
void pin_mux_mmc(void)
{
	/*
	 * NOTE: We don't do mmc-specific pin muxes here.
	 * They were done globally in pinmux_init().
	 */

	/* Bring up uSD and eMMC power */
	board_sdmmc_voltage_init();
}
#endif	/* MMC */

#ifdef CONFIG_USB_EHCI_TEGRA
/* USB mode. Non mandatory */
void muic_ap_usb_setup(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	ret = i2c_get_chip_for_busnum(1, MAX14526_I2C_ADDR, 1, &dev);
	if (ret) {
		debug("%s: Cannot find PMIC I2C chip\n", __func__);
		return;
	}

	/* Connect CP UART signals to AP */
	gpio_request(TEGRA_GPIO(Y, 3), "USIF_SW");
	gpio_direction_output(TEGRA_GPIO(Y, 3), 0);

	/* Connect CP UART to MUIC UART */
	gpio_request(TEGRA_GPIO(R, 4), "IFX_USB_VBUS_EN");
	gpio_direction_output(TEGRA_GPIO(R, 4), 0);

	gpio_request(TEGRA_GPIO(CC, 2), "DP3T_SW");
	gpio_direction_output(TEGRA_GPIO(CC, 2), 0);

	/* Enables USB Path */
	val = DP_USB | DM_USB;
	ret = dm_i2c_write(dev, SW_CONTROL, &val, 1);
	if (ret)
		printf("USB Path set failed: %d\n", ret);

	/* Enables 200K, Charger Pump, and ADC */
	val = ID_200 | ADC_EN | CP_EN;
	ret = dm_i2c_write(dev, CONTROL_1, &val, 1);
	if (ret)
		printf("200K, Charger Pump, and ADC set failed: %d\n", ret);
}

/*
 * Routine: pin_mux_usb
 * Description: setup the USB muxes, power rails, etc.
 */
void pin_mux_usb(void)
{
	/*
	 * NOTE: We don't do mmc-specific pin muxes here.
	 * They were done globally in pinmux_init().
	 */

	/* Set up MUIC for proper usb use */
	muic_ap_usb_setup();
}
#endif

int nvidia_board_init(void)
{
	u32 clk_out_3 = BIT(18) | BIT(22) | BIT(23);

	clock_start_periph_pll(PERIPH_ID_EXTPERIPH3, CLOCK_ID_CGENERAL,
			       24 * 1000000);
	clock_enable(PERIPH_ID_EXTPERIPH3);
	udelay(5);
	reset_set_enable(PERIPH_ID_EXTPERIPH3, 0);

	/* Configure clk_out_3 */
	tegra_pmc_writel(clk_out_3, PMC_CLK_OUT_CNTRL_0);

	return 0;
}
