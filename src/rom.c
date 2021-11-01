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

#include "ff_gen_drv.h"

#include "storage.h"
#include "rom.h"

#include "lib/tamalib.h"

/* Define this to include the ROM data at build time */
//#define ROM_BUILT_IN

#ifdef ROM_BUILT_IN
static __attribute__((used, section(".rom"))) const u12_t g_program[];
#include "rom_data.h"
#endif

/* This represents one page of u12_t from a storage point of view */
#define PAGE_SIZE_U12					((STORAGE_PAGE_SIZE << 2)/sizeof(u12_t))

#define RESET_VECTOR_ADDR_U12				0x100
#define RESET_VECTOR_ADDR_U8				(RESET_VECTOR_ADDR_U12 * sizeof(u12_t))

static char rom_file_name[] = "romX.bin";


int8_t rom_load(uint8_t slot)
{
	FIL f;
	UINT num;
	uint32_t size;
	uint32_t i = 0;
	uint8_t buf[2];
	u12_t steps[PAGE_SIZE_U12];

	if (slot >= ROM_SLOTS_NUM) {
		return -1;
	}

	rom_file_name[3] = slot + '0';

	if (f_open(&f, rom_file_name, FA_OPEN_EXISTING | FA_READ)) {
		/* Error */
		return -1;
	}

	size = f_size(&f)/2;

	while (i < size) {
		if (f_read(&f, buf, 2, &num) || (num < 2)) {
			/* Error */
			f_close(&f);
			return -1;
		}

		steps[i % PAGE_SIZE_U12] = buf[1] | ((buf[0] & 0xF) << 8);

		i++;

		if ((i % PAGE_SIZE_U12) == 0 || i == size) {
			/* Flash the page */
			if (storage_write(STORAGE_ROM_OFFSET + ((i - 1)/PAGE_SIZE_U12) * STORAGE_PAGE_SIZE, (uint32_t *) steps, ((((i - 1) % PAGE_SIZE_U12) + 1) * sizeof(u12_t) + sizeof(uint32_t) - 1)/sizeof(uint32_t)) < 0) {
				/* Error */
				f_close(&f);
				return -1;
			}
		}
	}

	f_close(&f);

	return 0;
}

uint8_t rom_stat(uint8_t slot)
{
	if (slot >= ROM_SLOTS_NUM) {
		return 0;
	}

	rom_file_name[3] = slot + '0';

	/* Check if the slot exists */
	return (f_stat(rom_file_name, NULL) == FR_OK);
}

uint8_t rom_is_loaded(void)
{
	uint8_t buf[8];
	u12_t *reset_vector;

	/* Read between 1 and 2 words */
	if (storage_read(STORAGE_ROM_OFFSET + (RESET_VECTOR_ADDR_U8 >> 2), (uint32_t *) buf, ((RESET_VECTOR_ADDR_U8 & 0x3) + sizeof(u12_t) + sizeof(uint32_t) - 1)/sizeof(uint32_t)) < 0) {
		return 0;
	}

	reset_vector = (u12_t *) &(buf[RESET_VECTOR_ADDR_U8 & 0x3]);

	/* Check that the reset vector is a regular JP instruction */
	return (((*reset_vector & 0xF00) == 0x000) && ((*reset_vector & 0x0FF) != 0x000));
}
