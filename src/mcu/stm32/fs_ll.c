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
#include "stm32_hal.h"
#include "ff_gen_drv.h"

#include "storage.h"
#include "fs_ll.h"

#define STORAGE_BLK_SIZE				512

static FATFS storage_drv_fs;
static char storage_drv_path[4];

static volatile DSTATUS status = STA_NOINIT;


static DSTATUS storage_drv_initialize(BYTE lun)
{
	status &= ~STA_NOINIT;
	return status;
}

static DSTATUS storage_drv_status(BYTE lun)
{
	return status;
}

static DRESULT storage_drv_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	if (storage_read(STORAGE_FS_OFFSET + sector * (STORAGE_BLK_SIZE >> 2), (uint32_t *) buff, count * (STORAGE_BLK_SIZE >> 2)) < 0) {
		return RES_ERROR;
	}

	return RES_OK;
}

#if _USE_WRITE == 1
static DRESULT storage_drv_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	if (storage_write(STORAGE_FS_OFFSET + sector * (STORAGE_BLK_SIZE >> 2), (uint32_t *) buff, count * (STORAGE_BLK_SIZE >> 2)) < 0) {
		return RES_ERROR;
	}

	return RES_OK;
}
#endif

#if _USE_IOCTL == 1
static DRESULT storage_drv_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;

	if (status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	switch (cmd) {
		/* Make sure that no pending write process */
		case CTRL_SYNC :
			res = RES_OK;
			break;

		/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT :
			*((DWORD*) buff) = (STORAGE_FS_SIZE << 2)/STORAGE_BLK_SIZE;
			res = RES_OK;
			break;

		/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE :
			*((WORD*) buff) = STORAGE_BLK_SIZE;
			res = RES_OK;
			break;

		/* Get erase block size (DWORD) */
		case GET_BLOCK_SIZE :
			*((DWORD*) buff) = ((STORAGE_PAGE_SIZE << 2) + STORAGE_BLK_SIZE - 1)/STORAGE_BLK_SIZE;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	return res;
}
#endif

static Diskio_drvTypeDef storage_drv_driver = {
	storage_drv_initialize,
	storage_drv_status,
	storage_drv_read,
#if  _USE_WRITE == 1
	storage_drv_write,
#endif
#if  _USE_IOCTL == 1
	storage_drv_ioctl,
#endif
};

void fs_ll_init(void)
{
	if (FATFS_LinkDriver(&storage_drv_driver, storage_drv_path)) {
		return;
	}
}

int8_t fs_ll_mount(void)
{
	BYTE work[_MAX_SS];

	if (f_mount(&storage_drv_fs, (TCHAR const*) storage_drv_path, 1) != FR_OK) {
		/* Format the storage if it is not valid (SFD mode) */
		if (f_mkfs((TCHAR const*) storage_drv_path, FM_SFD | FM_FAT, 0, work, sizeof work) != FR_OK) {
			return - 1;
		}
	}

	return 0;
}

int8_t fs_ll_umount(void)
{
	if (f_mount(0, (TCHAR const*) storage_drv_path, 0) != FR_OK) {
		return -1;
	}

	return 0;
}
