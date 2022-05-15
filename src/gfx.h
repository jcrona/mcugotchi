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
#ifndef _GFX_H_
#define _GFX_H_

#include <stdint.h>

#define DISPLAY_WIDTH				128
#define DISPLAY_HEIGHT				64

typedef enum {
	COLOR_OFF_WHITE = 0,
	COLOR_ON_BLACK = 1,
} color_t;

typedef enum {
	BACKGROUND_OFF = 0,
	BACKGROUND_ON = 1,
} background_t;


void gfx_pixel(uint8_t x, uint8_t y, color_t color);
void gfx_square(uint8_t x, uint8_t y, uint8_t w, uint8_t h, color_t color);
uint8_t gfx_char(unsigned char c, uint8_t x, uint8_t y, uint8_t size, color_t color, background_t bg);
uint8_t gfx_string(char *str, uint8_t x, uint8_t y, uint8_t size, color_t color, background_t bg);
void gfx_clear(void);

void gfx_register_display(void (*cb)(uint8_t *, uint16_t));

void gfx_print_screen(void);

#endif /* _GFX_H_ */
