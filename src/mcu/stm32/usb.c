/*
 * MCUGotchi - A Tamagotchi P1 emulator for microcontrollers
 *
 * Copyright (C) 2021 Jean-Christophe Rona <jc@rona.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdint.h>

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"

#include "system.h"
#include "storage.h"
#include "usb.h"

#define STORAGE_LUN_NBR					1
#define STORAGE_BLK_NBR					0x10000
#define STORAGE_BLK_SIZE				512

static USBD_HandleTypeDef USBD_Device;
extern PCD_HandleTypeDef g_hpcd;

static uint8_t state_lock = 0;

static int8_t msc_inquiry_data[] = { /* 36 */
	/* LUN 0 */
	0x00,
	0x80,
	0x02,
	0x02,
	(STANDARD_INQUIRY_DATA_LEN - 5),
	0x00,
	0x00,
	0x00,
	'M', 'G', ' ', 'H', 'W', ' ', ' ', ' ', /* Manufacturer: 8 bytes  */
	'M', 'C', 'U', 'G', 'o', 't', 'c', 'h', /* Product     : 16 Bytes */
	'i', ' ', 'D', 'e', 'v', 'i', 'c', 'e',
	'0', '.', '0','1',                      /* Version     : 4 Bytes  */
};


static int8_t msc_init(uint8_t lun)
{
	return 0;
}

static int8_t msc_get_capacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
	*block_num = (STORAGE_FS_SIZE << 2)/STORAGE_BLK_SIZE;
	*block_size = STORAGE_BLK_SIZE;

	return 0;
}

static int8_t msc_is_ready(uint8_t lun)
{
	return 0;
}

static int8_t msc_is_write_protected(uint8_t lun)
{
	return 0;
}

static int8_t msc_read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
	return storage_read(STORAGE_FS_OFFSET + blk_addr * (STORAGE_BLK_SIZE >> 2), (uint32_t *) buf, blk_len * (STORAGE_BLK_SIZE >> 2));
}

static int8_t msc_write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
	return storage_write(STORAGE_FS_OFFSET + blk_addr * (STORAGE_BLK_SIZE >> 2), (uint32_t *) buf, blk_len * (STORAGE_BLK_SIZE >> 2));
}

static int8_t msc_get_max_lun(void)
{
	return (STORAGE_LUN_NBR - 1);
}

static USBD_StorageTypeDef usbd_disk_fops = {
	msc_init,
	msc_get_capacity,
	msc_is_ready,
	msc_is_write_protected,
	msc_read,
	msc_write,
	msc_get_max_lun,
	msc_inquiry_data,
};

void usb_init(void)
{
	USBD_Init(&USBD_Device, &MSC_Desc, 0);
	USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);
	USBD_MSC_RegisterStorage(&USBD_Device, &usbd_disk_fops);
}

void usb_deinit(void)
{
	USBD_DeInit(&USBD_Device);
}

void usb_start(void)
{
	/* The USB does not work in low-power modes, thus those modes are not allowed */
	system_lock_max_state(STATE_SLEEP_S1, &state_lock);

	USBD_Start(&USBD_Device);
}

void usb_stop(void)
{
	USBD_Stop(&USBD_Device);

	system_unlock_max_state(STATE_SLEEP_S1, &state_lock);
}

void USB_IRQHandler(void)
{
	HAL_PCD_IRQHandler(&g_hpcd);
}
