/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2010 - 2011 NVIDIA Corporation <www.nvidia.com>
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

//Slot that contains SBK when booting U-Boot as first bootloader
#define TEGRA_AES_SLOT_SBK 0

/**
 * Sign a block of data
 *
 * \param source	Source data
 * \param length	Size of source data
 * \param signature	Destination address for signature, AES_KEY_LENGTH bytes
 */
int sign_data_block(u8 *source, unsigned int length, u8 *signature);

/**
 * Encrypt a block of data
 *
 * \param source	Source data
 * \param dest		Destination data
 * \param length	Size of source data
 * \param key		AES128 encryption key
 */
int encrypt_data_block(u8 *source, u8 *dest, unsigned int length);

/**
 * Decrypt a block of data
 *
 * \param source	Source data
 * \param dest		Destination data
 * \param length	Size of source data
 * \param key		AES128 encryption key
 */
int decrypt_data_block(u8 *source, u8 *dest, unsigned int length);

#endif /* #ifndef _CRYPTO_H_ */
