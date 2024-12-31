
void cmc623_pwm_brightness(u16 value) {
    if (value > 100) {
        value = 100;
    }

    reg = 0x4000 | (value<<4);

    cmc623_I2cWrite16(0x00, 0x0000);
    cmc623_I2cWrite16(0xB4, reg);
    cmc623_I2cWrite16(0x28, 0x0000);

}

static int hitachi_tx10d07vm0baa_probe(struct udevice *dev)
{
    struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);

    /* fill characteristics of DSI data link */
    plat->lanes = 2;
    plat->format = MIPI_DSI_FMT_RGB888;
    plat->mode_flags = MIPI_DSI_MODE_VIDEO;

    return hitachi_tx10d07vm0baa_hw_init(dev);
}

static const struct panel_ops samsung_cmc623_ops = {
        .enable_backlight	= hitachi_tx10d07vm0baa_enable_backlight,
        .set_backlight		= hitachi_tx10d07vm0baa_set_backlight,
        .get_display_timing	= hitachi_tx10d07vm0baa_timings,
};

static const struct udevice_id samsung_cmc623_ids[] = {
        { .compatible = "samsung,cmc623" },
        { }
};

U_BOOT_DRIVER(cmc623) = {
        .name		= "samsung_cmc623",
        .id			= UCLASS_PANEL,
        .of_match	= samsung_cmc623_ids,
        .ops		= &samsung_cmc623_ops,
        .of_to_plat	= samsung_cmc623_of_to_plat,
        .probe		= hitachi_tx10d07vm0baa_probe,
        .plat_auto	= sizeof(struct mipi_dsi_panel_plat),
        .priv_auto	= sizeof(struct hitachi_tx10d07vm0baa_priv),
};
