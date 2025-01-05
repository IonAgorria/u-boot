// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010 - 2011 NVIDIA Corporation <www.nvidia.com>
 */

#include <log.h>
#include <linux/errno.h>
#include <asm/arch-tegra/crypto.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include "uboot_aes.h"

int sign_data_block(u8 *source, unsigned int length, u8 *signature)
{
    struct udevice* dev = NULL;
    int ret;

    ret = uclass_get_device_by_driver(UCLASS_AES, DM_DRIVER_GET(tegra_aes), &dev);
    if (ret) {
        printf("Failed to get tegra_aes: %d\n", ret);
        return ret;
    }

    const struct aes_ops *ops = device_get_ops(dev);

    if (!ops->select_key_slot)
        return -ENOSYS;

    ops->select_key_slot(dev, 128, TEGRA_AES_SLOT_SBK);
    
    return aes_cmac(dev, source, signature, length);
}

int encrypt_data_block(u8 *source, u8 *dest, unsigned int length)
{
    struct udevice* dev = NULL;
    int ret;

    ret = uclass_get_device_by_driver(UCLASS_AES, DM_DRIVER_GET(tegra_aes), &dev);
    if (ret) {
        printf("Failed to get tegra_aes: %d\n", ret);
        return ret;
    }

    const struct aes_ops *ops = device_get_ops(dev);


    if (!ops->select_key_slot || !ops->aes_cbc_encrypt)
        return -ENOSYS;

    ops->select_key_slot(dev, 128, TEGRA_AES_SLOT_SBK);

    return ops->aes_cbc_encrypt(dev, (u8*) &AES_ZERO_BLOCK, source, dest, length);
}

int decrypt_data_block(u8 *source, u8 *dest, unsigned int length)
{
    struct udevice* dev = NULL;
    int ret;

    ret = uclass_get_device_by_driver(UCLASS_AES, DM_DRIVER_GET(tegra_aes), &dev);
    if (ret) {
        printf("Failed to get tegra_aes: %d\n", ret);
        return ret;
    }

    const struct aes_ops *ops = device_get_ops(dev);

    if (!ops->select_key_slot || !ops->aes_cbc_decrypt)
        return -ENOSYS;
    
    ops->select_key_slot(dev, 128, TEGRA_AES_SLOT_SBK);

    return ops->aes_cbc_decrypt(dev, (u8*) &AES_ZERO_BLOCK, source, dest, length);
}
