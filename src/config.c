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

#include "config.h"

#define CONFIG_FILE_NAME				"config"
#define CONFIG_FILE_SIZE				12
#define CONFIG_FILE_MAGIC				"TLCF"
#define CONFIG_FILE_VERSION				1

static uint8_t config_buf[CONFIG_FILE_SIZE];


void config_save(config_t *cfg)
{
	FIL f;
	UINT num;
	uint8_t *ptr = config_buf;

	/* First the magic, then the version, and finally the fields of
	 * the config_t struct written as u8 following the struct order
	 */
	ptr[0] = (uint8_t) CONFIG_FILE_MAGIC[0];
	ptr[1] = (uint8_t) CONFIG_FILE_MAGIC[1];
	ptr[2] = (uint8_t) CONFIG_FILE_MAGIC[2];
	ptr[3] = (uint8_t) CONFIG_FILE_MAGIC[3];
	ptr += 4;

	ptr[0] = CONFIG_FILE_VERSION & 0xFF;
	ptr += 1;

	ptr[0] = cfg->lcd_inverted & 0x1;
	ptr += 1;

	ptr[0] = cfg->backlight_always_on & 0x1;
	ptr += 1;

	ptr[0] = cfg->backlight_level & 0x1F;
	ptr += 1;

	ptr[0] = cfg->speaker_enabled & 0x1;
	ptr += 1;

	ptr[0] = cfg->led_enabled & 0x1;
	ptr += 1;

	ptr[0] = cfg->battery_enabled & 0x1;
	ptr += 1;

	ptr[0] = cfg->autosave_enabled & 0x1;
	ptr += 1;

	if (f_open(&f, CONFIG_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE)) {
		/* Error */
		return;
	}

        if (f_write(&f, config_buf, sizeof(config_buf), &num) || (num < sizeof(config_buf))) {
		/* Error */
		f_close(&f);
		return;
	}

	f_close(&f);
}

int8_t config_load(config_t *cfg)
{
	FIL f;
	UINT num;
	uint8_t *ptr = config_buf;

	if (f_open(&f, CONFIG_FILE_NAME, FA_OPEN_EXISTING | FA_READ)) {
		/* Error */
		return -1;
	}

	if (f_read(&f, config_buf, sizeof(config_buf), &num) || (num < sizeof(config_buf))) {
		/* Error */
		f_close(&f);
		return -1;
	}

	f_close(&f);

	/* First the magic, then the version, and finally the fields of
	 * the config_t struct written as u8 following the struct order
	 */
	if (ptr[0] != (uint8_t) CONFIG_FILE_MAGIC[0] || ptr[1] != (uint8_t) CONFIG_FILE_MAGIC[1] ||
		ptr[2] != (uint8_t) CONFIG_FILE_MAGIC[2] || ptr[3] != (uint8_t) CONFIG_FILE_MAGIC[3]) {
		return -1;
	}
	ptr += 4;

	if (ptr[0] != CONFIG_FILE_VERSION) {
		/* TODO: Handle migration at a point */
		return -1;
	}
	ptr += 1;

	cfg->lcd_inverted = ptr[0] & 0x1;
	ptr += 1;

	cfg->backlight_always_on = ptr[0] & 0x1;
	ptr += 1;

	cfg->backlight_level = ptr[0] & 0x1F;
	ptr += 1;

	cfg->speaker_enabled = ptr[0] & 0x1;
	ptr += 1;

	cfg->led_enabled = ptr[0] & 0x1;
	ptr += 1;

	cfg->battery_enabled = ptr[0] & 0x1;
	ptr += 1;

	cfg->autosave_enabled = ptr[0] & 0x1;
	ptr += 1;

	return 0;
}
