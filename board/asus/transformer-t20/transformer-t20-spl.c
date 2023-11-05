// SPDX-License-Identifier: GPL-2.0+
/*
 *  T20 Transformers SPL stage configuration
 *
 *  (C) Copyright 2010-2013
 *  NVIDIA Corporation <www.nvidia.com>
 *
 *  (C) Copyright 2023
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <asm/arch/tegra.h>
#include <asm/arch-tegra/tegra_i2c.h>
#include <linux/delay.h>

#define TPS658621_I2C_ADDR		(0x34 << 1)
#define TPS658621_SM1_VOLTAGE_V1	0x23
#define TPS658621_SM1_VOLTAGE_V2	0x24
#define TPS658621_SM0_VOLTAGE_V1	0x26
#define TPS658621_SM0_VOLTAGE_V2	0x27

#define TPS658621_SM1_VOLTAGE_V1_DATA	(0x0c00 | TPS658621_SM1_VOLTAGE_V1)
#define TPS658621_SM1_VOLTAGE_V2_DATA	(0x0c00 | TPS658621_SM1_VOLTAGE_V2)
#define TPS658621_SM0_VOLTAGE_V1_DATA	(0x1300 | TPS658621_SM0_VOLTAGE_V1)
#define TPS658621_SM0_VOLTAGE_V2_DATA	(0x1300 | TPS658621_SM0_VOLTAGE_V2)

void pmic_enable_cpu_vdd(void)
{
	/* Set VDD_CORE to 1.200V. */
	tegra_i2c_ll_write(TPS658621_I2C_ADDR, TPS658621_SM0_VOLTAGE_V1_DATA);
	tegra_i2c_ll_write(TPS658621_I2C_ADDR, TPS658621_SM0_VOLTAGE_V2_DATA);

	udelay(1000);

	/* Bring up VDD_CPU to 1.025V. */
	tegra_i2c_ll_write(TPS658621_I2C_ADDR, TPS658621_SM1_VOLTAGE_V1_DATA);
	tegra_i2c_ll_write(TPS658621_I2C_ADDR, TPS658621_SM1_VOLTAGE_V2_DATA);
	udelay(10 * 1000);
}
