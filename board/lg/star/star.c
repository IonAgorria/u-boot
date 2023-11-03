// SPDX-License-Identifier: GPL-2.0+
/*
 *  (C) Copyright 2023
 *  Svyatoslav Ryhel <clamor95@gmail.com>
 */

#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/funcmux.h>

#ifdef CONFIG_MMC_SDHCI_TEGRA
/*
 * Routine: pin_mux_mmc
 * Description: setup the pin muxes/tristate values for the SDMMC(s)
 */
void pin_mux_mmc(void)
{
	funcmux_select(PERIPH_ID_SDMMC4, FUNCMUX_SDMMC4_ATB_GMA_GME_8_BIT);
	funcmux_select(PERIPH_ID_SDMMC3, FUNCMUX_SDMMC3_SDB_4BIT);
}
#endif
