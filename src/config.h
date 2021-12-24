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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

typedef struct {
	uint8_t lcd_inverted;
	uint8_t backlight_always_on;
	uint8_t backlight_level;
	uint8_t speaker_enabled;
	uint8_t led_enabled;
	uint8_t battery_enabled;
	uint8_t autosave_enabled;
} config_t;


void config_save(config_t *cfg);
int8_t config_load(config_t *cfg);

#endif /* _CONFIG_H_ */
