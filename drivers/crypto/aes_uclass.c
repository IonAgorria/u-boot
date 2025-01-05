// SPDX-License-Identifier: GPL-2.0+

#define LOG_CATEGORY UCLASS_AES

#include <dm.h>

UCLASS_DRIVER(aes) = {
	.id		= UCLASS_AES,
	.name		= "aes",
};
