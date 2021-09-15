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
#include "stm32f0xx_hal.h"

#include "storage.h"

#define STORAGE_BASE_ADDRESS				0x8010000
#define PAGE_SIZE					512 // in words (sizeof(uint32_t))


static void flash_read(uint32_t addr, uint32_t *data, uint32_t length)
{
	__IO uint32_t *ptr = (__IO uint32_t *) addr;

	while (length-- > 0) {
		*(data++) = *(ptr++);
	}
}

static int8_t flash_write(uint32_t addr, uint32_t *data, uint32_t length)
{
	__IO uint32_t *ptr = (__IO uint32_t *) addr;
	HAL_StatusTypeDef status;

	while (length-- > 0) {
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t) (ptr++), (uint32_t) *(data++));
		if (status != HAL_OK) {
			return -1;
		}
	}

	return 0;
}

static int16_t flash_erase_page(uint32_t addr)
{
	uint32_t error;

	FLASH_EraseInitTypeDef erase_init;

	erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
	erase_init.PageAddress = addr;
	erase_init.NbPages = 1;

	return (HAL_FLASHEx_Erase(&erase_init, &error) == HAL_OK ? 0 : -1);
}

int8_t storage_read(uint32_t offset, uint32_t *data, uint32_t length)
{
	if (length == 0) {
		/* Nothing to do */
		return 0;
	}

	if ((offset + length) * sizeof(uint32_t) > STORAGE_SIZE) {
		return -1;
	}

	flash_read(STORAGE_BASE_ADDRESS + (offset << 2), data, length);

	return 0;
}

static int8_t write_within_page(uint32_t addr, uint32_t *data, uint32_t length)
{
	uint32_t page[PAGE_SIZE];
	uint32_t page_addr = addr & 0xFFFFF800;
	uint32_t offset_in_page = (addr & 0x7FC) >> 2;
	uint32_t i = 0;

	if (length > PAGE_SIZE - offset_in_page) {
		return -1;
	}

	/* Read the page if needed */
	if (offset_in_page != 0 ||  length < PAGE_SIZE) {
		flash_read(page_addr, page, PAGE_SIZE);
	}

	/* Update the page */
	for (i = 0; i < length; i++) {
		page[offset_in_page + i] = data[i];
	}

	/* Erase the page */
	if (flash_erase_page(page_addr) < 0) {
		return -1;
	}

	/* Write the page back */
	if (flash_write(page_addr, page, PAGE_SIZE) < 0) {
		return -1;
	}

	return 0;
}

int8_t storage_write(uint32_t offset, uint32_t *data, uint32_t length)
{
	uint32_t page_len;
	uint32_t addr = STORAGE_BASE_ADDRESS + (offset << 2);

	HAL_FLASH_Unlock();

	while (length > 0) {
		if (addr & 0x7FC) {
			/* First partial page */
			page_len = PAGE_SIZE - ((addr & 0x7FC) >> 2);
		} else {
			/* Full page */
			page_len = PAGE_SIZE;
		}

		if (page_len > length) {
			/* Partial page */
			page_len = length;
		}

		if (write_within_page(addr, data, page_len) < 0) {
			HAL_FLASH_Lock();
			return -1;
		}

		addr += page_len << 2;
		data += page_len;
		length -= page_len;
	}

	HAL_FLASH_Lock();

	return 0;
}

int8_t storage_erase(void)
{
	uint32_t i;

	HAL_FLASH_Unlock();

	for (i = 0; i < (STORAGE_SIZE >> 2)/PAGE_SIZE; i++) {
		if (flash_erase_page(STORAGE_BASE_ADDRESS + i * (PAGE_SIZE << 2)) < 0) {
			HAL_FLASH_Lock();
			return -1;
		}
	}

	HAL_FLASH_Lock();

	return 0;
}
