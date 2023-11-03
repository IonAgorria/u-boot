// SPDX-License-Identifier: GPL-2.0+
/*
 *  T20 LG Star SPL stage configuration
 *
 *  (C) Copyright 2010-2013
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2023
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <asm/arch-tegra/tegra.h>
#include <asm/arch-tegra/tegra_i2c.h>
#include <linux/delay.h>

#define MAX8907_I2C_ADDR		(0x3c << 1)

#define MAX8907_REG_SDCTL1		0x04
#define MAX8907_REG_SDV1		0x06
#define MAX8907_REG_SDCTL2		0x07
#define MAX8907_REG_SDV2		0x09
#define MAX8907_REG_SDCTL3		0x0A
#define MAX8907_REG_SDV3		0x0C
#define MAX8907_REG_LDOCTL1		0x18
#define MAX8907_REG_LDO1VOUT		0x1A
#define MAX8907_REG_LDOCTL4		0x24
#define MAX8907_REG_LDO4VOUT		0x26

#define MAX8907_REG_SDCTL1_DATA		(0x0100 | MAX8907_REG_SDCTL1)
#define MAX8907_REG_SDV1_DATA		(0x1600 | MAX8907_REG_SDV1)
#define MAX8907_REG_SDCTL2_DATA		(0x0100 | MAX8907_REG_SDCTL2)
#define MAX8907_REG_SDV2_DATA		(0x2d00 | MAX8907_REG_SDV2)
#define MAX8907_REG_SDCTL3_DATA		(0x0100 | MAX8907_REG_SDCTL3)
#define MAX8907_REG_SDV3_DATA		(0x1500 | MAX8907_REG_SDV3)
#define MAX8907_REG_LDOCTL1_DATA	(0x0100 | MAX8907_REG_LDOCTL1)
#define MAX8907_REG_LDO1VOUT_DATA	(0x3300 | MAX8907_REG_LDO1VOUT)
#define MAX8907_REG_LDOCTL4_DATA	(0x0100 | MAX8907_REG_LDOCTL4)
#define MAX8907_REG_LDO4VOUT_DATA	(0x3300 | MAX8907_REG_LDO4VOUT)

#define MAX8952_I2C_ADDR		(0x60 << 1)
#define MAX8952_MODE1			0x01
#define MAX8952_MODE1_DATA		(0x1800 | MAX8952_MODE1)

void pmic_enable_cpu_vdd(void)
{
	/* Set VDD_CORE to 1.200V. */
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDV2_DATA);
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDCTL2_DATA);
	udelay(1000);

	/* Set VDDIO_SYS to 1.800V. */
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDV3_DATA);
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDCTL3_DATA);
	udelay(1000);

	/* Set VCC_IO to 1.200V. */
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDV1_DATA);
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_SDCTL1_DATA);
	udelay(1000);

	/* Set VDDIO_RX_DDR to 3.300V. */
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_LDO1VOUT_DATA);
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_LDOCTL1_DATA);
	udelay(1000);

	/* Set AVDD_USB to 3.300V. */
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_LDO4VOUT_DATA);
	tegra_i2c_ll_write(MAX8907_I2C_ADDR, MAX8907_REG_LDOCTL4_DATA);
	udelay(1000);

	/* Bring up VDD_CPU to 1.01V. */
	tegra_i2c_ll_write(MAX8952_I2C_ADDR, MAX8952_MODE1_DATA);
	udelay(10 * 1000);
}
