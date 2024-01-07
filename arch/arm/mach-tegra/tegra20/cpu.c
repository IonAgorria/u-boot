// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/funcmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/tegra_i2c.h>
#include <linux/delay.h>
#include "../cpu.h"

#define NV_PA_DVC_I2C_BASE	0x7000d000

/* In case this function is not defined */
__weak void pmic_enable_cpu_vdd(void) {}

static void enable_cpu_power_rail(void)
{
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	u32 reg;

	reg = readl(&pmc->pmc_cntrl);
	reg |= CPUPWRREQ_OE;
	writel(reg, &pmc->pmc_cntrl);

	/*
	 * The TI PMU65861C needs a 3.75ms delay between enabling
	 * the power rail and enabling the CPU clock.  This delay
	 * between SM1EN and SM1 is for switching time + the ramp
	 * up of the voltage to the CPU (VDD_CPU from PMU).
	 */
	udelay(3750);
}

static void t20_init_clocks(void)
{
	struct dvc_ctlr *dvc = (struct dvc_ctlr *)NV_PA_DVC_I2C_BASE;

	funcmux_select(PERIPH_ID_DVC_I2C, FUNCMUX_DVC_I2CP);

	/* Put i2c in reset and enable clocks */
	reset_set_enable(PERIPH_ID_DVC_I2C, 1);
	clock_set_enable(PERIPH_ID_DVC_I2C, 1);

	/*
	 * Our high-level clock routines are not available prior to
	 * relocation. We use the low-level functions which require a
	 * hard-coded divisor. Use CLK_M with divide by (n + 1 = 17)
	 */
	clock_ll_set_source_divisor(PERIPH_ID_DVC_I2C, 3, 16);

	/* Give clocks time to stabilize, then take i2c out of reset */
	udelay(1000);
	reset_set_enable(PERIPH_ID_DVC_I2C, 0);

	/* Enable software control over the line */
	setbits_le32(&dvc->ctrl3, DVC_CTRL_REG3_I2C_HW_SW_PROG_MASK);
}

void start_cpu(u32 reset_vector)
{
	t20_init_clocks();

	/* Enable VDD_CPU */
	enable_cpu_power_rail();
	pmic_enable_cpu_vdd();

	/* Hold the CPUs in reset */
	reset_A9_cpu(1);

	/* Disable the CPU clock */
	enable_cpu_clock(0);

	/* Enable CoreSight */
	clock_enable_coresight(1);

	/*
	 * Set the entry point for CPU execution from reset,
	 *  if it's a non-zero value.
	 */
	if (reset_vector)
		writel(reset_vector, EXCEP_VECTOR_CPU_RESET_VECTOR);

	/* Enable the CPU clock */
	enable_cpu_clock(1);

	/* If the CPU doesn't already have power, power it up */
	powerup_cpu();

	/* Take the CPU out of reset */
	reset_A9_cpu(0);
}
